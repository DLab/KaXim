/*
 * Results.h
 *
 *  Created on: Oct 27, 2021
 *      Author: naxo
 */

#ifndef SRC_SIMULATION_RESULTS_H_
#define SRC_SIMULATION_RESULTS_H_

#include <string>
#include <vector>
#include <map>
#include "../util/params.h"
#include "../data_structs/DataTable.h"


namespace simulation {

class Simulation;

using namespace std;
using DataTable = data_structs::DataTable;

class Results {
	vector<Simulation*> sims;
	mutable multimap<string,DataTable*> tabs;

public:
	~Results();
	void append(Simulation* sim);

	const Simulation& getSimulation(int sim_id) const;

	DataTable& getTrajectory(int sim_id = 0);
	two<DataTable&> getAvgTrajectory();
	map<string,DataTable*> getSpatialTrajectories(int sim_id);

	void collectHistogram() const;
	void collectRawData() const;

	void printTabNames() const;
	string listTabs() const;
	map<string,list<DataTable*>> getTabs() const;
	DataTable& getTab(string tab_name,int i = 0);
	map<FL_TYPE,vector<FL_TYPE>> getRawData(string name,int i = 0);

	string toString() const;


};

} /* namespace simulation */

#endif /* SRC_SIMULATION_RESULTS_H_ */
