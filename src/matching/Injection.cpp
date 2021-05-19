/*
 * Injection.cpp
 *
 *  Created on: Apr 11, 2017
 *      Author: naxo
 */

#include <queue>
#include <iostream>
#include "Injection.h"
#include "../state/State.h"
#include "../state/Node.h"
#include "../expressions/Vars.h"
#include "../pattern/mixture/Component.h"
#include "../pattern/mixture/Agent.h"

namespace matching {


/********** CcInjection **********/

CcInjection::CcInjection(const pattern::Pattern& _cc) : Injection(_cc) {};

CcInjection::CcInjection(const pattern::Pattern::Component& _cc)
		: Injection(_cc),ccAgToNode(_cc.size(),nullptr) {
	//reuse(_cc,node,port_list,root);
}

bool CcInjection::reuse(const pattern::Pattern::Component& _cc,Node& node,
		map<int,InjSet*>& port_list, const simulation::SimContext& context,small_id root) {
	//ccAgToNode.clear();
	for(auto& n : ccAgToNode)
		n = nullptr;
	//if(_cc.getAgent(root).getId() == node.getId()){
	ccAgToNode.resize(_cc.size(),nullptr);
	map<Node*,small_id> nodeToCcAg;
	std::list<pair<small_id,Node*> > q;
	q.emplace_back(root,&node);
	const pattern::Mixture::Agent* ag;
	const Node* curr_node;
	pair<small_id,Node*> next_node(0,nullptr);
	while(!q.empty()){
		ag = &_cc.getAgent(q.front().first);
		curr_node = q.front().second;
		if(ag->getId() != curr_node->getId())
			return false;
		for(auto& site : *ag){
			auto curr_int_state = curr_node->getInternalState(site.getId());
			next_node.second = site.testMatch(curr_int_state,port_list,context);
			if(next_node.second == nullptr)
				return false;
			if(next_node.second->getId() != short_id(-1)){//compare different pointers
				next_node.first = _cc.follow(q.front().first,site.getId()).first;
				//need to check site of links?? TODO No
				//if(next_node.first >= _cc.size())
				//	return false;
				if(nodeToCcAg.count(next_node.second) && nodeToCcAg[next_node.second] != next_node.first)
					return false;
				if(ccAgToNode[next_node.first]){
					if(ccAgToNode[next_node.first] != next_node.second)
						return false;
				}
				else {
					q.emplace_back(next_node);
					ccAgToNode[next_node.first] = next_node.second;//todo review
					nodeToCcAg[next_node.second] = next_node.first;//todo review
				}//next_node.second = nullptr;
			}
		}
		ccAgToNode[q.front().first] = q.front().second;//case when agent mixture with no sites
		nodeToCcAg[next_node.second] = next_node.first;//todo review
		q.pop_front();
	}
	return true;
	//} else throw False();//TODO check_aditional_edges???
}

CcInjection::~CcInjection(){};

const vector<Node*>& CcInjection::getEmbedding() const {
	return ccAgToNode;
}

void CcInjection::codomain(vector<Node*>& injs,set<Node*>& cod) const {
	int i = 0;
	for(auto node_p : ccAgToNode){
		if(cod.find(node_p) != cod.end())
			throw False();//TODO do not throw
		cod.emplace(node_p);
		injs[i] = node_p;
		i++;
	}
}

size_t CcInjection::count() const {
	return ccAgToNode[0]->getCount();//TODO empty cc ??
}


Injection* CcInjection::clone(const map<Node*,Node*>& mask) const {
	auto new_inj = new CcInjection(pattern());
	new_inj->copy(this,mask);
	return new_inj;
}

void CcInjection::copy(const CcInjection* inj,const map<Node*,Node*>& mask){
	ccAgToNode.resize(inj->ccAgToNode.size());
	for(size_t i = 0; i < ccAgToNode.size(); i++){
		ccAgToNode[i] = (mask.at(inj->ccAgToNode[i]));//safe
	}
}

bool CcInjection::operator==(const Injection& inj) const{
	auto& cc_inj = static_cast<const CcInjection&>(inj);
	if(&ptrn != &inj.pattern())
		return false;
	for(size_t i = 0;i < ccAgToNode.size(); i++)
		if(ccAgToNode[i] != cc_inj.ccAgToNode[i])
			return false;
	return true;
}


/********** CcValueInj *******************/



/************ InjSet *****************/

//----------------------------------



} /* namespace simulation */
