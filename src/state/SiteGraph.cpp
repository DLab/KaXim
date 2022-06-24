/*
 * SiteGraph.cpp
 *
 *  Created on: Mar 23, 2016
 *      Author: naxo
 */

#include "SiteGraph.h"
#include "State.h"
#include "../pattern/Environment.h"
#include "../pattern/mixture/Agent.h"
#include "../pattern/mixture/Component.h"
#include "../simulation/Parameters.h"

#include <algorithm>
//#include <a>

namespace state {

SiteGraph::SiteGraph() : fresh(0),nodeCount(0),population(0),subNodeCount(0) {
	//container.reserve(10);
}

SiteGraph::~SiteGraph() {
	for(auto node : container){
		if(node)
			delete node;
	}
}

#define MAX_CC_SIZE 10	//TODO
#define MULTINODE_LIM 500 //TODO
void SiteGraph::addComponents(unsigned n,const pattern::Mixture::Component& cc,
		const State &context,vector<Node*>& buff_nodes) {
	unsigned i = 0;
	if(!context.params->useMultiNode || n < MULTINODE_LIM) {//=> n = 1
		for(unsigned j = 0; j < n; j++){
			i = 0;
			for(auto p_ag : cc){
				auto node = new Node(context.getEnv().getSignature(p_ag->getId()));
				//node->setCount(n);
				//cout << "A" << endl;
				for(auto& site : *p_ag)
					if(site.getValueType() != pattern::Pattern::EMPTY)
						node->setInternalValue(site.getId(),site.getValue(context));
				//cout << "B" << endl;
				this->allocate(node);
				buff_nodes[i] = node;
				i++;
			}
			for(auto bind : cc.getGraph())
				buff_nodes[bind.first.first]->setInternalLink(bind.first.second,
						buff_nodes[bind.second.first],bind.second.second);
		}
	} else {
		auto multi = new MultiNode(cc.size(),n);
		for(auto p_ag : cc){
			auto node = new SubNode(context.getEnv().getSignature(p_ag->getId()),*multi);
			for(auto& site : *p_ag)
				if(site.getValueType() != pattern::Pattern::EMPTY)
					node->setInternalValue(site.getId(),site.getValue(context));
			this->allocate(node);
			buff_nodes[i] = node;
			i++;
		}
		for(auto bind : cc.getGraph())
			buff_nodes[bind.first.first]->setInternalLink(bind.first.second,
					buff_nodes[bind.second.first],bind.second.second);
	}
}


void SiteGraph::allocate(Node* node){
	if(!free.empty()){
		container[free.front()] = node;
		node->alloc(free.front());
		free.pop_front();
	}
	else{
		node->alloc(container.size());
		/*if(container.size() == 5){
			cout << "going to bug" << endl;
		}*/
		container.push_back(node);
	}
	nodeCount++;
	population += node->getCount();
}


void SiteGraph::allocate(SubNode* node){
	if(subNodeCount < container.size()){
		container[subNodeCount]->alloc(container.size());
		container.push_back(container[subNodeCount]);
		container[subNodeCount] = node;
	}
	else{
		container.push_back(node);
	}
	node->alloc(subNodeCount);
	subNodeCount++;
}

void SiteGraph::remove(Node* node){
	free.push_back(node->getAddress());
	if(node->getAddress() > container.size() || node->getAddress() < 0)
		cout << "\nERRRRRROOOOOOOOOOOOOOOORRRRRRRRRRRRR\n" << endl;
	container[node->getAddress()] = nullptr;
	nodeCount--;
	population--;// -= node->getCount();
	//delete node;   Do not delete, save nodes for reuse
}

void SiteGraph::moveIn(set<Node*> nodes){
	for(auto node : nodes){
		allocate(node);//could be faster
	}
}

void SiteGraph::moveOut(set<Node*> nodes,InjSet& injs){
	for(auto node : nodes){
		injs.insert(node->getLifts().begin(),node->getLifts().end());
		remove(node);//could be faster
	}
}


void SiteGraph::remove(SubNode* node){
	subNodeCount--;
	//free.push_back(node->getAddress());
	//container[node->getAddress()] = nullptr;
	//population--;// -= node->getCount();
	//delete node;   Do not delete, save nodes for reuse
}

size_t SiteGraph::getNodeCount() const{
	return nodeCount;
}
size_t SiteGraph::getPopulation() const {
	return population;
}

void SiteGraph::decPopulation(size_t pop){
	population -= pop;
}

vector<Node*>::iterator SiteGraph::begin() {
	return container.begin();
}
vector<Node*>::iterator SiteGraph::end() {
	return container.end();
}

vector<Node*>::const_iterator SiteGraph::begin() const {
	return container.cbegin();
}
vector<Node*>::const_iterator SiteGraph::end() const {
	return container.cend();
}

void SiteGraph::neighborhood(map<int,list<Node*>>& nb,set<Node*>& visited,
		int radius,function<bool(Node*,int)>* filter) const {
	//radius -> nodes
	list<Node*> to_visit(nb[0]);
	if(filter)
		for(auto node : to_visit){
			if((*filter)(node,0))
				nb[0].emplace_back(node);
			visited.emplace(node);
		}
	else
		visited.insert(to_visit.begin(),to_visit.end());
	for(int r = 0; r < radius; r++){
		if(nb.count(r) == 0)
			return;
		int n = to_visit.size();
		for(int i = 0; i < n;i++){
			auto node = to_visit.front();
			to_visit.pop_front();
			for(auto site : *node){
				auto target_node = site->getLink().first;
				if(target_node && visited.emplace(target_node).second){
					to_visit.emplace_back(target_node);
					if(!(filter && (*filter)(target_node,r+1)))
						nb[r+1].emplace_back(target_node);	//only if no filter or filter(node) = true
				}
			}
		}
	}
}

bool SiteGraph::areConnected(list<Node*> &to_visit,
		set<Node*>& to_find,int max_dist) const {
	set<Node*> visited(to_visit.begin(),to_visit.end());
	while(!to_visit.empty() && max_dist){
		int n = to_visit.size();
		while(n--){
			auto node = to_visit.front();
			to_visit.pop_front();
			for(auto site : *node){
				auto target_node = site->getLink().first;
				if(target_node){
					if(to_find.count(target_node))
						return true;
					else if(visited.emplace(target_node).second)
						to_visit.emplace_back(target_node);
				}
			}
		}
		max_dist--;
	}
	return false;
}

void SiteGraph::print(const pattern::Environment& env) const {
	cout << "StateGraph[" << getPopulation() << "]";
	big_id nodes_to_show = std::min<big_id>(100,container.size());

	if(nodes_to_show)
		cout << " -> {\n";
	else
		cout << endl;
	for(big_id i = 0; i < nodes_to_show; i++){
		auto node = container[i];
		if(!node)
			continue;
		cout << "  [" << node->getAddress() << "] ";
		if(node->getCount() > 1)
			cout << node->getCount() << " ";
		cout << node->toString(env,true) << endl;
		if(i != node->getAddress())
			throw std::invalid_argument("bad allocation of node.");
	}
	if(nodes_to_show)
		cout << "}\n";
}

string SiteGraph::toString(const pattern::Environment& env) const {
	string ret;
	ret += "StateGraph[" + to_string(getPopulation()) + "]";
	big_id nodes_to_show = std::min<big_id>(100,container.size());

	if(nodes_to_show)
		ret += " -> {\n";
	else
		ret += "\n";
	for(big_id i = 0; i < nodes_to_show; i++){
		auto node = container[i];
		if(!node)
			continue;
		ret += "  [" + to_string(node->getAddress()) + "] ";
		if(node->getCount() > 1)
			ret += to_string(node->getCount()) + " ";
		ret += node->toString(env,true) +"\n";
		if(i != node->getAddress())
			throw std::invalid_argument("bad allocation of node.");
	}
	if(nodes_to_show)
		ret += "}\n";
	return ret;
}


} /* namespace ast */
