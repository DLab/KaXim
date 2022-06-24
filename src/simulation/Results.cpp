/*
 * Results.cpp
 *
 *  Created on: Oct 27, 2021
 *      Author: naxo
 */

#include "Results.h"
#include "Simulation.h"

#include <unsupported/Eigen/MatrixFunctions>

namespace simulation {


Results::~Results() {
	for(auto sim : sims)
		delete sim;
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

map<string,DataTable*> Results::getSpatialTrajectories(int sim_id) {
	map<string,DataTable*> ret;
	for(auto cell : sims[sim_id]->getCells()){
		string name(cell->getName());
		string key("Sim["+to_string(sim_id)+"] - Trajectory of "+name);
		auto tab_it = tabs.find(key);
		auto tab = tab_it == tabs.end() ? &tabs.emplace(key,nullptr)->second : &tab_it->second;
		if(*tab)
			ret[name] = *tab;
		else
			*tab = ret[name] = cell->getTrajectory();
	}
	return ret;
}

two<DataTable&> Results::getAvgTrajectory() {
	string name("Average Trajectory");
	auto tab_it = tabs.find(name);
	if(tab_it != tabs.end())
		return two<DataTable&>(*(tab_it->second),*(tabs.find("SD")->second));

	auto dt = new DataTable(static_cast<const DataTable&>(getTrajectory(0)));//casting to call copy constructor
	auto& avg = *(tabs.emplace(name,dt)->second);
	if(isinf(sims[0]->params->maxTime)){//event-based plot
		throw invalid_argument("TODO: avg traj for event based sim.");//TODO
	}
	else {
		for(size_t i = 1; i < sims.size(); i++)
			avg.data += getTrajectory(i).data;
		avg.data = avg.data / sims.size();
	}
	dt = new DataTable(static_cast<const DataTable&>(avg));
	dt->data.setZero();
	auto& sd = *(tabs.emplace("SD",dt)->second);
	if(isinf(sims[0]->params->maxTime)){//event-based plot
		throw invalid_argument("TODO: avg traj for event based sim.");//TODO
	}
	else {
		double val;
		for(size_t i = 0; i < sims.size(); i++){
			auto traj_i = getTrajectory(i).data.data();
			for(int j = 0; j < avg.data.size(); j++){
				val = avg.data.data()[j] - traj_i[j];
				sd.data.data()[j] += val*val;
			}
			//cout << sd.data << endl;
		}
		sd.data /= sims.size();
		for(auto it = sd.data.data(); it != sd.data.data()+sd.data.size(); it++)
			*it = sqrt(*it);
	}
	return two<DataTable&>(avg,sd);
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
		sim_tabs.clear();
	}
}

void Results::collectRawData() const {
	for(auto sim : sims) {
		sim->collectRawData();
	}
}

void Results::printTabNames() const {
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

string Results::listTabs() const {
	int i = 0;
	string ret,last_tab;
	for(auto tab : tabs){
		if(tab.first == last_tab) {
			i++;
			continue;
		}
		else if(i > 1)
			ret += " (" + to_string(i) + ")";
		ret += (i?"\n":"") + tab.first;
		last_tab = tab.first;
		i = 1;

	}
	if(i > 1)
		ret += " (" + to_string(i) + ")";
	cout << endl;
	return ret;
}

map<string,list<DataTable*>> Results::getTabs() const {
	map<string,list<DataTable*>> tab_lists;
	for(auto tab : tabs)
		tab_lists[tab.first].emplace_back(tab.second);
	return tab_lists;
}

DataTable& Results::getTab(string s,int i) {
	auto rng = tabs.equal_range(s);
	while(rng.first != rng.second && i--)
		rng.first++;
	if(i != -1)
		throw out_of_range("Data Table "+s+"["+to_string(i)+"]");
	return *(rng.first->second);
}

map<FL_TYPE,vector<FL_TYPE>> Results::getRawData(string s,int i) {
	map<FL_TYPE,vector<FL_TYPE>> ret;
	for(auto vec : sims[i]->rawList.at(s))
		ret.emplace((*vec)[0],*vec);
	return ret;
}

string Results::toString() const {
	return "Results -> "+std::to_string(sims.size())+" Simulation(s), "
			+ std::to_string(tabs.size()) + " (pre-computed) DataFrame(s)";
}


} /* namespace simulation */
