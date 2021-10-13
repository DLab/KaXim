/*
 * SiteGraph.h
 *
 *  Created on: Mar 23, 2016
 *      Author: naxo
 */

#ifndef STATE_SITEGRAPH_H_
#define STATE_SITEGRAPH_H_

#include <utility> //pair
#include <list>
#include <vector>
#include <functional>
#include "Node.h"
#include "../util/params.h"
#include "../expressions/AlgExpression.h"
#include "../pattern/Signature.h"


namespace pattern {
	class Environment;
}
namespace state {
class State;

using namespace std;

class SiteGraph {
public:
	SiteGraph();
	~SiteGraph();

	void addComponents(unsigned n,const pattern::Mixture::Component& cc,
			const State &context,vector<Node*>& nodes);
	inline void addComponents(unsigned n,const pattern::Mixture::Component& cc,
			const State &context){
		vector<Node*> v(10);
		addComponents(n,cc,context,v);
	}


	/** \brief Put a node in the sitegraph.
	 * Put a node in the sitegraph, set its address,
	 * and use free space in the container if any.
	 */
	void allocate(Node* n);
	/** \brief Put a subnode in the graph at initialization.
	 * Put a subnode in the graph at the beginning of container,
	 * swapping the first normal node to the last position in
	 * container, and set both addresses. Use only at initialization.
	 */
	void allocate(SubNode* n);
	void remove(Node* n);
	void remove(SubNode* n);
	size_t getNodeCount() const;
	vector<Node*>::iterator begin();
	vector<Node*>::iterator end();
	vector<Node*>::const_iterator begin() const;
	vector<Node*>::const_iterator end() const;
	size_t getPopulation() const;
	void decPopulation(size_t pop = 1);

	/*void neighbourhood(map<int,list<Node*>>& nb, int radius = 1000) const {
		nb[0].insert(nb[0].begin(),visited.begin(),visited.end());
	}*/
	/** \brief Explore the neighborhood of nodes.
	 *
	 * Starting from nbh[0] node-list, visit every node in their neighborhood
	 * and stores them once in nbh[distance] lists.
	 * @param nbh Mapping of the NBH as distance -> node list. Used as input and output.
	 * @param visited set of nodes to avoid visiting (filled as output).
	 * @param radius the max distance to look for nodes.
	 * @param filter function to avoid insert a node when true.	 */
	void neighborhood(map<int,list<Node*>>& nbh,set<Node*>& visited, int radius = 1000,
			function<bool(Node*,int)>* filter = nullptr) const;
	bool areConnected(const vector<Node*> &emb1,const vector<Node*> &emb2,int max_dist) const {
		list<Node*> to_visit;
		set<Node*> to_find;
		for(auto node : emb1){
			if(node == nullptr)
				break;
			to_visit.emplace_back(node);
		}
		for(auto node : emb2){
			if(node == nullptr)
				break;
			to_find.emplace(node);
		}
		return areConnected(to_visit,to_find,max_dist);
	}
	bool areConnected(list<Node*> &to_visit,set<Node*>& to_find,int max_dist) const;
	void print(const pattern::Environment& env) const;


protected:
	//Node& addNode(short sign_id);
	//Node& getNode(size_t address);
	vector<Node*> container;
	size_t fresh;
	size_t nodeCount;
	size_t population;
	big_id subNodeCount;
	list<size_t> free;


};




} /* namespace state */

#endif /* STATE_SITEGRAPH_H_ */
