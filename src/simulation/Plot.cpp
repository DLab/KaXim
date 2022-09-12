/*
 * Plot.cpp
 *
 *  Created on: Dec 12, 2017
 *      Author: naxo
 */

#include "Plot.h"
#include "../pattern/Environment.h"
#include "Simulation.h"
#include <boost/filesystem.hpp>

namespace simulation {

using namespace boost::filesystem;

Plot::Plot(const SimContext& sim) : state(sim){
	auto& params = *sim.params;
	auto& env = sim.getEnv();
	//if(params.outputFileType.find("data") != string::npos)
	if(sim.getEnv().getCellCount() > 1)

	if(params.outputFile == "")
		return; //no outputfile
	try {
		path p(params.outputDirectory);
		if(!exists(p))
			create_directories(p);
		else
			if(!is_directory(p))
				throw invalid_argument("Cannot create folder "+p.string()+": another file with the same name exists.");
		if(params.runs > 1){
			char file_name[2000],buff[1000];//big chars[] to avoid errors here
			sprintf(buff,"%s/%%s-%%0%dd.%%s",params.outputDirectory.c_str(),int(log10(params.runs-1))+1);
			sprintf(file_name,buff,params.outputFile.c_str(),sim.getId(),params.outputFileType.c_str());
			if(strlen(file_name) > 1999 || strlen(buff) > 999)
				throw invalid_argument("Output file name is too long");
			file.open(file_name,ios::out);
		}
		else
			file.open((params.outputDirectory+"/"+params.outputFile+"."+params.outputFileType).c_str(),ios::out);
	}
	catch(exception &e){
		throw invalid_argument("Error on creating output file:\n\t"+string(e.what()));
	}
	file << "#Time";
	for(auto obs : env.getObservables())
		file << "\t" << obs->toString();
	file << endl;
}

Plot::~Plot() {
	file.close();
}




TimePlot::TimePlot(const SimContext& sim) : Plot(sim), nextPoint(0.){
	dT = sim.params->maxTime / sim.params->points;
	//if(dT == 0.0)
	//	dT = 0.0001;
}


void TimePlot::fill() {
	auto& env = state.getEnv();
	auto t = state.getCounter().getTime();
	//AuxMixEmb aux_map;
	while(t >= nextPoint && nextPoint <= state.getParams().maxTime){
		file << nextPoint;
		data.emplace_back();
		data.back().emplace_back(nextPoint);
		//cout << state.getSim().getId() <<"]\t" << nextPoint;
		for(auto var : env.getObservables()){
			auto val = state.getVarValue(var->getId());
			file << "\t" << val;
			data.back().emplace_back(val.valueAs<FL_TYPE>());
			//cout << "\t" << state.getVarValue(var->getId(), aux_map);
		}
		file << endl;
		//cout << endl;
		nextPoint += dT;
	}
}

void TimePlot::fillBefore() {
	auto& env = state.getEnv();
	//auto t = min(std::nextafter(state.getCounter().getTime(),0.),state.getCounter().next_sync_at);
	auto t = std::nextafter(state.getCounter().getTime(),0.);//TODO remember why nexafter
	//AuxMixEmb aux_map;
	while(t >= nextPoint && nextPoint <= state.getParams().maxTime){
		file << nextPoint;
		data.emplace_back();
		data.back().emplace_back(nextPoint);
		//cout << state.getSim().getId() <<"]\t" << nextPoint;
		for(auto var : env.getObservables()){
			auto val = state.getVarValue(var->getId());
			data.back().emplace_back(val.valueAs<FL_TYPE>());
			file << "\t" << val;
			//cout << "\t" << state.getVarValue(var->getId(), aux_map);
		}
		file << endl;
		//cout << endl;
		nextPoint += dT;
	}
}


EventPlot::EventPlot(const SimContext& sim) : Plot(sim), nextPoint(0.){
	dE = sim.params->maxEvent / sim.params->points;
	//if(dT == 0.0)
	//	dT = 0.0001;
}

void EventPlot::fill() {
	auto& env = state.getEnv();
	auto e = state.getCounter().getEvent();
	//AuxMixEmb aux_map;
	while(e >= nextPoint){
		file << state.getCounter().getTime();
		data.emplace_back();
		data.back().emplace_back(state.getCounter().getTime());
		//cout << state.getSim().getId() <<"]\t" << nextPoint;
		for(auto var : env.getObservables()){
			auto val = state.getVarValue(var->getId());
			data.back().emplace_back(val.valueAs<FL_TYPE>());
			file << "\t" << val;
			//cout << "\t" << state.getVarValue(var->getId(), aux_map);
		}
		file << endl;
		//cout << endl;
		nextPoint += dE;
	}
	if(state.isDone() && e > (nextPoint-dE)){
		file << state.getCounter().getTime();
		data.emplace_back();
		data.back().emplace_back(state.getCounter().getTime());
		//cout << state.getSim().getId() <<"]\t" << nextPoint;
		for(auto var : env.getObservables()){
			auto val = state.getVarValue(var->getId());
			data.back().emplace_back(val.valueAs<FL_TYPE>());
			file << "\t" << val;
			//cout << "\t" << state.getVarValue(var->getId(), aux_map);
		}
		file << endl;
		//cout << endl;
		nextPoint += dE;
	}
}

void EventPlot::fillBefore() {
	//auto& env = state.getEnv();

}


} /* namespace simulation */
