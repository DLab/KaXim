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

InjRandContainer::InjRandContainer(const pattern::Pattern::Component& _cc) : cc(_cc){}

InjRandContainer::~InjRandContainer(){
	for(auto inj_it = freeInjs.begin(); inj_it != freeInjs.end(); inj_it++){
#ifdef DEBUG
		for(auto inj2_it = next(inj_it); inj2_it != freeInjs.end(); inj2_it++)
			if(*inj_it == *inj2_it)
				throw invalid_argument("inj is repeated in freeinjs!");//TODO do not throw on destructors
#endif
		delete *inj_it;
	}
}

Injection* InjRandContainer::emplace(Node& node,
		map<int,InjSet*> &port_list,const expressions::EvalArgs& _args,small_id root) {
	CcInjection* inj;
	if(freeInjs.empty()){
		inj = newInj();
		freeInjs.push_back(inj);
	}
	else {
		inj = freeInjs.front();
	}
	expressions::AuxCcEmb aux_map(inj->getEmbedding());
	expressions::EvalArgs args(&_args.getState(),&_args.getVars(),&aux_map);
	if(inj->reuse(cc,node,port_list,args,root) == false)
		return nullptr;
	insert(inj,args);
	freeInjs.pop_front();

	for(auto& s : sumList)
		s.second += inj->evalAuxExpr(args,s.first);

	return inj;
}

Injection* InjRandContainer::emplace(Injection* base_inj,map<Node*,Node*>& mask,const expressions::EvalArgs& _args){
	CcInjection* inj;
	if(freeInjs.empty()){
		inj = static_cast<CcInjection*>(base_inj->clone(mask));
		freeInjs.push_back(inj);
	}
	else
		inj = freeInjs.front();
	expressions::AuxCcEmb aux_map(inj->getEmbedding());
	expressions::EvalArgs args(&_args.getState(),&_args.getVars(),&aux_map);

	inj->copy(static_cast<CcInjection*>(base_inj),mask);//TODO static_cast??
	insert(inj,args);
	freeInjs.pop_front();

	for(auto& s : sumList)
		s.second += inj->evalAuxExpr(args,s.first);
	return inj;
}

void InjRandContainer::selectRule(int rid,small_id cc) const {/*do nothing*/};
FL_TYPE InjRandContainer::getM2() const {
	throw invalid_argument("calling InjRandContainer::getM2() is forbidden.");
}

FL_TYPE InjRandContainer::sumInternal(const expressions::BaseExpression* aux_func,
			const expressions::EvalArgs& _args) const {
	try{
		return trackedSums.at(aux_func);
	}
	catch(out_of_range &e){
		for(auto& ts : trackedSums)
			if((*ts.first) == (*aux_func)){
				trackedSums.emplace(aux_func,ts.second);
				return ts.second;
			}
		expressions::AuxCcEmb aux_values;
		sumList.emplace_back(aux_func,0.0);
		auto& sum = sumList.back().second;
		expressions::EvalArgs args(&_args.getState(),&_args.getVars(),&aux_values);
		auto func = [&](const Injection* inj) -> void {
			aux_values.setEmb(inj->getEmbedding());
			sum += aux_func->getValue(args).valueAs<FL_TYPE>();
		};
		fold(func);
		trackedSums.emplace(aux_func,sum);
		return sum;
	}
}

/********* MultiInjSet ********************/

InjRandSet::InjRandSet(const pattern::Pattern::Component& _cc) : InjRandContainer(_cc)
		,counter(0),multiCount(0) {
	container.reserve(100);
}

InjRandSet::~InjRandSet(){
	for(auto inj : container)
		delete inj;
}

//TODO better way to count?
size_t InjRandSet::count() const {
	return counter;
	/*size_t c = 0;
	for(size_t i = 0; i < multiCount; i++)
		c += container[i]->count();
	return c + container.size() - multiCount;*/
}

