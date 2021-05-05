/*
 * Plot.h
 *
 *  Created on: Dec 12, 2017
 *      Author: naxo
 */

#ifndef SRC_SIMULATION_PLOT_H_
#define SRC_SIMULATION_PLOT_H_

#include <iostream>
#include <fstream>
#include "../state/State.h"
#include "Parameters.h"

namespace simulation {

using namespace std;

class Plot {
protected:
	ofstream file;
public:
	Plot(const pattern::Environment& env,int run_id = 0);
	virtual ~Plot();

	virtual void fill(const state::State& state,const pattern::Environment& env) = 0;
	virtual void fillBefore(const state::State& state,const pattern::Environment& env) = 0;
};

class TimePlot : public Plot {
	FL_TYPE nextPoint;
	FL_TYPE dT;
public:
	TimePlot(const pattern::Environment& env,int run_id = 0);

	void fill(const state::State& state,const pattern::Environment& env) override;
	void fillBefore(const state::State& state,const pattern::Environment& env) override;
};

class EventPlot : public Plot {
	FL_TYPE nextPoint;
	FL_TYPE dE;
public:
	EventPlot(const pattern::Environment& env,int run_id = 0);

	void fill(const state::State& state,const pattern::Environment& env) override;
	void fillBefore(const state::State& state,const pattern::Environment& env) override;
};

} /* namespace simulation */

#endif /* SRC_SIMULATION_PLOT_H_ */
