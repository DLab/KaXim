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

namespace simulation {
using namespace std;


class Simulation : public SimContext {
	//int id;
	//pattern::Environment env;
	vector<state::State> states;//vector?
	pattern::RuleSet rules;
	//GlobalCounter counter;
	Plot* plot;
	mutable map<string,list<const vector<FL_TYPE>*>> rawList;

	set<matching::Injection*> *ccInjections;//[cc_env_id].at(node_id)
	set<matching::Injection*> *mixInjections;//[mix_id].at(node_id)[cc_mix_id]


	unordered_map<unsigned int,state::State> cells;

	bool done;

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

	//void setCells(const list<unsigned int>& cells,const VarVector& vars);
	const state::State& getCell(int id) const;

	void initialize(const vector<list<unsigned int>>& _cells,grammar::ast::KappaAst&);
	void run(const Parameters& params);

	int getId() const;

	template <template <typename,typename...> class Range,typename... Args>
	void addTokens(const Range<int,Args...> &cell_ids,float count,short token_id);

	template <template <typename,typename...> class Range,typename... Args>
	void addAgents(const Range<int,Args...> &cell_ids,unsigned count,const pattern::Mixture& mix);

	static vector<list<unsigned int> > allocCells(int n_cpus, const vector<double> &w_vertex,
			const map<pair<int,int>,double> &w_edges, int tol);

	bool isDone() const override {
		return done;
	}

	data_structs::DataTable* getTrajectory() const;

	void collectRawData() const;
	void collectTabs(map<string,list<const data_structs::DataTable*>> &tab_list) const;

	void print() const;


	//TODO matching::InjRandContainer& getInjContainer(int cc_id);
	//TODO const matching::InjRandContainer& getInjContainer(int cc_id) const;

private:
	static vector<pair<pair<int,int>,double>> sortEdgesByWeidht( const map<pair<int,int>,double> &w_edges );

	static unsigned minP( vector<list<unsigned int>> P );

	static int searchCompartment( vector<list<unsigned int>> assigned, unsigned c );

};

}

#endif /* SIMULATION_SIMULATION_H_ */