//TODO
const Injection& InjRandSet::chooseRandom(RNG& randGen) const{
	auto uni = uniform_int_distribution<size_t>(0,count()-1);
	auto selection = uni(randGen);
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

const Injection& InjRandSet::choose(unsigned id) const {
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

void InjRandSet::insert(CcInjection* inj,const expressions::EvalArgs& _args){
	inj->alloc(container.size());
	container.push_back(inj);
	if(inj->count() > 1){
		counter += inj->count();
		multiCount++;
	}
	else
		counter++;
}

void InjRandSet::del(Injection* inj){
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

void InjRandSet::clear() {
	multiCount = 0;
	counter = 0;
	for(auto inj : container){
		inj->alloc(unsigned(-1));
		freeInjs.push_back(inj);
	}
	container.clear();
}


CcInjection* InjRandSet::newInj() const{
	return new CcInjection(cc);
}


FL_TYPE InjRandSet::partialReactivity() const {
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

void InjRandSet::fold(const function<void (const Injection*)> func) const{
	for(auto inj : container)
		func(inj);
}



/************ InjRandTree *******************/

const float InjRandTree::MAX_INVALIDATIONS = 0.03;//3%

InjRandTree::Root::Root() : tree(new distribution_tree::Node<CcValueInj>(1.0)),
		second_moment(-1.,0.),is_valid(false) {}

InjRandTree::InjRandTree(const pattern::Pattern::Component& _cc,const vector<simulation::Rule::Rate*>& rates) :
		InjRandContainer(_cc),counter(0),invalidations(0),selected_root(0,0){
	auto& deps = cc.getRateDeps();
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
				roots[rid_cc] = new Root();
				roots_to_push[rid_cc] = roots[rid_cc];
			}
		}
		//else
		//	roots[r_cc.first.getId(),r_cc.second] = nullptr;//no need to make another tree for this root
	}

}

InjRandTree::~InjRandTree(){
	for(auto inj : infList)
		delete inj;
	//roots.begin()->second->tree->deleteContent();//delete only injs inside tree
	set<Root*> deleted;
	for(auto root : roots_to_push){
		root.second->tree->deleteContent();//delete only injs inside treeif(root.second && !deleted.count(root.second)){
		delete root.second;
	}
}

void InjRandTree::insert(CcInjection* inj,const expressions::EvalArgs& args) {
	auto ccval_inj = static_cast<CcValueInj*>(inj);
	for(auto& ridcc_tree : roots_to_push){// for each (rid,cc_index) that is using this cc-pattern
		auto& r = args.getState().getEnv().getRules()[ridcc_tree.first.first];
		FL_TYPE val = 1.0;
		auto ccaux_func = args.getState().getRuleRate(r.getId()).getExpression(ridcc_tree.first.second);
		val *= ccaux_func->getValue(args).valueAs<FL_TYPE>();
		if(val < 0.)
			throw invalid_argument("When applying rule '"+r.getName()+
					"':\nPartial reactivities cannot be negative.");
		ridcc_tree.second->tree->push(ccval_inj,val);//TODO static cast?
	}
	counter += inj->count();
	invalidations++;
}


const Injection& InjRandTree::chooseRandom(RNG& randGen) const {
	distribution_tree::DistributionTree<CcValueInj> *tree = nullptr;
	if(roots.count(selected_root)){
		tree = roots.at(selected_root)->tree;
	}

	if(tree){
		auto uni = uniform_real_distribution<FL_TYPE>(0,partialReactivity());
		auto selection = uni(randGen);
		return tree->choose(selection);
		/*for(auto& cc_node : roots.at(selected_root.first)){
		if(selection > cc_node.second->total())
			selection -= cc_node.second->total();
		else
			return cc_node.second->choose(selection);*/
	}
	else{
		auto uni = uniform_int_distribution<unsigned int>(0,count());
		auto selection = uni(randGen);
		return *roots.begin()->second->tree->choose(selection).first;
	}
	//throw std::out_of_range("Selected injection out of range. [InjRandTree::chooseRandom()]");
}

const Injection& InjRandTree::choose(unsigned id) const {
	return *(roots.begin()->second->tree->choose(id).first);
}

void InjRandTree::del(Injection* inj){
	auto val_inj = static_cast<CcValueInj*>(inj);
	val_inj->selfRemove();
	counter -= inj->count();
	invalidations++;
}


void InjRandTree::clear() {
	counter = 0;
	invalidations = 0;
	list<CcValueInj*> frees;
	for(auto root : roots_to_push)
		root.second->tree->clear(&frees);
	freeInjs.insert(freeInjs.end(),frees.begin(),frees.end());
}

CcInjection* InjRandTree::newInj() const{
	return new CcValueInj(cc);
}

size_t InjRandTree::count() const {
	return roots.begin()->second->tree->count();
}


FL_TYPE InjRandTree::partialReactivity() const {
	if(roots.count(selected_root)){
		return roots.at(selected_root)->tree->total();
	}
	else
		return roots.begin()->second->tree->total();
}

//return an approximation of the sum(elem_i*elem_i)
FL_TYPE InjRandTree::getM2() const {
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


void InjRandTree::selectRule(int rid,small_id cc) const {
	selected_root.first = rid;
	selected_root.second = cc;
};


FL_TYPE InjRandTree::sumInternal(const expressions::BaseExpression* aux_func,
		const expressions::EvalArgs& _args) const {
	for(auto& ridcc_tree : roots){
		auto ccaux_func = _args.getState().getRuleRate(ridcc_tree.first.first).getExpression(ridcc_tree.first.second);
		if((*aux_func) == (*ccaux_func))
			return ridcc_tree.second->tree->total();
	}
	return InjRandContainer::sumInternal(aux_func,_args);
}

void InjRandTree::fold(const function<void (const Injection*)> func) const{
	roots.begin()->second->tree->fold(func);
}





/***************** InjLazyContainer *********************/



InjLazyContainer::InjLazyContainer(const pattern::Pattern::Component& _cc,const state::State& _state,
			const state::SiteGraph& _graph,InjRandContainer *&_holder) :
					InjRandContainer(_cc),state(_state),graph(_graph),holder(&_holder),loaded(false) {}

InjLazyContainer::~InjLazyContainer(){
	if(!loaded){
		ADD_WARN_NOLOC("The connected component ID("+to_string(cc.getId())+") was never used.");
	}
}

void InjLazyContainer::loadContainer() const {
	(*holder) = new InjRandSet(cc);
	for(auto& node_p : graph){
		map<int,InjSet*> port_list;
		auto inj_p = (*holder)->emplace(*node_p,port_list,expressions::EvalArgs(&state,&state.getVars()));
		if(inj_p){
			if(port_list.empty())
				node_p->addDep(inj_p);
			else
				for(auto port : port_list)
					port.second->emplace(inj_p);
		}
	}
	loaded = true;
	//cout << "cc " << cc.getId() << " loaded!" << endl;
}

const Injection& InjLazyContainer::chooseRandom(RNG& randGen) const {
	loadContainer();
	auto& re = (*holder)->chooseRandom(randGen);
	delete this;
	return re;
}
const Injection& InjLazyContainer::choose(unsigned id) const {
	loadContainer();
	auto& re = (*holder)->choose(id);
	delete this;
	return re;
}
size_t InjLazyContainer::count() const {
	loadContainer();
	auto re = (*holder)->count();
	delete this;
	return re;
}
FL_TYPE InjLazyContainer::partialReactivity() const {
	loadContainer();
	auto re = (*holder)->partialReactivity();
	delete this;
	return re;
}

Injection* InjLazyContainer::emplace(Node& node,map<int,InjSet*> &port_lists,
		const expressions::EvalArgs& _args,small_id root) {
	return nullptr;
}
Injection* InjLazyContainer::emplace(Injection* base_inj,map<Node*,Node*>& mask,
		const expressions::EvalArgs& _args) {
	return nullptr;
}
void InjLazyContainer::erase(Injection* inj,const expressions::EvalArgs& _args) {}
void InjLazyContainer::del(Injection* inj) {}
void InjLazyContainer::clear() {}

void InjLazyContainer::selectRule(int rid,small_id cc) const {}
FL_TYPE InjLazyContainer::getM2() const {
	loadContainer();
	auto re = (*holder)->getM2();
	delete this;
	return re;
}

FL_TYPE InjLazyContainer::sumInternal(const expressions::BaseExpression* aux_func,
		const expressions::EvalArgs& _args) const{
	loadContainer();
	auto re = (*holder)->sumInternal(aux_func,_args);
	delete this;
	return re;
}

void InjLazyContainer::fold(const function<void (const Injection*)> func) const {
	loadContainer();
	(*holder)->fold(func);
	delete this;
}

void InjLazyContainer::insert(CcInjection* inj,const expressions::EvalArgs& _args) {}
CcInjection* InjLazyContainer::newInj() const {return nullptr;}



}

