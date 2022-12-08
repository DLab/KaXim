/*
 * Simulation.h
 *
 *  Created on: Apr 19, 2016
 *      Author: naxo
 */

#ifndef SIMULATION_SIMULATION_H_
#define SIMULATION_SIMULATION_H_

#include <map>
#include <set>
#include <list>
#include <vector>
#include <unordered_map>
#include "Parameters.h"
#include "Plot.h"
#include "SimContext.h"
#include "../state/State.h"
#include "../pattern/Environment.h"
#include "../pattern/RuleSet.h"
#include "../matching/Injection.h"
#include "../grammar/ast/KappaAst.h"


namespace data_structs { class DataTable; }
namespace state { struct EventInfo;}

/** The family of classes that related to the simulation process.
 *
 */
namespace simulation {
using namespace std;


class Simulation : public SimContext {
	friend class state::State;
	friend class Results;
	//int id;
	//pattern::Environment env;
	vector<state::State> states;//vector?

	data_structs::RandomTree* activityTree;
	//GlobalCounter counter;
	Plot* plot;
	mutable map<string,list<const vector<FL_TYPE>*>> rawList;

	set<matching::Injection*> *ccInjections;//[cc_env_id].at(node_id)
	set<matching::Injection*> *mixInjections;//[mix_id].at(node_id)[cc_mix_id]



	mutable state::EventInfo ev;
	mutable unordered_map<small_id,simulation::Perturbation> perts;
	mutable pattern::Dependencies activeDeps;
	mutable list<small_id> pertIds;//maybe part of event-info?
	mutable list<pair<float,pattern::Dependency>> timePerts;//ordered


	vector<state::State*> cells;

	bool done;

	GlobalCounter counter;

	template <typename T>
	list<T> allocParticles(unsigned cells,T count,const list<T>* vol_ratios = nullptr);

	// deterministic
	vector<unsigned> allocAgents1(unsigned cells,unsigned ag_count,const list<float>* vol_ratios = nullptr);
	// random
	vector<unsigned> allocAgents2(unsigned cells, unsigned ag_count, const list<float>* vol_ratios = nullptr);

public:
	//Simulation(pattern::Environment& env,VarVector& vars,int id = 0);
	//Simulation();
	Simulation(SimContext& sim,int _id);
	~Simulation();

	string getName() const {
		return "Sim-"+to_string(id);
	}

	//void setCells(const list<unsigned int>& cells,const VarVector& vars);
	const state::State& getCell(int id) const {
			return *cells.at(id);
	}
	state::State& getCell(int id) {
		return *cells.at(id);
	}
	const vector<state::State*> getCells() const {
		return cells;
	}


	void addNodes(unsigned n,const Mixture& mix){
		auto allocs = allocAgents1(cells.size(),n);
		auto alloc_it = allocs.begin();
		for(auto cell : cells)
			cell->addNodes(*(alloc_it++),mix);
	}
	pair<int,matching::Injection*> chooseInstance(const Pattern::Component& cc,int sel){
		for(auto cell : cells){
			auto& inj_cont =  cell->getInjContainer(cc.getId());
			if(sel < inj_cont.count())
				return make_pair(int(cell->getId()),&inj_cont.choose(sel));
			sel -= inj_cont.count();
		}
		throw invalid_argument("Simulation::chooseInstance(): not found.");
	}
	void removeInstances(unsigned n, const Pattern::Component& cc){
		auto distr = uniform_int_distribution<unsigned>(0,n);
		for(size_t i = 0; i < n; i++){
			auto choose = distr(rng);
			auto comp_inj = chooseInstance(cc,choose);
			for(auto node : *comp_inj.second->getEmbedding()){
				node->removeFrom(*cells[comp_inj.first]);
			}
		}
	}
	void clearInstances(const Pattern::Component& cc){
		for(auto cell : cells)
			cell->getInjContainer(cc.getId()).clear();
	}
	/*virtual FL_TYPE getTokenValue(unsigned id) const {
		FL_TYPE val = 0;
		for(auto cell : cells)
			val += cell->getTokenValue(id);
		return val;
	}
	virtual void setTokenValue(unsigned id,FL_TYPE val) const {
		for(auto cell : cells)
			cell->setTokens(val/cells.size(),id);
	}*/

	//void initialize(const vector<list<unsigned int>>& _cells,grammar::ast::KappaAst&);
	void initialize();
	void run();

	FL_TYPE advanceTime();
	pair<int,int> drawRule();
	void positiveUpdate(const Rule::CandidateMap& wake_up){
		if(ev.comp_id >= 0)
			cells[ev.comp_id]->positiveUpdate(wake_up);
	}

	void tryPerturbate();
	void updateDeps(const pattern::Dependency& d);

	void activityUpdate();
	void updateActivity(int cell_id,small_id r_id){
			auto act = cells[cell_id]->getRuleActivity(r_id);
			activityTree->add(maskSpatialRule(cell_id,r_id),act.first+act.second);
		}

	int count(int cc_id) const {
		int n = 0;
		for(auto cell : cells)
			n += cell->count(cc_id);
		return n;
	}
	void fold(int cc_id,const function<void (const matching::Injection*)> func) const override {
		for(auto cell : cells)
			cell->fold(cc_id,func);
	}

	int getId() const;

	//template <template <typename,typename...> class Range,typename... Args>
	//void addTokens(const Range<int,Args...> &cell_ids,float count,short token_id);

	template <template <typename,typename...> class Range,typename... Args>
	void addAgents(const Range<int,Args...> &cell_ids,unsigned count,const pattern::Mixture& mix);

	static vector<list<unsigned int> > allocCells(int n_cpus, const vector<double> &w_vertex,
			const map<pair<int,int>,double> &w_edges, int tol);

	bool isDone() const override {
		return done;
	}

	data_structs::DataTable* getTrajectory() const;
	data_structs::DataTable* getTrajectory(int i) const;

	void collectRawData() const;
	void collectTabs(map<string,list<const data_structs::DataTable*>> &tab_list) const;

	void print() const;


	//TODO matching::InjRandContainer& getInjContainer(int cc_id);
	//TODO const matching::InjRandContainer& getInjContainer(int cc_id) const;

private:
	static vector<pair<pair<int,int>,double>> sortEdgesByWeidht( const map<pair<int,int>,double> &w_edges );

	static unsigned minP( vector<list<unsigned int>> P );

	static int searchCompartment( vector<list<unsigned int>> assigned, unsigned c );

	int maskSpatialRule(int comp_id,int r_id) const {
		return r_id*cells.size()+comp_id;
	}
	two<int> unmaskSpatialRule(int id) const {
		return two<int>(id % cells.size(),id / cells.size());
	}

};

}

#endif /* SIMULATION_SIMULATION_H_ */
