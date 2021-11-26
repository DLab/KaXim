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
#include "../simulation/SimContext.h"
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

namespace data_structs { class DataTable; }

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
class State : public SimContext {
	friend class RateVar;
	friend class simulation::Rule::Rate;
	friend class simulation::AuxDepRate;
	//friend class expressions::SimContext<false>;
	//friend class expressions::SimContext<true>;
	friend class Node;
	friend class SubNode;
	friend class InternalState;

	//const pattern::Environment& env;
	SiteGraph graph;
	const state::BaseExpression& volume;
	//std::vector<Variable*> vars;
	vector<simulation::Rule::Rate*> rates;
	FL_TYPE* tokens;

	data_structs::RandomTree* activityTree;
	CcInjRandContainer** injections;
	map<int,matching::InjRandContainer<matching::MixInjection>*> nlInjections;

	//simulation::LocalCounter counter;
	simulation::Plot& plot;
	mutable EventInfo ev;
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
	State(int id,simulation::Simulation& sim,
			const BaseExpression& vol,
			simulation::Plot& plot);
	~State();


	//void del(Node* node);

	//const simulation::Counter& getCounter() const;

	/** \brief Add tokens population to the state.
	 * @param n count of tokens (can be negative).
	 * @param tok_id token id type to add.
	 */
	void addTokens(float n,unsigned tok_id);
	void setTokens(float n,unsigned tok_id);

	const simulation::Rule::Rate& getRuleRate(int id) const;
	SomeValue getVarValue(short_id var_id) const;
	//const VarVector& getVars() const{
	//	return vars;
	//}


	void positiveUpdate(const Rule::CandidateMap& wake_up);
	void nonLocalUpdate(const simulation::Rule& rule,
			const list<matching::Injection*>& new_injs);
	void activityUpdate();

	FL_TYPE getTotalActivity() const;
	CcInjRandContainer& getInjContainer(int cc_id) override;
	const CcInjRandContainer& getInjContainer(int cc_id) const override;
	MixInjRandContainer& getMixInjContainer(int mix_id) override;
	const MixInjRandContainer& getMixInjContainer(int mix_id) const override;
	FL_TYPE getTokenValue(unsigned tok_id) const override;

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

	void updateActivity(small_id r_id){
		auto act = rates[r_id]->evalActivity(*this);
		activityTree->add(r_id,act.first+act.second);
	}

	//two<FL_TYPE> evalActivity(const simulation::Rule& r) const;

	/** \brief Apply actions described by r to the state.
	 * @param r rule to apply.
	 * @param ev embedding of nodes and other event information.
	 */
	void apply(const simulation::Rule& r);

	void advanceUntil(FL_TYPE sync_t,UINT_TYPE max_e);

	void updateVar(const Variable& val,bool by_value = false);
	void updateDeps(const pattern::Dependency& dep);

	void tryPerturbate();


	void selectInjection(const simulation::Rule &r,two<FL_TYPE> bin_act,
			two<FL_TYPE> un_act);
	const simulation::Rule& drawRule();
	int event();

	void initInjections();
	void initUnaryInjections();
	void initActTree();

	//const pattern::Environment& getEnv() const;
	//const simulation::Simulation& getSim() const;

	void plotOut() const;

	bool isDone() const override {
		return parent->isDone();
	}

	data_structs::DataTable* getTrajectory() const ;
	void collectRawData(map<string,list<const vector<FL_TYPE>*>> &raw_list) const;
	void collectTabs(map<string,list<const data_structs::DataTable*>> &tab_list) const;

	/** \brief Print the state for debugging purposes.
	 **/
	void print() const;

	static const State empty;
};


/*inline const simulation::Simulation& State::getSim() const {
	return sim;
}*/
/*inline const simulation::Counter& State::getCounter() const {
	return counter;
}*/
inline void State::addTokens(float n,unsigned tok_id){
	tokens[tok_id] += n;
}
inline void State::setTokens(float n,unsigned tok_id){
	tokens[tok_id] = n;
}
inline FL_TYPE State::getTokenValue(unsigned tok_id) const{
	return tokens[tok_id];
}
inline const simulation::Rule::Rate& State::getRuleRate(int _id) const {
	return *(rates[_id]);
}
inline SomeValue State::getVarValue(short_id var_id) const {
	return vars[var_id]->getValue(*this);
}
inline FL_TYPE State::getTotalActivity() const {
	return activityTree->total();
}
inline const CcInjRandContainer& State::getInjContainer(int cc_id) const{
	return *(injections[cc_id]);
}
inline matching::InjRandContainer<matching::CcInjection>& State::getInjContainer(int cc_id) {
	return *(injections[cc_id]);
}

inline const MixInjRandContainer& State::getMixInjContainer(int mix_id) const{
	return *(nlInjections.at(mix_id));
}
inline MixInjRandContainer& State::getMixInjContainer(int mix_id) {
	return *(nlInjections.at(mix_id));
}


} /* namespace state */

#endif /* STATE_STATE_H_ */
