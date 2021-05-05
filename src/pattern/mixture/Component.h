/*
 * Component.h
 *
 *  Created on: Jan 21, 2019
 *      Author: naxo100
 */

#ifndef SRC_PATTERN_MIXTURE_COMPONENT_H_
#define SRC_PATTERN_MIXTURE_COMPONENT_H_

//#include "Mixture.h"
#include "Pattern.h"
#include "Agent.h"

namespace pattern {

class Environment;

using namespace std;
/** \brief Defines a set of agents that are explicitly connected by sites.
 * Class Mixture is initialized empty and then filled with agents and binds.
 * After that, setGraph() must be called to reorder agents and produce graph,
 * making the mixture comparable and ready to be declared in the environment.
 *
 */
class Pattern::Component : public Pattern {
	friend class Mixture;
	short_id id;				///< id of a declared Component
	vector<Agent*> agents;		///< agent-pointer pointer (referencing to mixture's agent).
	///(ag_pos,site_id) -> (ag_pos,site_id)
	map<ag_st_id,ag_st_id> graph;

	list<pair<const simulation::Rule&,small_id>> deps;

public:
	Component();
	Component(const Component& comp) = delete;
	~Component();
	//bool contains(const Mixture::Agent* a);
	short addAgent(Pattern::Agent* a);
	void addBind(ag_st_id p1,ag_st_id p2);

	map<small_id,small_id> declareAgents(Environment& env);

	size_t size() const override;
	void setId(short_id i);
	const short_id& getId() const override;

	const vector<Agent*>::const_iterator begin() const;
	const vector<Agent*>::const_iterator end() const;

	//vector<small_id> setGraph();
	const map<ag_st_id,ag_st_id>& getGraph() const;
	string toString(const Environment& env) const;

	const Agent& getAgent(small_id ag_id ) const override;

	//void setAuxCoords(const std::map<std::string,std::tuple<int,small_id,small_id>>& aux_coords);

	/** \brief Returns agent and site ids of site-link (unsafe).		*/
	inline ag_st_id follow(small_id ag_id,small_id site) const {
		return graph.at(ag_st_id(ag_id,site));
	}

	bool operator==(const Pattern::Component &m) const;

	/** \brief Tests if this CC pattern embeds the rhs_cc.
	 * @param rhc_cc a Component to test embed. It should not be a pattern.
	 * @param root a pair (ag-pos in ptrn,ag-pos in rhs_cc) to start embedding.
	 * @param vars the variables of the simulation.
	 * @param emb an empty map to store and return the embedding. Can be omitted.*/
	bool testEmbed(const Component& rhs_cc,two<small_id> root,
			const simulation::SimContext &context,map<small_id,small_id>& emb) const;
	bool testEmbed(const Component& rhs_cc,two<small_id> root,const simulation::SimContext &context) const {
		map<small_id,small_id> emb;
		return testEmbed(rhs_cc,root,context,emb);
	}

	void addRateDep(const simulation::Rule& dep,small_id cc);
	/** \brief Returns a list of (Rule,cc-pos) where Rule's rate value
	 * depends on Node's internal values matching with this CC.
	 *
	 * This CC is declared in LHS mixture of the rule at position cc-pos. */
	const list<pair<const simulation::Rule&,small_id>>& getRateDeps() const;
};


} /* namespace pattern */

#endif /* SRC_PATTERN_MIXTURE_COMPONENT_H_ */
