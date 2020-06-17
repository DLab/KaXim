/*
 * Mixture.h
 *
 *  Created on: Apr 26, 2016
 *      Author: naxo
 */

#ifndef PATTERN_MIXTURE_H_
#define PATTERN_MIXTURE_H_

#include <map>
#include <unordered_map>
#include <vector>
#include <list>
#include <string>
#include "../../util/params.h"
#include "../../expressions/AlgExpression.h"
#include "Pattern.h"
#include "Component.h"




namespace pattern {

using namespace std;

/** \brief Representation of a set of agent complexes.
 * The class Mixture must be initialized with the quantity of agents it will contain.
 * Using the addAgent() and addLink() methods the class is filled until it's complete.
 * Then setComponents(env) must be called to end initialization and declare components
 * in the environment. After that, the mixture is ready to be declare in the environment.
 * Any attempt to add links or agents to the mixture after setComponents() will
 * throw exceptions.
 */
class Mixture : public Pattern {
public:
	using Agent = Pattern::Agent;
	using Site = Pattern::Site;
	using LinkType = Pattern::LinkType;

	struct AuxCoord{
		small_id cc_pos,ag_pos,st_id;
		bool operator==(const AuxCoord& c) const {
			return cc_pos == c.cc_pos && ag_pos == c.ag_pos
					&& st_id == c.st_id;
		}
	};

	const yy::location loc;

	/** \brief Initialize a new Mixture with fixed capacity for agents
	 *
	 * @param agent_count total agents of this mixture.				*/
	Mixture(short agent_count,yy::location loc = yy::location());
	Mixture(const Mixture& m) = delete;
	~Mixture();

	void setId(short_id id);
	const short_id& getId() const override;

	/** \brief Declare an auxiliary variable for this Mixture.
	 * @param aux_name auxiliary variable name.
	 * @param agent_pos the position of the Agent in this Mixture.
	 * @param site_id the signature-id of the agent's site.
	 * @returns false if the aux is already declared in Mixture.		*/
	bool addAuxCoords(string aux_name,small_id agent_pos,small_id site_id);

	/** \brief Returns a const reference to the coordinates of an Auxiliary variable.
	 * @param aux_name auxiliary variable name.
	 * @returns a const reference to the coordinates of the variable.
	 * The coordinates may not be valid until the Mixture is declared.
	const tuple<small_id&,small_id&,small_id>& getAuxCoords(const string& aux_name);*/
	const map<string,AuxCoord>& getAuxMap() const;

	void addAuxRef(expressions::Auxiliar<FL_TYPE>* expr) const;

	/** \brief Add a new agent to the mixture.
	 *
	 * Add a new agent to the mixture from a pointer.
	 * @param a a pointer to agent. */
	void addAgent(Agent* a);

	/** \brief Add one of 2 connected sites under a unique label.
	 * @param lbl the unique id of the edge between sites.
	 * @param ag_pos the agent's mixture-position declaring the site.
	 * @param site_id the id of the site.							 */
	void addBindLabel(int lbl,small_id ag_pos,small_id site_id);

	/** \brief Set the (agent-pos,site-id) binds from labeled binds. */
	void setBindings();

	/** \brief Declare Components to the environment.
	 *
	 * This method has to be call when mixture is a pattern and
	 * components are set. It will declare agents too.
	 * @param env the environment of simulation.		 */
	void declareComponents(Environment &env);

	/** \brief Identify and declare connected components in this mixture.
	 *
	 * Using the binds and agents declared in the mixture, this method
	 * identifies and construct all connected components. This method will
	 * throw if binds are invalid. After this call the Mixture is
	 * considered set (isSet()).										*/
	void setComponents();

	bool isSet() const;

	//bool operator==(const Mixture& m) const;

	/** \brief Returns the agent by (cc-position,ag-pos-in-cc) (not safe). */
	inline const Agent& getAgent(small_id cc_pos,small_id ag_pos) const {
		return comps[cc_pos]->getAgent(ag_pos);
	}
	/** \brief Returns the agent by (cc-position,ag-pos-in-cc) (not safe). */
	inline const Agent& getAgent(ag_st_id cc_ag_pos ) const {
		return comps[cc_ag_pos.first]->getAgent(cc_ag_pos.second);
	}
	/** \brief Returns the agent by mixture position (safe) */
	const Agent& getAgent(small_id ag_pos ) const override;

	/** \brief Returns the agent by mixture position (safe) */
	Agent& getAgent(small_id ag_pos ) ;

	/** \brief Returns the coordinates of the agent by mix position (cc_pos,ag_pos). */
	inline const two<small_id> getAgentCoords(small_id ag_pos) const {
		return ccAgPos.at(ag_pos);
	}
	/** \brief Returns the position of the agent in mix by the coordinates (cc_pos,ag_pos). */
	inline const small_id getAgentPos(ag_st_id cc_ag_pos) const {
		return mixAgPos.at(cc_ag_pos);
	}

	/** \brief Add the rule that declares this LHS pattern
	 * @param id of the rule declaring this pattern.	 */
	void addInclude(small_id id) override;

	/** \brief Returns the CC by position (not safe). */
	inline const Component& getComponent(small_id cc_pos) const{
		return *comps[cc_pos];
	}
	/** \brief Returns the CC by position (safe). */
	Component& getComponent(small_id);


	ag_st_id follow(ag_st_id cc_ag_pos,small_id site) const {
		return comps[cc_ag_pos.first]->follow(cc_ag_pos.second,site);
	}

	/** \brief Returns the number of agents added.	 */
	inline size_t size() const {return agents.size();};
	inline size_t compsCount() const {return comps.size();};

	const vector<Component*>::const_iterator begin() const;
	const vector<Component*>::const_iterator end() const;

	/** \brief returns a string representation of this mixture.
	 *
	 * @param env the environment of simulation.	 */
	string toString(const Environment &env) const;
	string toString(const Environment &env,const vector<ag_st_id> &mask) const;

private:
	vector<Agent*> agents;				///< Mixture's agents. The order is important for LHS
	vector<Component*> comps;			///< Connected components are created with setComponents() methods.
	map<small_id,ag_st_id> ccAgPos;		///< agent-position -> (cc-position,agent-position in cc)
	map<ag_st_id,small_id> mixAgPos;	///< (cc-position,agent-position in cc) -> agent-position
	map<string,AuxCoord> auxCoords;		///< every Auxiliary declared in the mixture.
	mutable list<expressions::Auxiliar<FL_TYPE>*> auxRefs;///< every Aux. Expr. referencing this mixture.

	map<int,list<ag_st_id>> labelBinds;	///< labeled bindings constructed from kappa.
	map< ag_st_id , ag_st_id> binds;	///< the mapping for (agent,site) bindings. Only one edge direction.
	short_id id;						///< the environment id for declared mixtures

};

typedef pair<const Mixture&,const vector<ag_st_id&> >OrderedMixture;

//bool (*f)(pair<short,short>,pair<short,short>) = [](pair<short,short>& p1,pair<short,short>& p2) {return p1.first < p2.first ? p1 : (p1.second < p2.second ? p1 : p2 );};






} /* namespace pattern */

#endif /* PATTERN_MIXTURE_H_ */
