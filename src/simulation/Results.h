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
	vector<Simulation*> sims;					///> Simulation stored by this Results object (after run_kappa_sim())
	mutable multimap<string,DataTable*> tabs;	///> DataTables obtained by this object methods are name-mapped here.

public:
	~Results();
	void append(Simulation* sim);

	/** \brief returns the [sim_id] simulation stored in this results. */
	const Simulation& getSimulation(int sim_id) const;

	/** \brief Gets the global time-trajectory of [sim_id] simulation. */
	DataTable& getTrajectory(int sim_id = 0);
	/** \brief Gets the Average time-trajectory and its standard deviation
	 *  of all simulations in this Results object. */
	two<DataTable&> getAvgTrajectory();
	/** \brief Gets local time-trajectories of every compartment defined in the simulation [sim_id]. */
	map<string,DataTable*> getSpatialTrajectories(int sim_id);

	/** \brief Produces and stores every Histogram done by every simulation. Histograms can be created using
	 *  Kappa perturbations (%mod:) and [Histogram] action.	 */
	void collectHistogram() const;
	/** \brief Produces and stores every Histogram Data done by every simulation. Histograms can be created using
	 *  Kappa perturbations (%mod:) and [Histogram] action.	 */
	void collectRawData() const;

	/** \brief Print the names of every trajectory, Histogram or Raw-Data stored by this Results object. */
	void printTabNames() const;
	/** \brief Returns a string with the names of every trajectory, Histogram or Raw-Data stored by this Results object. */
	string listTabs() const;
	/** \brief Returns the map containing every trajectory, Histogram or Raw-Data stored by this Results object. */
	map<string,list<DataTable*>> getTabs() const;
	/** \brief Returns the i-th DataTable stored in this object under tab_name. There can be several Histograms with same
	 * name but taken at different times. */
	DataTable& getTab(string tab_name,int i = 0);
	/** \brief Returns the Raw Data of i-th named histogram. */
	map<FL_TYPE,vector<FL_TYPE>> getRawData(string name,int i = 0);

	/** \brief Return a string representation of this Results object. */
	string toString() const;


};

} /* namespace simulation */

#endif /* SRC_SIMULATION_RESULTS_H_ */
