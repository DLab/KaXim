/*
 * CcInjection.cpp
 *
 *  Created on: Oct 7, 2021
 *      Author: naxo
 */

#include "CcInjection.h"
/*
#include "../state/State.h"
#include "../expressions/Vars.h"
#include "../pattern/mixture/Component.h"
#include "../pattern/mixture/Agent.h"
*/
namespace matching {



/********** CcInjection **********/
CcInjection::CcInjection(const pattern::Pattern& _cc) :
		Injection(_cc),ccAgToNode(_cc.size(),nullptr) {}

/*CcInjection::CcInjection(const pattern::Pattern& _cc) : Injection(_cc) {};

CcInjection::CcInjection(const pattern::Pattern::Component& _cc)
		: Injection(_cc),ccAgToNode(_cc.size(),nullptr) {
	//reuse(_cc,node,port_list,root);
}*/

bool CcInjection::reuse(const simulation::SimContext& context,
		Node* node,map<int,InjSet*>& port_list, small_id root) {
	//ccAgToNode.clear();
	for(auto& n : ccAgToNode)
		n = nullptr;
	//if(_cc.getAgent(root).getId() == node.getId()){
	ccAgToNode.resize(ptrn.size(),nullptr);
	map<Node*,small_id> nodeToCcAg;
	std::list<pair<small_id,Node*> > q;
	q.emplace_back(root,node);
	const pattern::Mixture::Agent* ag;
	const Node* curr_node;
	pair<small_id,Node*> next_node(0,nullptr);
	while(!q.empty()){
		ag = &ptrn.getAgent(q.front().first);
		curr_node = q.front().second;
		if(ag->getId() != curr_node->getId())
			return false;
		for(auto& site : *ag){
			auto curr_int_state = curr_node->getInternalState(site.getId());
			next_node.second = site.testMatch(curr_int_state,port_list,context);
			if(next_node.second == nullptr)
				return false;
			if(next_node.second->getId() != short_id(-1)){//compare different pointers
				next_node.first = ptrn.follow(q.front().first,site.getId()).first;
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

/*const vector<Node*>& CcInjection::getEmbedding() const {
	return ccAgToNode;
}*/

bool CcInjection::codomain(vector<Node*>* injs,set<Node*>& cod) const {
	int i = 0;
	for(auto node_p : ccAgToNode){
		if(cod.find(node_p) != cod.end())
			return 2;//overlapped codomains
		cod.emplace(node_p);
		injs[0][i] = node_p;
		i++;
	}
	return 0;
}

/*size_t CcInjection::count() const {
	return ccAgToNode[0]->getCount();//TODO empty cc ??
}*/

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




} /* namespace matching */
