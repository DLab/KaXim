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

Plot::Plot(const pattern::Environment& env,int run_id) {
	auto& params = Parameters::get();
	try {
		path p(params.outputDirectory);
		if(!exists(p))
			create_directories(p);
		else
			if(!is_directory(p))
				throw invalid_argument("Cannot create folder "+p.string()+": another file with the same name exists.");
		if(params.runs > 1){
			char file_name[400],buff[200];//big chars[] to avoid errors here
			sprintf(buff,"%s/%%s-%%0%dd.%%s",params.outputDirectory.c_str(),int(log10(params.runs-1))+1);
			sprintf(file_name,buff,params.outputFile.c_str(),run_id,params.outputFileType.c_str());
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




TimePlot::TimePlot(const pattern::Environment& env,int run_id) : Plot(env,run_id), nextPoint(0.){
	auto& params = Parameters::get();
	dT = params.maxTime / params.points;
	//if(dT == 0.0)
	//	dT = 0.0001;
}


void TimePlot::fill(const state::State& state,const pattern::Environment& env) {
	auto t = min(state.getCounter().getTime(),state.getCounter().next_sync_at);
	//AuxMixEmb aux_map;
	while(t >= nextPoint){
		file << nextPoint;
		//cout << state.getSim().getId() <<"]\t" << nextPoint;
		for(auto var : env.getObservables()){
			file << "\t" << state.getVarValue(var->getId());
			//cout << "\t" << state.getVarValue(var->getId(), aux_map);
		}
		file << endl;
		//cout << endl;
		nextPoint += dT;
	}
}

void TimePlot::fillBefore(const state::State& state,const pattern::Environment& env) {
	auto t = min(std::nextafter(state.getCounter().getTime(),0.),state.getCounter().next_sync_at);
	//AuxMixEmb aux_map;
	while(t >= nextPoint){
		file << nextPoint;
		//cout << state.getSim().getId() <<"]\t" << nextPoint;
		for(auto var : env.getObservables()){
			file << "\t" << state.getVarValue(var->getId());
			//cout << "\t" << state.getVarValue(var->getId(), aux_map);
		}
		file << endl;
		//cout << endl;
		nextPoint += dT;
	}
}

EventPlot::EventPlot(const pattern::Environment& env,int run_id) : Plot(env,run_id), nextPoint(0.){
	auto& params = Parameters::get();
	dE = params.maxEvent / params.points;
	//if(dT == 0.0)
	//	dT = 0.0001;
}

void EventPlot::fill(const state::State& state,const pattern::Environment& env) {
	auto e = state.getCounter().getEvent();
	//AuxMixEmb aux_map;
	while(e >= nextPoint){
		file << state.getCounter().getTime();
		//cout << state.getSim().getId() <<"]\t" << nextPoint;
		for(auto var : env.getObservables()){
			file << "\t" << state.getVarValue(var->getId());
			//cout << "\t" << state.getVarValue(var->getId(), aux_map);
		}
		file << endl;
		//cout << endl;
		nextPoint += dE;
	}
	if(state.isDone() && e > (nextPoint-dE)){
		file << state.getCounter().getTime();
		//cout << state.getSim().getId() <<"]\t" << nextPoint;
		for(auto var : env.getObservables()){
			file << "\t" << state.getVarValue(var->getId());
			//cout << "\t" << state.getVarValue(var->getId(), aux_map);
		}
		file << endl;
		//cout << endl;
		nextPoint += dE;
	}
}

void EventPlot::fillBefore(const state::State& state,const pattern::Environment& env) {

}


} /* namespace simulation */
