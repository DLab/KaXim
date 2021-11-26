/*
 * Results.cpp
 *
 *  Created on: Oct 27, 2021
 *      Author: naxo
 */

#include "Results.h"
#include "Simulation.h"

namespace simulation {


Results::~Results() {
	//for(auto sim : sims)
	//	delete sim;
}


void Results::append(Simulation* sim) {
	if(!sim->isDone())
		throw invalid_argument("Simulation is not done!");
	if(sim->getId()+1 > sims.size())
		sims.resize(sim->getId()+1);
	sims[sim->getId()] = sim;
}

const Simulation& Results::getSimulation(int sim_id) const {
	return *sims.at(sim_id);
}

DataTable& Results::getTrajectory(int sim_id) {
	string name(sims.size() > 1 ? "Sim["+to_string(sim_id)+"] - " : "");
	name += "Trajectory";
	auto tab_it = tabs.find(name);
	if(tab_it != tabs.end())
		return *(tab_it->second);
	return *(tabs.emplace(name,sims.at(sim_id)->getTrajectory())->second);
}
DataTable& Results::getAvgTrajectory() {
	string name("Average Trajectory");
	auto tab_it = tabs.find(name);
	if(tab_it != tabs.end())
		return *(tab_it->second);

	auto dt = new DataTable(static_cast<const DataTable&>(getTrajectory(0)));
	auto& tab = *(tabs.emplace(name,dt)->second);
	if(isinf(Parameters::get().maxTime)){//event-based plot
		//TODO
	}
	else {
		for(size_t i = 1; i < sims.size(); i++)
			tab.data += getTrajectory(i).data;
		tab.data = tab.data / sims.size();
	}
	return tab;
}

void Results::collectHistogram() const {
	map<string,list<const data_structs::DataTable*>> sim_tabs;
	for(auto sim : sims) {
		sim->collectTabs(sim_tabs);
		for(auto& tab_list : sim_tabs){
			string name(sims.size()>1? "Sim["+to_string(sim->getId())+"] - ":"");
			name += "Histogram of " + tab_list.first;
			for(auto tab : tab_list.second){
				tabs.emplace(name,new DataTable(*tab));
			}
		}
	}
}

void Results::listTabs() const {
	int i = 0;
	string last_tab("");
	for(auto tab : tabs){
		if(tab.first == last_tab) {
			i++;
			continue;
		}
		else if(i > 1)
			cout << " (" << i << ")";
		cout << (i?"\n":"") << tab.first;
		last_tab = tab.first;
		i = 1;

	}
	if(i > 1)
		cout << " (" << i << ")";
	cout << endl;
}

DataTable& Results::getTab(string s,int i) {
	auto rng = tabs.equal_range(s);
	while(i-- && rng.first != rng.second)
		rng.first++;
	if(i != -1)
		throw out_of_range("Data Table "+s+"["+to_string(i)+"]");
	return *(rng.first->second);
}

string Results::toString() const {
	return "Results -> "+std::to_string(sims.size())+" Simulation(s), "
			+ std::to_string(tabs.size()) + " (pre-computed) DataFrame(s)";
}


} /* namespace simulation */
