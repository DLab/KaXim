/*
 * State.h
 *
 *  Created on: Mar 23, 2016
 *      Author: naxo
 */

#ifndef STATE_STATE_H_
#define STATE_STATE_H_

#include <vector>
#include <ctime>
#include <random>
#include "Variable.h"
#include "SiteGraph.h"
#include "../simulation/Counter.h"
#include "../simulation/Rule.h"
#include "../simulation/Perturbation.h"
#include "../data_structs/RandomTree.h"
#include "../pattern/Dependencies.h"
#include "../matching/InjRandSet.h"
#include "../pattern/Action.h"

namespace simulation {
	class Plot;
	class Simulation;
}

namespace state {
struct EventInfo;

/** \brief The state of a (set of) compartment(s).
 *
 * An object of this class store all the data to define
 * the state of a set of compartments to be run by an MPI
 * process. This class contains the SiteGraph of agents
 * and all variables, rules and perturbations declared with
 * Kappa.
 */
class State {
	friend class RateVar;
	friend class simulation::Rule::Rate;
	friend class simulation::AuxDepRate;
	friend class expressions::SimContext<false>;
	friend class expressions::SimContext<true>;
	friend class Node;
	friend class SubNode;
	friend class InternalState;

	const simulation::Simulation& sim;
	const pattern::Environment& env;
	SiteGraph graph;
	const state::BaseExpression& volume;
	std::vector<Variable*> vars;
	vector<simulation::Rule::Rate*> rates;
	float* tokens;

	data_structs::RandomTree* activityTree;
	matching::InjRandContainer** injections;

	mutable RNG rng;
	simulation::LocalCounter counter;
	simulation::Plot& plot;
	mutable EventInfo ev;
	expressions::EvalArgs args;
	mutable unordered_map<small_id,simulation::Perturbation> perts;
	mutable pattern::Dependencies activeDeps;
	mutable list<small_id> pertIds;//maybe part of event-info?
	mutable list<pair<float,pattern::Dependency>> timePerts;//ordered
	//time_t program_t0;

	/** \brief Method of the actions that a rule can apply.
	 * @param act Action to apply and target agents.
	 * @param ev Embedding of nodes and other event information.
	 */
	/*void bind(const simulation::Rule::Action& act);
	void free(const simulation::Rule::Action& act);
	void modify(const simulation::Rule::Action& act);
	void del(const simulation::Rule::Action& act);
	void assign(const simulation::Rule::Action& act);*/
	//void add(const simulation::Rule::Action& a);

	void selectBinaryInj(const simulation::Rule& mix,bool clsh_if_un) const;
	void selectUnaryInj(const simulation::Rule& mix) const;


public:
	/** \brief Initialize the state with the size of token vector, the vars vector and the volume.
	 * @param tok_count the size of token vector.
	 * @param _vars the variable vector.
	 * @param vol the volume of this state.
	 */
	State(const simulation::Simulation& sim,
			const std::vector<Variable*>& _vars,
			const BaseExpression& vol,simulation::Plot& plot,
			const pattern::Environment& env,int seed);
	~State();


	//void del(Node* node);

	const simulation::Counter& getCounter() const;

	/** \brief Add tokens population to the state.
	 * @param n count of tokens (can be negative).
	 * @param tok_id token id type to add.
	 */
	void addTokens(float n,unsigned tok_id);

	float getTokenValue(unsigned tok_id) const;

	const simulation::Rule::Rate& getRuleRate(int id) const;
	SomeValue getVarValue(short_id var_id,const AuxMap& aux_values) const;
	const VarVector& getVars() const{
		return vars;
	}


	void positiveUpdate(const simulation::Rule::CandidateMap& wake_up);

	FL_TYPE getTotalActivity() const;
	RNG& getRandomGenerator() const;
	matching::InjRandContainer& getInjContainer(int cc_id);
	const matching::InjRandContainer& getInjContainer(int cc_id) const;

	/** \brief Add nodes to the SiteGraph using a fully described mixture.
	 * @param n count of copies of the mixture.
	 * @param mix a mixture without patterns to create nodes.
	 * @param env the environment of the simulation.
	 */
	void addNodes(unsigned n,const pattern::Mixture& mix);
	void initNodes(unsigned n,const pattern::Mixture& mix);
	inline void removeNode(Node* n){
		graph.remove(n);
	}

	UINT_TYPE mixInstances(const pattern::Mixture& mix) const;

	//two<FL_TYPE> evalActivity(const simulation::Rule& r) const;

	/** \brief Apply actions described by r to the state.
	 * @param r rule to apply.
	 * @param ev embedding of nodes and other event information.
	 */
	void apply(const simulation::Rule& r);

	void advanceUntil(FL_TYPE sync_t);

	void updateVar(const Variable& val,bool by_value = false);
	void updateDeps(const pattern::Dependency& dep);

	void tryPerturbate();


	void selectInjection(const simulation::Rule &r,two<FL_TYPE> bin_act,
			two<FL_TYPE> un_act);
	const simulation::Rule& drawRule();
	int event();

	void initInjections();
	void initActTree();

	const pattern::Environment& getEnv() const;
	const simulation::Simulation& getSim() const;

	/** \brief Print the state for debugging purposes.
	 **/
	void print() const;

	static const State empty;
};




inline const pattern::Environment& State::getEnv() const{
	return env;
}
inline const simulation::Simulation& State::getSim() const {
	return sim;
}
inline const simulation::Counter& State::getCounter() const {
	return counter;
}
inline void State::addTokens(float n,unsigned tok_id){
	tokens[tok_id] += n;
}
inline float State::getTokenValue(unsigned tok_id) const{
	return tokens[tok_id];
}
inline const simulation::Rule::Rate& State::getRuleRate(int _id) const {
	return *(rates[_id]);
}
inline SomeValue State::getVarValue(short_id var_id, const AuxMap& aux_values) const {
	return vars[var_id]->getValue(args);
}
inline FL_TYPE State::getTotalActivity() const {
	return activityTree->total();
}
inline RNG& State::getRandomGenerator() const{
	return rng;
}
inline const matching::InjRandContainer& State::getInjContainer(int cc_id) const{
	return *(injections[cc_id]);
}
inline matching::InjRandContainer& State::getInjContainer(int cc_id) {
	return *(injections[cc_id]);
}


} /* namespace state */

#endif /* STATE_STATE_H_ */
