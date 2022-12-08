/*
 * InjRandSet.cpp
 *
 *  Created on: Jun 26, 2018
 *      Author: naxo100
 */

#include "InjRandSet.h"
#include "../state/Node.h"
//#include "../simulation/Rule.h"
#include "../state/State.h"
#include "../pattern/Environment.h"
#include "../pattern/mixture/Component.h"
#include "../pattern/mixture/Agent.h"
#include "../state/SiteGraph.h"

namespace matching {

template <typename InjType>
InjRandContainer<InjType>::InjRandContainer(const pattern::Pattern& _ptrn) : ptrn(_ptrn){}

template <typename InjType>
InjRandContainer<InjType>::~InjRandContainer(){
	for(auto inj_it = freeInjs.begin(); inj_it != freeInjs.end(); inj_it++){
#ifdef DEBUG
		for(auto inj2_it = next(inj_it); inj2_it != freeInjs.end(); inj2_it++)
			if(*inj_it == *inj2_it)
				throw invalid_argument("~InjRandContainer(): inj is repeated in freeinjs!");//TODO do not throw on destructors
#endif
		delete *inj_it;
	}
}

/*template <typename InjType> template <typename... Args>
InjType* InjRandContainer<InjType>::emplace(const state::State& state,Args... args){
	InjType* inj;
	if(freeInjs.empty())
		inj = newInj();
	if(inj == nullptr)
		return nullptr;
	else {
		inj = freeInjs.front();
		freeInjs.pop_front();
	}
	CcEmbMap<expressions::Auxiliar,FL_TYPE,Node> cc_map(inj->getEmbedding());
	state.setAuxMap(&cc_map);
	if(inj->reuse(state,ptrn,args...) == false)
		return nullptr;
	insert(inj,state);

	for(auto& s : sumList)
		s.second += inj->evalAuxExpr(state,s.first);

	return inj;
}*/

template <typename InjType>
InjType* InjRandContainer<InjType>::selectInj(const state::State& state) {
	InjType* inj;
	if(freeInjs.empty()){
		inj = newInj();
		if(inj == nullptr)
			return nullptr;
	}
	else {
		inj = freeInjs.front();
		freeInjs.pop_front();
	}
	state.setAuxMap(*inj->getEmbedding());
	return inj;
}

template MixInjection* InjRandContainer<MixInjection>::selectInj(const state::State& state);

template <typename InjType>
InjType* InjRandContainer<InjType>::emplace(Injection* base_inj,map<Node*,Node*>& mask,const state::State& state){
	InjType* inj;
	if(freeInjs.empty()){
		inj = static_cast<InjType*>(base_inj->clone(mask));
		freeInjs.push_back(inj);
	}
	else
		inj = freeInjs.front();
	CcEmbMap<expressions::Auxiliar,FL_TYPE,Node> cc_map(*inj->getEmbedding());
	state.setAuxMap(&cc_map);

	inj->copy(static_cast<InjType*>(base_inj),mask);//TODO static_cast??
	insert(inj,state);
	freeInjs.pop_front();

	for(auto& s : sumList)
		s.second += inj->evalAuxExpr(state,s.first);
	return inj;
}

template <typename InjType>
void InjRandContainer<InjType>::selectRule(int rid,small_id cc) const {/*do nothing*/};
template <typename InjType>
FL_TYPE InjRandContainer<InjType>::getM2() const {
	throw invalid_argument("calling InjRandContainer::getM2() is forbidden.");
}

template <typename InjType>
FL_TYPE InjRandContainer<InjType>::sumInternal(const expressions::BaseExpression* aux_func,
			const simulation::SimContext& state) const {
	try{
		return trackedSums.at(aux_func);
	}
	catch(out_of_range &e){
		for(auto& ts : trackedSums)
			if((*ts.first) == (*aux_func)){
				trackedSums.emplace(aux_func,ts.second);
				return ts.second;
			}
		CcEmbMap<expressions::Auxiliar,FL_TYPE,Node> cc_map;
		state.setAuxMap(&cc_map);
		sumList.emplace_back(aux_func,0.0);
		auto& sum = sumList.back().second;
		auto func = [&](const Injection* inj) -> void {
			cc_map.setEmb(*inj->getEmbedding());
			sum += aux_func->getValue(state).valueAs<FL_TYPE>();
		};
		fold(func);
		trackedSums.emplace(aux_func,sum);
		return sum;
	}
}

/********* MultiInjSet ********************/

template <typename InjType>
InjRandSet<InjType>::InjRandSet(const pattern::Pattern& _ptrn) : InjRandContainer<InjType>(_ptrn)
		,counter(0),multiCount(0) {
	container.reserve(100);
}

template <typename InjType>
InjRandSet<InjType>::~InjRandSet(){
	for(auto inj : container)
		delete inj;
}

//TODO better way to count?
template <typename InjType>
size_t InjRandSet<InjType>::count() const {
	return counter;
	/*size_t c = 0;
	for(size_t i = 0; i < multiCount; i++)
		c += container[i]->count();
	return c + container.size() - multiCount;*/
}

//TODO
template <typename InjType>
InjType& InjRandSet<InjType>::chooseRandom(RNG& randGen) const{
	auto uni = uniform_int_distribution<size_t>(0,count()-1);
	auto selection = uni(randGen);//0; deterministic
	if(selection < container.size() - multiCount)
		return *container[selection + multiCount];
	else{
		selection -= container.size() - multiCount;
		auto it = container.begin();
		while(selection > (*it)->count()){
			selection -= (*it)->count();
			it++;
		}
		return **it;
	}
}

template <typename InjType>
InjType& InjRandSet<InjType>::choose(unsigned id) const {
	if(id < container.size() - multiCount)  //The id can be one of the Node injections
			return *container[id + multiCount];
	else{	// the id has to be in the multi-node injections
		id -= container.size() - multiCount;
		auto it = container.begin();
		while(id > (*it)->count()){
			id -= (*it)->count();
			it++;
		}
		return **it;
	}	//the id is > that size
	throw out_of_range("InjRandSet::choose(): Out of bounds.");
}

template <typename InjType>
list<pair<const BE*,FL_TYPE>>& InjRandSet<InjType>::insert(InjType* inj,const state::State& state){
	inj->alloc(container.size());
	container.push_back(inj);
	if(inj->count() > 1)
		multiCount++;
	counter += inj->count();
	return this->sumList;
}

template <typename InjType>
void InjRandSet<InjType>::del(Injection* inj){
#ifdef DEBUG
	if(container.empty() || inj->getAddress() > counter )
		throw out_of_range("InjRandSet::del(): Id out of bounds. What injection are you trying to delete?");
#endif
	if(inj->getAddress() < multiCount)
		multiCount--;
	if(container.size() > 1){
		container.back()->alloc(inj->getAddress());
		container[inj->getAddress()] = container.back();
	}
	container.pop_back();
	counter -= inj->count();
}

template <typename InjType>
void InjRandSet<InjType>::clear() {
	multiCount = 0;
	counter = 0;
	for(auto inj : container){
		inj->alloc(unsigned(-1));
		this->freeInjs.push_back(inj);
	}
	container.clear();
}


template <typename InjType>
InjType* InjRandSet<InjType>::newInj() const{
	return new InjType(this->ptrn);
}


template <typename InjType>
FL_TYPE InjRandSet<InjType>::partialReactivity() const {
	return counter;
}

template <typename InjType>
void InjRandSet<InjType>::fold(const function<void (const Injection*)> func) const{
	for(auto inj : container)
		func(inj);
}




//*********** InjRandSet<MixInjection> *********************//

//template <> class InjRandSet<MixInjection>;

InjRandSet<MixInjection>::InjRandSet(const pattern::Mixture& mix,InjRandSet<MixInjection>* nxt) :
		InjRandContainer<MixInjection>(mix),next(nxt),counter(0),multiCount(0),
		radius(mix.getRadius()){
	container.reserve(100);
}

InjRandSet<MixInjection>::~InjRandSet(){
	for(auto inj : container)
		delete inj;
	if(next)
		delete next;
}


//TODO better way to count?
size_t InjRandSet<MixInjection>::count() const {
	return counter + (next? next->count() : 0);
}

//TODO
MixInjection& InjRandSet<MixInjection>::chooseRandom(RNG& randGen) const{
	auto uni = uniform_int_distribution<size_t>(0,count()-1);
	auto selection = uni(randGen);
	return choose(selection);
}

MixInjection& InjRandSet<MixInjection>::choose(unsigned id) const {
	if(id >= counter)
		return this->next->choose(id - counter);
	if(id < container.size() - multiCount)
			return *container[id + multiCount];
	else{
		id -= container.size() - multiCount;
		auto it = container.begin();
		while(id > (*it)->count()){
			id -= (*it)->count();
			it++;
		}
		return **it;
	}
}

list<pair<const BE*,FL_TYPE>>& InjRandSet<MixInjection>::insert(MixInjection* inj,const state::State& state){
	if(inj->getDistance() > radius)
		return next->insert(inj,state);
	inj->alloc(container.size());
	container.push_back(inj);
	counter += inj->count();
	if(inj->count() > 1)
		multiCount++;
	return sumList;
}

void InjRandSet<MixInjection>::del(Injection* inj){
#ifdef DEBUG
	if(container.empty())
		throw out_of_range("InjRandSet is empty, what injection are you trying to delete?");
#endif
	if(inj->getAddress() < multiCount)
		multiCount--;
	if(container.size() > 1){
		container.back()->alloc(inj->getAddress());
		container[inj->getAddress()] = container.back();
	}
	container.pop_back();
	counter -= inj->count();
}

void InjRandSet<MixInjection>::clear() {
	multiCount = 0;
	counter = 0;
	for(auto inj : container){
		inj->alloc(unsigned(-1));
		InjRandContainer<MixInjection>::freeInjs.push_back(inj);
	}
	container.clear();
}

MixInjection* InjRandSet<MixInjection>::newInj() const{
	return new MixInjection(static_cast<const pattern::Mixture&>(InjRandContainer<MixInjection>::ptrn));
}


FL_TYPE InjRandSet<MixInjection>::partialReactivity() const {
	return counter;
}

/*FL_TYPE InjRandSet::sumInternal(const expressions::BaseExpression* aux_func,
			const expressions::EvalArgs& _args) const {
	expressions::AuxCcEmb aux_values;
	expressions::EvalArgs args(&_args.getState(),&_args.getVars(),&aux_values);
	auto func = [&](const CcInjection* inj) -> FL_TYPE {
		aux_values.setEmb(inj->getEmbedding());
		return aux_func->getValue(args).valueAs<FL_TYPE>();
	};
	FL_TYPE sum = 0;
	for(auto inj : container)
		sum += func(inj);
	return sum;
}*/

void InjRandSet<MixInjection>::fold(const function<void (const Injection*)> func) const{
	for(auto inj : container)
		func(inj);
	if(next)
		next->fold(func);
}




/************ InjRandTree *******************/

template <typename InjType>
const float InjRandTree<InjType>::MAX_INVALIDATIONS = 0.03;//3%

template <typename InjType>
InjRandTree<InjType>::Root::Root(const expressions::BaseExpression* rate) :
		tree(new distribution_tree::Node<CcValueInj>(1.0)),
		partial_rate(rate),second_moment(-1.,0.),is_valid(false) {}

template <typename InjType>
InjRandTree<InjType>::InjRandTree(const pattern::Pattern::Component& _cc,const vector<simulation::Rule::Rate*>& rates) :
		InjRandContainer<InjType>(_cc),counter(0),invalidations(0),selected_root(0,0){
	auto& deps = _cc.getRateDeps();
	//distribution_tree::Node<CcValueInj>* pointer = nullptr;
	for(auto r_cc : deps){//
		auto r_id = r_cc.first.getId();
		auto rid_cc = make_pair(r_id,r_cc.second);
		if(r_cc.first.getRate().getVarDeps() & expressions::BaseExpression::AUX){
			auto faux_ridcc = rates[r_id]->getExpression(r_cc.second);
			for(auto& root : roots)//dont create another root if there is one assoc. with the same aux.func.
				if(*faux_ridcc == *(rates[root.first.first]->getExpression(root.first.second)) ){
					roots[rid_cc] = root.second;
					break;
				}
			if(!roots.count(rid_cc)){
				roots[rid_cc] = new Root(faux_ridcc);
				roots_to_push[rid_cc] = roots[rid_cc];
			}
		}
		//else
		//	roots[r_cc.first.getId(),r_cc.second] = nullptr;//no need to make another tree for this root
	}

}

template <typename InjType>
InjRandTree<InjType>::~InjRandTree(){
	for(auto inj : infList)
		delete inj;
	//roots.begin()->second->tree->deleteContent();//delete only injs inside tree
	set<Root*> deleted;
	for(auto root : roots_to_push){
		root.second->tree->deleteContent();//delete only injs inside tree if(root.second && !deleted.count(root.second)){
		delete root.second;
	}
}

/*template <>
void InjRandTree<CcValueInj>::insert(CcValueInj* inj,const state::State& state) {
	//auto ccval_inj = static_cast<CcValueInj*>(inj);
	for(auto& ridcc_tree : roots_to_push){// for each (rid,cc_index) that is using this cc-pattern
		auto& r = state.getEnv().getRules()[ridcc_tree.first.first];
		FL_TYPE val = 1.0;
		auto ccaux_func = state.getRuleRate(r.getId()).getExpression(ridcc_tree.first.second);
		val *= ccaux_func->getValue(state).
				valueAs<FL_TYPE>();//TODO at this point injection is already set in state?
		if(val < 0.)
			throw invalid_argument("When applying rule '"+r.getName()+
					"':\nPartial reactivities cannot be negative.");
		ridcc_tree.second->tree->push(inj,val);//TODO static cast?
	}
	counter += inj->count();
	invalidations++;
}
*/

template <typename InjType>
list<pair<const BE*,FL_TYPE>>& InjRandTree<InjType>::insert(InjType* _inj,const state::State& state) {
	auto inj = dynamic_cast<CcValueInj*>(_inj);
	for(auto& ridcc_tree : roots_to_push){// for each (rid,cc_index) that is using this cc-pattern
		auto& r = state.getEnv().getRule(ridcc_tree.first.first);
		FL_TYPE val = 1.0;
		auto ccaux_func = state.getRuleRate(r.getId()).getExpression(ridcc_tree.first.second);
		val *= ccaux_func->getValue(state).template valueAs<FL_TYPE>();//TODO at this point injection is already set in state?
		if(val < 0.)
			throw invalid_argument("When applying rule '"+r.getName()+
					"':\nPartial reactivities cannot be negative.");
		ridcc_tree.second->tree->push(inj,val);//TODO static cast?
	}
	counter += inj->count();
	invalidations++;
	return this->sumList;
}


template <typename InjType>
InjType& InjRandTree<InjType>::chooseRandom(RNG& randGen) const {
	distribution_tree::DistributionTree<CcValueInj> *tree = nullptr;
	if(roots.count(selected_root)){
		tree = this->roots.at(selected_root)->tree;
	}

	if(tree){
		auto uni = uniform_real_distribution<FL_TYPE>(0,partialReactivity());
		auto selection = uni(randGen);
		return tree->choose(selection);
	}
	else{
		auto uni = uniform_int_distribution<unsigned int>(0,count()-1);
		auto selection = uni(randGen);
		return *this->roots.begin()->second->tree->choose(selection).first;
	}
	//throw std::out_of_range("Selected injection out of range. [InjRandTree::chooseRandom()]");
}

template <typename InjType>
InjType& InjRandTree<InjType>::choose(unsigned id) const {
	return *(this->roots.begin()->second->tree->choose(id).first);
}

template <typename InjType>
void InjRandTree<InjType>::del(Injection* inj){
	auto val_inj = static_cast<CcValueInj*>(inj);
	val_inj->selfRemove();
	counter -= inj->count();
	invalidations++;
}


template <typename InjType>
void InjRandTree<InjType>::clear() {
	counter = 0;
	invalidations = 0;
	list<CcValueInj*> frees;
	for(auto root : roots_to_push)
		root.second->tree->clear(&frees);
	this->freeInjs.insert(this->freeInjs.end(),frees.begin(),frees.end());
}

template <typename InjType>
InjType* InjRandTree<InjType>::newInj() const{
	return new CcValueInj(static_cast<const pattern::Pattern::Component&>(this->ptrn));
}

template <typename InjType>
size_t InjRandTree<InjType>::count() const {
	return this->roots.begin()->second->tree->count();
}


template <typename InjType>
FL_TYPE InjRandTree<InjType>::partialReactivity() const {
	if(roots.count(selected_root)){
		return this->roots.at(selected_root)->tree->total();
	}
	else
		return this->roots.begin()->second->tree->total();
}

//return an approximation of the sum(elem_i*elem_i)
template <typename InjType>
FL_TYPE InjRandTree<InjType>::getM2() const {
	if(count() == 0)
		return 0;
	auto root = roots.at(selected_root);
	if(invalidations > MAX_INVALIDATIONS*count()){
		for(auto r : roots)
			r.second->is_valid = false;
		invalidations = 0;
	}
	if(!root->is_valid){
		root->second_moment = make_pair(root->tree->squares(),root->tree->total());
		root->is_valid = true;
	}
	auto val = sqrt(root->second_moment.first)*root->tree->total()/root->second_moment.second;
	return val*val;
}


template <typename InjType>
void InjRandTree<InjType>::selectRule(int rid,small_id cc) const {
	selected_root.first = rid;
	selected_root.second = cc;
};


template <typename InjType>
FL_TYPE InjRandTree<InjType>::sumInternal(const expressions::BaseExpression* aux_func,
		const simulation::SimContext& context) const {
	for(auto& ridcc_tree : roots){
		if((*aux_func) == (*ridcc_tree.second->partial_rate))
			return ridcc_tree.second->tree->total();
	}
	return InjRandContainer<InjType>::sumInternal(aux_func,context);
}

template <typename InjType>
void InjRandTree<InjType>::fold(const function<void (const Injection*)> func) const{
	this->roots.begin()->second->tree->fold(func);
}





/***************** InjLazyContainer *********************/



template <typename InjType>
InjLazyContainer<InjType>::InjLazyContainer(const pattern::Pattern::Component& _cc,const state::State& _state,
			const state::SiteGraph& _graph,InjRandContainer<InjType> *&_holder) :
					InjRandContainer<InjType>(_cc),state(_state),graph(_graph),holder(&_holder),loaded(false) {}

template <typename InjType>
InjLazyContainer<InjType>::~InjLazyContainer(){
	if(!loaded){
		ADD_WARN_NOLOC("The connected component ID("+to_string(this->ptrn.getId())+") was never used.");
	}
}

template <typename InjType>
void InjLazyContainer<InjType>::loadContainer() const {
	//cout << "loading container" << endl;
	(*holder) = new InjRandSet<InjType>(this->ptrn);
	for(auto& node_p : graph){
		map<int,InjSet*> port_list;
		auto inj_p = (*holder)->emplace(state,node_p,port_list);
		if(inj_p){
			//if(port_list.empty())
			node_p->addDep(inj_p);
			//else
			for(auto port : port_list)
				port.second->emplace(inj_p);
		}
	}
	loaded = true;
	//cout << "cc " << cc.getId() << " loaded!" << endl;
}

template <typename InjType>
void InjLazyContainer<InjType>::loadEmptyContainer() const {
	//cout << "loading empty container" << endl;
	(*holder) = new InjRandSet<InjType>(this->ptrn);
	loaded = true;
}

template <typename InjType>
InjType& InjLazyContainer<InjType>::chooseRandom(RNG& randGen) const {
	loadContainer();
	auto& re = (*holder)->chooseRandom(randGen);
	delete this;
	return re;
}
template <typename InjType>
InjType& InjLazyContainer<InjType>::choose(unsigned id) const {
	loadContainer();
	auto& re = (*holder)->choose(id);
	delete this;
	return re;
}
template <typename InjType>
size_t InjLazyContainer<InjType>::count() const {
	loadContainer();
	auto re = (*holder)->count();
	delete this;
	return re;
}
template <typename InjType>
FL_TYPE InjLazyContainer<InjType>::partialReactivity() const {
	if(!loaded)
		loadContainer();
	auto re = (*holder)->partialReactivity();
	delete this;
	return re;
}

/*template <typename InjType>
InjType* InjLazyContainer<InjType>::emplace(Node& node,map<int,InjSet*> &port_lists,
		const state::State& state,small_id root) {
	return nullptr;
}
template <typename InjType>
InjType* InjLazyContainer<InjType>::emplace(Injection* base_inj,map<Node*,Node*>& mask,
		const state::State& state) {
	return nullptr;
}*/
template <typename InjType>
void InjLazyContainer<InjType>::erase(Injection* inj,const simulation::SimContext& context) {}
template <typename InjType>
void InjLazyContainer<InjType>::del(Injection* inj) {}
template <typename InjType>
void InjLazyContainer<InjType>::clear() {}

template <typename InjType>
void InjLazyContainer<InjType>::selectRule(int rid,small_id cc) const {
	loadContainer();
	(*holder)->selectRule(rid,cc);
}
template <typename InjType>
FL_TYPE InjLazyContainer<InjType>::getM2() const {
	loadContainer();
	auto re = (*holder)->getM2();
	delete this;
	return re;
}

template <typename InjType>
FL_TYPE InjLazyContainer<InjType>::sumInternal(const expressions::BaseExpression* aux_func,
		const simulation::SimContext& context) const{
	loadContainer();
	auto re = (*holder)->sumInternal(aux_func,state);
	//cout << "lazy sum internal = " << re << endl;
	delete this;
	return re;
}

template <typename InjType>
void InjLazyContainer<InjType>::fold(const function<void (const Injection*)> func) const {
	loadContainer();
	(*holder)->fold(func);
	delete this;
}

template <typename InjType>
list<pair<const BE*,FL_TYPE>>& InjLazyContainer<InjType>::insert(InjType* inj,const state::State& state) {
	if(!loaded)
		throw invalid_argument("InjLazyContainer::insert(): invalid call.");
	auto hold = (*holder);
	hold->insert(inj,state);
	delete this;
	return hold->sumList;
}
template <typename InjType>
InjType* InjLazyContainer<InjType>::newInj() const {
	loadEmptyContainer();
	auto re = (*holder)->newInj();
	//delete this;//TODO delete this will invalidate [this] in emplace
	return re;
}

template class InjLazyContainer<CcInjection>;
template class InjRandContainer<CcInjection>;
template class InjRandTree<CcInjection>;
//template class InjRandTree<CcValueInj>;
}

