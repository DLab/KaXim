/*
 * Node.cpp
 *
 *  Created on: Oct 18, 2017
 *      Author: naxo
 */

#include <cstring>
#include "Node.h"
#include "SiteGraph.h"
#include "../matching/Injection.h"
#include "../matching/InjRandSet.h"
#include "../pattern/Environment.h"
#include "../pattern/mixture/Agent.h"
#include "State.h"
#include "../simulation/Parameters.h"


namespace state {

/********* Node Class ************/

void (Node::*Node::applyAction[6])(State&) ={
		[pattern::ActionType::CHANGE] = &Node::changeIntState,
		[pattern::ActionType::ASSIGN] = &Node::assign,
		[pattern::ActionType::LINK] = &Node::bind,
		[pattern::ActionType::UNBIND] = &Node::unbind,
		[pattern::ActionType::CREATE] = nullptr,
		[pattern::ActionType::DELETE] = &Node::removeFrom
};

Node::Node() : signId(-1),address(-1),
		intfSize(0),deps(),interface(nullptr){};

Node::Node(const pattern::Signature& sign) : signId(sign.getId()),address(-1),
		intfSize(sign.getSiteCount()),deps(new InjSet()){
	//deps->reserve(10);
	interface = new InternalState*[intfSize];
	for(int i = 0; i < intfSize; i++){
		auto& site = sign.getSite(i);
		interface[i] = InternalState::createState(site.getType());
		interface[i]->setValue(site.getDefaultValue());
	}
}

/*Node::Node(const Node& node) : signId(),address(-1),
		intfSize(0),deps(nullptr),interface(nullptr){//useless
	throw invalid_argument("do not call basic copy constructor of node.");
}*/

/*Node& Node::operator=(const Node& node){
	throw invalid_argument("do not call operator=() of node.");
}*/

Node::Node(const Node& node,const map<Node*,Node*>& mask) :
		signId(node.signId),address(-1),intfSize(node.intfSize),deps(new InjSet()){
	interface = new InternalState*[intfSize];
	for(small_id i = 0; i < intfSize; i++){
		interface[i] = node.interface[i]->clone(mask);
	}
}

Node::~Node() {
	//cout << "destroying node " << this << endl;
	for(int i = 0; i < intfSize; i++)
		delete interface[i];
	delete[] interface;
	delete deps;
}

void Node::copyDeps(const Node& node,EventInfo& ev,matching::InjRandContainer** injs,
		const State& state) {
	for(small_id i = 0; i < intfSize; i++){
		auto& site_deps = node.interface[i]->getDeps();
		for(auto inj : *site_deps.first){
			matching::Injection* new_inj;
			if(ev.inj_mask.count(inj))
				new_inj = ev.inj_mask.at(inj);
			else{
				new_inj = injs[inj->pattern().getId()]->emplace(inj,ev.new_cc,state);
				ev.inj_mask[inj] = new_inj;
			}
			site_deps.first->emplace(new_inj);
			ev.to_update.emplace(&new_inj->pattern());
		}
		for(auto inj : *site_deps.second){
			matching::Injection* new_inj;
			if(ev.inj_mask.count(inj))
				new_inj = ev.inj_mask.at(inj);
			else{
				new_inj = injs[inj->pattern().getId()]->emplace(inj,ev.new_cc,state);
				ev.inj_mask[inj] = new_inj;
			}
			site_deps.second->emplace(new_inj);
			ev.to_update.emplace(&new_inj->pattern());
		}
	}
	for(auto inj : *node.deps){
		matching::Injection* new_inj;
		if(ev.inj_mask.count(inj))
			new_inj = ev.inj_mask.at(inj);
		else{
			new_inj = injs[inj->pattern().getId()]->emplace(inj,ev.new_cc,state);
			ev.inj_mask[inj] = new_inj;
		}
		deps->emplace(new_inj);
		ev.to_update.emplace(&new_inj->pattern());
	}
}

void Node::alloc(big_id addr){
	address = addr;
}

big_id Node::getAddress() const {
	return address;
}

void Node::setCount(pop_size q){
	throw invalid_argument("Cannot set population of basic Node");
}

pop_size Node::getCount() const {
	return 1;
}

/*short_id Node::getId() const{ //inlined
	return signId;
}*/


/*template <> void Node::setState(small_id site_id,small_id value){
	interface[site_id].val.smallVal = value;
}
template <> void Node::setState(small_id site_id,float value){
	interface[site_id].val.fVal = value;
}
template <> void Node::setState(small_id site_id,int value){
	interface[site_id].val.iVal = value;
}*/



void Node::removeFrom(State& state) {
	state.removeNode(this);
	for(small_id i = 0; i < intfSize; i++){
		interface[i]->negativeUpdate(state.ev,state.injections,state);
	}
	InternalState::negativeUpdate(state.ev,state.injections,deps,state);
	state.ev.side_effects.erase(this);
#ifdef DEBUG
	if(simulation::Parameters::get().verbose > 2){
		auto& ag_sign = state.getEnv().getSignature(signId);
		cout << "[remove] " << ag_sign.getName() << "()\n";
	}
#endif
	delete this;//TODO do not delete, reuse nodes
}

void Node::changeIntState(State& state){
	auto old_val = interface[state.ev.act.trgt_st]->getValue().smallVal;
	if(old_val == state.ev.act.new_label){
		state.ev.null_actions.emplace(this,state.ev.act.trgt_st);
#ifdef DEBUG
	if(simulation::Parameters::get().verbose > 2)
		cout << "[null-action] ";
#endif
	} else {
		interface[state.ev.act.trgt_st]->setValue(state.ev.act.new_label);
		interface[state.ev.act.trgt_st]->negativeUpdateByValue(state.ev,state.injections,state);
	}
#ifdef DEBUG
	if(simulation::Parameters::get().verbose > 2){
		auto& agent = state.getEnv().getSignature(signId);
		auto& site_sign = dynamic_cast<const pattern::Signature::LabelSite&>(
				agent.getSite(state.ev.act.trgt_st));
		cout << "[change] " << agent.getName() << "(" << site_sign.getName()
				<< "~{ " << site_sign.getLabel(old_val)
				<< " -> " << site_sign.getLabel(state.ev.act.new_label) << " })\n";
	}
#endif
}

void Node::assign(State& state){
	auto val = state.ev.act.new_value->getValue(state);
	auto old_val = interface[state.ev.act.trgt_st]->getValue();
	if(old_val == val){
		state.ev.null_actions.emplace(this,state.ev.act.trgt_st);
#ifdef DEBUG
		if(simulation::Parameters::get().verbose > 2)
			cout << "[null-action] ";
#endif
	} else {
		interface[state.ev.act.trgt_st]->setValue(val);
		interface[state.ev.act.trgt_st]->negativeUpdateByValue(state.ev,state.injections,state);
	}
#ifdef DEBUG
	if(simulation::Parameters::get().verbose > 2){
		auto& agent = state.getEnv().getSignature(signId);
		auto& site_sign = agent.getSite(state.ev.act.trgt_st);
		cout << "[assign] " << agent.getName() << "(" << site_sign.getName()
				<< "~{ " << old_val	<< " -> " << val.valueAs<FL_TYPE>() << " })\n";
	}
#endif
}

void Node::unbind(State& state){
	auto lnk = interface[state.ev.act.trgt_st]->getLink();
	if(lnk.first){
		if(state.ev.act.side_eff)//when unbinding s!_ or s? pattern
			state.ev.side_effects.emplace(lnk);
		interface[state.ev.act.trgt_st]->negativeUpdateByBind(state.ev,state.injections,state);
		//if(state.ev.act.chain){//unbinding s!1 pattern
			lnk.first->interface[lnk.second]->negativeUpdateByBind(state.ev,state.injections,state);
			lnk.first->setInternalLink(lnk.second,nullptr,0);
		//}
		interface[state.ev.act.trgt_st]->setLink(nullptr,0);
#ifdef DEBUG
		if(simulation::Parameters::get().verbose > 2){
			auto& agent = state.getEnv().getSignature(signId);
			auto& site_sign = agent.getSite(state.ev.act.trgt_st);
			auto& trgt_ag = state.getEnv().getSignature(lnk.first->getId());
			cout << "[unbind] " << agent.getName() << "." << site_sign.getName()
					<< "!" << trgt_ag.getSite(lnk.second).getName() << "."
					<< trgt_ag.getName() << "\n";
		}
#endif
	}
	else{//unbinding s? pattern
		state.ev.null_actions.emplace(this,-int(state.ev.act.trgt_st)-1);
#ifdef DEBUG
		if(simulation::Parameters::get().verbose > 2)
			cout << "[unbind] null-action\n";
#endif
	}
}
void Node::bind(State& state){
	auto chain = state.ev.act.chain;
	auto trgt_node = state.ev.emb[chain->trgt_ag.first][chain->trgt_ag.second];
	auto trgt_site = chain->trgt_st;
	auto lnk = interface[state.ev.act.trgt_st]->getLink();
	if(lnk.first){
		if(state.ev.act.side_eff){//binding BND_PTRN/BND_ANY/WILD
			if(lnk.first == trgt_node && lnk.second == trgt_site){//WILD/ANY and no unary rate
				state.ev.null_actions.emplace(this,-int(state.ev.act.trgt_st)-1);
				state.ev.null_actions.emplace(lnk.first,-int(trgt_site)-1);
#ifdef DEBUG
			if(simulation::Parameters::get().verbose > 2)
				cout << "[bind] null-action\n";
#endif
				return;
			}
			state.ev.side_effects.emplace(lnk);
		}
		lnk.first->interface[lnk.second]->negativeUpdateByBind(state.ev,state.injections,state);
		lnk.first->setInternalLink(lnk.second,nullptr,0);
#ifdef DEBUG
		if(simulation::Parameters::get().verbose > 2){
			auto& agent = state.getEnv().getSignature(signId);
			auto& site_sign = agent.getSite(state.ev.act.trgt_st);
			auto& trgt_ag = state.getEnv().getSignature(lnk.first->getId());
			cout << "[unbind(bind-swap)] " << agent.getName() << "." << site_sign.getName()
					<< "!" << trgt_ag.getSite(lnk.second).getName() << "."
					<< trgt_ag.getName() << "\n";
		}
#endif
	}
	interface[state.ev.act.trgt_st]->negativeUpdateByBind(state.ev,state.injections,state);
	interface[state.ev.act.trgt_st]->setLink(trgt_node,trgt_site);
	trgt_node->interface[trgt_site]->negativeUpdateByBind(state.ev,state.injections,state);
	trgt_node->interface[trgt_site]->setLink(this,state.ev.act.trgt_st);
#ifdef DEBUG
	if(simulation::Parameters::get().verbose > 2){
		auto& agent = state.getEnv().getSignature(signId);
		auto& site_sign = agent.getSite(state.ev.act.trgt_st);
		auto& trgt_ag = state.getEnv().getSignature(trgt_node->getId());
		cout << "[bind] " << agent.getName() << "." << site_sign.getName()
				<< "_" << trgt_ag.getSite(trgt_site).getName() << "."
				<< trgt_ag.getName() << "\n";
	}
#endif
}

InternalState** Node::begin(){
	return interface;
}
InternalState** Node::end(){
	return interface+intfSize;
}

/*inlined** bool Node::removeDep(matching::Injection* inj) {
	return deps->erase(inj);
}*/



/*********** MultiNode *****************/

MultiNode::MultiNode(unsigned n,pop_size pop) :
		population(pop),count(0) {
	cc = new SubNode*[n];
}
MultiNode::~MultiNode(){
	delete[] cc;
}

pop_size MultiNode::getPopulation() const {
	return population;
}

small_id MultiNode::add(SubNode* node){
	cc[count] = node;
	//count++;
	return count++;//count-1;//test
}

void MultiNode::remove() {
	count--;
	if(!count)
		delete this;
}

void MultiNode::dec(EventInfo& ev) {
	population--;//TODO its ok???
	if(population == 0){
		throw OutOfInstances();
	}
	//or here
	for(small_id i = 0; i < count; i++){
		ev.new_cc.emplace(cc[i],nullptr);
	}
}

/************ SubNode ******************/

SubNode::SubNode(const pattern::Signature& sign,MultiNode& multi) :
		Node(sign),cc(multi) {
	multiId = multi.add(this);
};

SubNode::~SubNode(){
	cc.remove();
}

pop_size SubNode::getCount() const {
	return cc.getPopulation();
}

void SubNode::removeFrom(State& state){
	if(!state.ev.new_cc.count(this))
		try { cc.dec(state.ev); } catch(MultiNode::OutOfInstances &ex){
			Node::removeFrom(state);
			state.removeNode(this);
			//ev.new_cc[this] = this;
			//graph.decPopulation();//will be decreased when calling Node::removeFrom()
			return;
		}
	if(state.ev.new_cc[this]){
		state.ev.side_effects.erase(state.ev.new_cc[this]);//erasing possible side-effects on this node.
		delete state.ev.new_cc[this];
	}
	state.ev.new_cc[this] = this;
	state.graph.decPopulation();
	for(auto i = 0; i < intfSize; i++){
		auto& deps = interface[i]->getDeps();
		for(auto dep : *deps.first)//necessary? TODO
			state.ev.inj_mask[dep] = nullptr;
		for(auto dep : *deps.second)
			state.ev.inj_mask[dep] = nullptr;
		auto lnk = interface[i]->getLink();
		if(lnk.first){
			auto& opt_lnkd_node = state.ev.new_cc[lnk.first];
			if(opt_lnkd_node == lnk.first){
				state.ev.side_effects.erase(lnk.first);//maybe unnecessary
			}
			else{
				for(auto dep : *lnk.first->getLifts(lnk.second).second)//TODO review
					state.ev.inj_mask[dep] = nullptr;
				if(opt_lnkd_node == nullptr)
					opt_lnkd_node = new Node(*lnk.first,state.ev.new_cc);
				state.ev.side_effects.emplace(opt_lnkd_node,i);
			}
			//new_lnk_node->setLink(lnk.second,nullptr,0);//set link at constructor??
		}
	}
	for(auto dep : *deps)
		state.ev.inj_mask[dep] = nullptr;
}

void SubNode::changeIntState(State& state){
	if(!state.ev.new_cc.count(this))
		try { cc.dec(state.ev); } catch(MultiNode::OutOfInstances &ex){
			Node::changeIntState(state);
			return;
		}
	auto& new_node = state.ev.new_cc.at(this);
	if(new_node == nullptr)
		new_node = new Node(*this,state.ev.new_cc);
	if(interface[state.ev.act.trgt_st]->getValue().smallVal == state.ev.act.new_label)
	{}//state.ev.warns++;//ev.null_actions.emplace(this,id);
	//else
		new_node->setInternalValue(state.ev.act.trgt_st,state.ev.act.new_label);
	for(auto dep : *interface[state.ev.act.trgt_st]->getDeps().first)
		state.ev.inj_mask[dep] = nullptr;
}

void SubNode::assign(State& state){
	if(!state.ev.new_cc.count(this))
		try { cc.dec(state.ev); } catch(MultiNode::OutOfInstances &ex){
			Node::assign(state);
			return;
		}
	auto& new_node = state.ev.new_cc.at(this);
	if(new_node == nullptr)
		new_node = new Node(*this,state.ev.new_cc);
	auto value = state.ev.act.new_value->getValue(state);
	if(interface[state.ev.act.trgt_st]->getValue() == value)
	{}//state.ev.warns++;//ev.null_actions.emplace(this,id);
	//else
		new_node->setInternalValue(state.ev.act.trgt_st,value);
	for(auto dep : *interface[state.ev.act.trgt_st]->getDeps().first)
		state.ev.inj_mask[dep] = nullptr;
}

void SubNode::unbind(State& state){
	//unbind must be inside cc
	auto lnk = this->interface[state.ev.act.trgt_st]->getLink();
	if(!lnk.first){
		//state.ev.warns++;//ev.null_actions.emplace(this,-id);
		return;//null_event
	}
	if(!state.ev.new_cc.count(this))
		try { cc.dec(state.ev); } catch(MultiNode::OutOfInstances &ex){
			Node::unbind(state);
			return;
		}
	auto& node1 = state.ev.new_cc.at(this);
	auto& node2 = state.ev.new_cc.at(lnk.first);
	if(node1 == nullptr)
		node1 = new Node(*this,state.ev.new_cc);
	if(node2 == nullptr)
		node2 = new Node(*lnk.first,state.ev.new_cc);;
	if(state.ev.act.side_eff)
		state.ev.side_effects.emplace(node2,lnk.second);
	node1->setInternalLink(state.ev.act.trgt_st,nullptr,0);
	node2->setInternalLink(lnk.second,nullptr,0);
	for(auto dep : *interface[state.ev.act.trgt_st]->getDeps().second)
		state.ev.inj_mask[dep] = nullptr;
	for(auto dep : *lnk.first->getLifts(lnk.second).second)
		state.ev.inj_mask[dep] = nullptr;
}

void SubNode::bind(State& state){
	if(!state.ev.new_cc.count(this))
		try { cc.dec(state.ev); } catch(MultiNode::OutOfInstances &ex){
			Node::bind(state);
			return;
		}
	auto& new_node = state.ev.new_cc.at(this);
	if(new_node == nullptr)
		new_node = new Node(*this,state.ev.new_cc);
	auto trgt_node = state.ev.emb[state.ev.act.chain->trgt_ag.first][state.ev.act.chain->trgt_ag.second];
	auto trgt_site = state.ev.act.chain->trgt_st;
	auto lnk = trgt_node->getInternalLink(trgt_site);
	/*for(auto it = ev.side_effects.find(this);it != ev.side_effects.end(); it++){
		ev.side_effects.emplace(new_node,it->second);
		ev.side_effects.erase(it);//TODO this may invalidate it++
	}*/
	if(lnk.first && lnk.first == this)//TODO compare with site?
		trgt_node->setInternalLink(trgt_site,new_node,lnk.second);
	try{
		auto& new_trgt_node = state.ev.new_cc.at(trgt_node);
		if(new_trgt_node){//TODO check if true
			//throw invalid_argument("link between 2 sites of same agent, this cannot happen. do not make sense");
		}
		else{
			//link with a node in the same cc // or in other multi cc
			new_trgt_node = new Node(*trgt_node,state.ev.new_cc);
		}
		new_node->Node::bind(state);
	}
	catch(std::out_of_range &e){
		new_node->Node::bind(state);
	}
	for(auto dep : *interface[state.ev.act.trgt_st]->getDeps().second)
		state.ev.inj_mask[dep] = nullptr;
	if(interface[state.ev.act.trgt_st]->getLink().first){
		for(auto dep : *interface[state.ev.act.trgt_st]->getLink().first->getLifts(trgt_site).second)
			state.ev.inj_mask[dep] = nullptr;
	}

}





/*******************************************/
/********** class InternalState ************/
/*******************************************/

InjSet* InternalState::EMPTY_INJSET = new InjSet();

void InternalState::negativeUpdate(EventInfo& ev,matching::InjRandContainer** injs,InjSet* deps,const State& state){
	auto dep_it = deps->begin(),nxt = dep_it,end = deps->end();
	while(dep_it != end){//there is still injs to delete
		auto inj = *dep_it;
		nxt = next(dep_it);
		if(inj->isTrashed())//already deleted?
			deps->erase(dep_it);
		else{//deleting the injection
			auto& emb = inj->getEmbedding();
			auto& cc = inj->pattern();
			for(unsigned i = 0; i < emb.size(); i++){//removing deps for each node pointed by injection
				if(emb[i]->removeDep(inj))
					continue;//if removed from node-deps, its not in the site-deps
				auto& mix_ag = cc.getAgent(i);
				for(auto& site : mix_ag){//removing dep from site depsets
					if(site.getLinkType() != pattern::Pattern::WILD)
						emb[i]->getLifts(site.getId()).second->erase(inj);
					if(site.getValueType() != pattern::Pattern::EMPTY)
						emb[i]->getLifts(site.getId()).first->erase(inj);
				}
			}//for i
			injs[cc.getId()]->erase(inj,state);
			ev.to_update.emplace(&cc);
		}//else
		dep_it = nxt;
	}
}

InternalState* InternalState::createState(char type){
	using ss = pattern::Signature::Site;
	if(type == ss::BIND)
		return new BindState();
	else if(type > ss::BIND)
		return new ValueBindState();
	else
		return new ValueState();
}

InternalState::InternalState(InjSet* d1,InjSet* d2) : deps(d1,d2){}

InternalState::~InternalState(){
	//cout << "deps count: " << deps.first->size() << " - " << deps.second->size() << endl;
}

void InternalState::negativeUpdateByValue(EventInfo& ev,matching::InjRandContainer** injs,const State& state){
	throw invalid_argument("calling InternalState::negativeUpdateByValue() is unexpected.");
	//negativeUpdate(ev,injs,deps.first,state);
}

void InternalState::negativeUpdateByBind(EventInfo& ev,matching::InjRandContainer** injs,const State& state){
	throw invalid_argument("calling InternalState::negativeUpdateByBind() is unexpected.");
	//negativeUpdate(ev,injs,deps.second,state);
}

void InternalState::setValue(SomeValue val){/*do nothing cause its always called on node constructor. */}
SomeValue InternalState::getValue() const{
	throw invalid_argument("Calling InternalState::getValue() is unexpected.");
}
void InternalState::setLink(Node* n,small_id i){
	throw invalid_argument("Calling InternalState::setLink() is unexpected.");
}
pair<Node*,small_id> InternalState::getLink() const{
	throw invalid_argument("Calling InternalState::getLink() is unexpected.");
}

/************ class ValueState *****************/

ValueState::ValueState() : InternalState(new InjSet(),EMPTY_INJSET) {}

ValueState::~ValueState(){
	delete deps.first;
}

InternalState* ValueState::clone(const map<Node*,Node*>& mask) const {
	auto state = new ValueState();
	state->value = value;
	return state;
}
void ValueState::setValue(SomeValue val){
#ifdef DEBUG
	if(val.t != FLOAT && val.t != INT && val.t != SMALL_ID )
		throw invalid_argument("ValueState::setValue(): no a valid value type.");
#endif
	value = val;
}
SomeValue ValueState::getValue() const{
	return value;
}

void ValueState::negativeUpdate(EventInfo& ev,matching::InjRandContainer** injs,const State& state){
	InternalState::negativeUpdate(ev,injs,deps.first,state);
}

void ValueState::negativeUpdateByValue(EventInfo& ev,matching::InjRandContainer** injs,const State& state){
	InternalState::negativeUpdate(ev,injs,deps.first,state);
}

string ValueState::toString(const pattern::Signature::Site& sit,
		bool show_binds,map<const Node*,bool> *visit) const {
	string s(sit.getName());
	if(!dynamic_cast<const pattern::Signature::EmptySite*>(&sit)){
		s += "~";
		switch(value.t){
		case FLOAT:
		case INT:
			s += value.toString();break;
		case SHORT_ID:
		case SMALL_ID:
			s += dynamic_cast<const pattern::Signature::LabelSite&>(sit)
					.getLabel(value.smallVal);
			break;
		case NONE:
			s += "AUX";break;
		default:
			s += "?";//TODO exception?
		}
	}
	return s;
}

/************ class BindState ******************/

BindState::BindState() : InternalState(EMPTY_INJSET,new InjSet()) {}

BindState::~BindState(){
	delete deps.second;
}

InternalState* BindState::clone(const map<Node*,Node*>& mask) const {
	auto st = new BindState();
	//st->id = id;
	if(target.first){
#ifdef DEBUG
		Node* lnk;
		if(!mask.count(target.first) || !(lnk = mask.at(target.first)) ||
				lnk == target.first)//if old-node is masked and new-node was created
			throw invalid_argument("ValueBindState::clone(): fix me!");
#endif
		st->target.first = mask.at(target.first);
		st->target.second = target.second;
		//lnk->interface[st->link.second]->setLink(this,id);//maybe twice TODO
	}
	return st;
}

void BindState::negativeUpdate(EventInfo& ev,matching::InjRandContainer** injs,const State& state){
	InternalState::negativeUpdate(ev,injs,deps.second,state);
	/*if(target.first){
		target.first->setInternalLink(target.second,nullptr,0);
		InternalState::negativeUpdate(ev,injs,target.first->getLifts(target.second).second,state);
		ev.side_effects.emplace(target);
	}*/
}

void BindState::negativeUpdateByBind(EventInfo& ev,matching::InjRandContainer** injs,const State& state){
	InternalState::negativeUpdate(ev,injs,deps.second,state);
}

void BindState::setLink(Node* n,small_id i){
	target.first = n;
	target.second = i;
}
pair<Node*,small_id> BindState::getLink() const{
	return target;
}


string BindState::toString(const pattern::Signature::Site& sit,
		bool show_binds,map<const Node*,bool> *visit) const {
	string s(sit.getName());
	if(target.first){
		if(show_binds){
			s += "!("+to_string(target.first->getAddress())+")";
			if(visit)
				(*visit)[target.first];
		}
		else
			s += "!_";
	}
	return s;
}

/************ class ValueBindState *************/

ValueBindState::ValueBindState() : InternalState(new InjSet(),new InjSet()){}

InternalState* ValueBindState::clone(const map<Node*,Node*>& mask) const {
	auto st = new ValueBindState();
	//st->id = id;
	st->value = value;
	if(target.first){
#ifdef DEBUG
		Node* lnk;
		if(!mask.count(target.first) || !(lnk = mask.at(target.first))
				|| lnk == target.first)//if old-node is masked and new-node was created
			throw invalid_argument("ValueBindState::clone(): fix me!");
#endif
		st->target.first = mask.at(target.first);
		st->target.second = target.second;
		//lnk->interface[st->link.second]->setLink(this,id);//maybe twice TODO
	}
	return st;
}

void ValueBindState::negativeUpdate(EventInfo& ev,matching::InjRandContainer** injs,const State& state){
	negativeUpdateByValue(ev,injs,state);
	negativeUpdateByBind(ev,injs,state);
}

string ValueBindState::toString(const pattern::Signature::Site& sit,
		bool show_binds,map<const Node*,bool> *visit) const {
	string s(sit.getName());
	if(!dynamic_cast<const pattern::Signature::EmptySite*>(&sit)){
		s += "~";
		switch(value.t){
		case FLOAT:
		case INT:
			s += value.toString();break;
		case SHORT_ID:
		case SMALL_ID:
			s += dynamic_cast<const pattern::Signature::LabelSite&>(sit)
					.getLabel(value.smallVal);
			break;
		case NONE:
			s += "AUX";break;
		default:
			s += "?";//TODO exception?
		}
	}
	if(target.first){
		if(show_binds){
			s += "!("+to_string(target.first->getAddress())+")";
			if(visit)
				(*visit)[target.first];
		}
		else
			s += "!_";
	}
	return s;
}



/*************************************************************/



string Node::toString(const pattern::Environment &env,bool show_binds,map<const Node*,bool> *visited) const {
	auto& sign = env.getSignature(signId);
	string s(sign.getName()+"(");
	for(small_id i = 0; i < intfSize; i++){
		s += interface[i]->toString(sign.getSite(i),show_binds,visited) + ",";
	}
	if(intfSize)
		s.back() = ')';
	else
		s += ")";
	if(visited){
		(*visited)[this] = true;
		for(auto nhb : *visited)
			if(!nhb.second)
				s += ", " + nhb.first->toString(env, show_binds, visited);
	}
	return s;
}

string SubNode::toString(const pattern::Environment &env,bool show_binds,map<const Node*,bool> *visit) const {
	return to_string(cc.getPopulation())+" " + Node::toString(env,show_binds,visit);
}


/********************************
 ******* EventInfo **************
 ********************************/

EventInfo::EventInfo() : emb(nullptr),cc_count(0) {//,aux_map(emb) {
	//TODO these numbers are arbitrary!!
	emb = new vector<Node*>[4];
	for(int i = 0; i < 4 ; i++)
		emb[i].resize(12);
	//aux_map.setEmb(emb);
}

EventInfo::~EventInfo(){
	if(emb){
		delete[] emb;
	}
}

void EventInfo::clear(){
	side_effects.clear();
	pert_ids.clear();
	new_cc.clear();
	inj_mask.clear();
	to_update.clear();
	rule_ids.clear();
	//aux_map.clear();
	//fresh_emb.clear();
	null_actions.clear();
	//cc_count = _cc_count;
}



} /* namespace state */
