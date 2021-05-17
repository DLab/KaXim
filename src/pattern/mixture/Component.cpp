/*
 * Component.cpp
 *
 *  Created on: Jan 21, 2019
 *      Author: naxo100
 */

#include "Component.h"
#include "Mixture.h"
#include "Agent.h"
#include "../Environment.h"

namespace pattern {

/************** class Component ****************/

Pattern::Component::Component() : id(-1){};
/*Pattern::Component::Component(const Component& comp) : id(-1),
		agents(comp.size(),nullptr),graph(comp.graph){
	for(size_t i = 0; i < agents.size() ; i++)
		*agents[i] = *comp.agents[i];
}*/
Pattern::Component::~Component(){}

size_t Pattern::Component::size() const{
	return agents.size();
}

void Pattern::Component::setId(short_id i) {
	id = i;
}
const short_id& Pattern::Component::getId() const {
	return id;
}

const Mixture::Agent& Pattern::Component::getAgent(small_id ag) const {
	return *(agents[ag]);
}

const vector<Mixture::Agent*>::const_iterator Pattern::Component::begin() const{
	return agents.begin();
}
const vector<Mixture::Agent*>::const_iterator Pattern::Component::end() const{
	return agents.end();
}


short Pattern::Component::addAgent(Mixture::Agent* ag){
	agents.emplace_back(ag);
	//ag->addCc(this,agents.size()-1);
	return agents.size()-1;
}

void Pattern::Component::addBind(ag_st_id p1,ag_st_id p2){
	graph.emplace(p1,p2);
	graph.emplace(p2,p1);
}

map<small_id,small_id> Pattern::Component::declareAgents(Environment &env){
	vector<pair<Agent*,small_id>> old_order;
	map<small_id,small_id> order_mask;//old-pos -> new-pos
	for(unsigned i = 0; i < agents.size(); i++){
		env.declareAgentPattern(agents[i]);//this can change agent pointer
		old_order.emplace_back(agents[i],small_id(i));
	}
	stable_sort(agents.begin(),agents.end());//sorts in pointer order
	stable_sort(old_order.begin(),old_order.end(),[](const pair<Agent*,small_id>& a,const pair<Agent*,small_id>& b) -> bool {
		return a.first < b.first;
	});
	//if(agents.size() == 1)
	//	return order_mask;//debugging purposes
	small_id i = 0;
	for(auto ag : old_order)//this will iterate in order
		order_mask.emplace(ag.second,i++);
	map<ag_st_id,ag_st_id> old_graph(graph);
	graph.clear();
	for(auto bind : old_graph){
		ag_st_id p1(order_mask[bind.first.first],bind.first.second);
		ag_st_id p2(order_mask[bind.second.first],bind.second.second);
		graph.emplace(p1,p2);
	}
	return order_mask;
}

/*ag_st_id Mixture::Component::follow(small_id ag_id,small_id site) const{
	if(graph.count(ag_st_id(ag_id,site)))
		return graph.at(ag_st_id(ag_id,site));
	else
		return make_pair(ag_id,site);
}*/

bool Pattern::Component::operator==(const Component &c) const{
	if(this == &c)
		return true;
	if(c.graph != graph)
		return false;
	if(size() == c.size()){
		for(size_t i = 0; i< size(); i++){
			if(!(*agents[i] == *c.agents[i]))
				return false;
		}
	}
	else
		return false;
	return true;
}

bool Pattern::Component::testEmbed(const Component& rhs_cc,two<small_id> root,
		const simulation::SimContext &context,map<small_id,small_id> &tested) const {
	list<ag_st_id> to_test{root};
	list<small_id> sites_to_follow;
	while(!to_test.empty()){
		sites_to_follow.clear();
		auto emb = to_test.front();
		to_test.pop_front();
		if((agents[emb.first])->testEmbed(*rhs_cc.agents[emb.second],context,sites_to_follow))
			for(auto site : sites_to_follow){
				auto ptrn_trgt = follow(emb.first,site);
				auto rhs_trgt = rhs_cc.follow(emb.second,site);
				//if(ptrn_trgt.second == rhs_trgt.second)//not needed cause Site::testEmbed will compare bind patterns
				if(tested.count(ptrn_trgt.first)){
					if(tested[ptrn_trgt.first] != rhs_trgt.first)
						return false;
				}
				else{
					to_test.emplace_back(ptrn_trgt.first,rhs_trgt.first);
					tested[ptrn_trgt.first] = rhs_trgt.first;
				}
			}
		else
			return false;
	}
	return true;
}


const map<ag_st_id,ag_st_id>& Pattern::Component::getGraph() const{
	return graph;
}

/*vector<small_id> Mixture::Component::setGraph() {
	list<pair<Agent*,short> > reorder;
	for(size_t i = 0; i < agents.size(); i++ )
		reorder.emplace_back(agents[i],(short)i);
	reorder.sort();
	size_t i = 0;
	vector<small_id> mask(agents.size());
	for(auto aptr_id : reorder){
		agents[i] = aptr_id.first;
		mask[aptr_id.second] = i;
		i++;
	}

	auto graph_ptr = new map<ag_st_id,ag_st_id>();
	for(auto l_it = links->begin(); l_it != links->end(); advance(l_it,2)){
		auto p1 = *l_it, p2 = *next(l_it,1);
		graph_ptr->emplace(make_pair(mask[p1.first],p1.second),
				make_pair(mask[p2.first],p2.second));
		graph_ptr->emplace(make_pair(mask[p2.first],p2.second),
				make_pair(mask[p1.first],p1.second));
	}
	delete links;
	graph = graph_ptr;
	return mask;
}*/

void Pattern::Component::addRateDep(const simulation::Rule& dep,small_id cc){
	deps.emplace_back(dep,cc);
}

const list<pair<const simulation::Rule&,small_id>>& Pattern::Component::getRateDeps() const{
	return deps;
}

string Pattern::Component::toString(const Environment& env) const {
	string out, glue = ",";

	// put labels to bindings
	map<ag_st_id,short> bindLabels;
	short bindLabel = 1;
	for(auto lnk = graph.begin() ; lnk != graph.end() ; ++lnk ) {
		/* Logic structure: [ag_id,site_id],[ag_id',site_id'] that means [Agent,Site] is conected with [Agent',Site']
		 * C++ structure:   [lnk->first.first,lnk->first.second],[lnk->second.first,lnk->second.second]
		 * the graph is bidirectional because the links are symmetrical, that means exist [ag_id,site_id],[ag_id',site_id'0]
		 * and [ag_id',site_id'],[ag_id,site_id]
		 */
		if( !bindLabels.count(lnk->first) && !bindLabels.count(lnk->second) ) {
			bindLabels[lnk->first] = bindLabel;
			bindLabels[lnk->second] = bindLabel;
			bindLabel++;

		}

	}

	for( unsigned mixAgId = 0; mixAgId < agents.size(); mixAgId++ ) {
		out += (agents[mixAgId])->toString(env, mixAgId, bindLabels ) + glue;
		bindLabel++;
	}

	// remove the last glue
	if( out.substr(out.size()-glue.size(), out.size()) == glue )
		out = out.substr(0, out.size()-glue.size());

	return out;
}


} /* namespace pattern */
