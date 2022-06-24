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
	//TODO class outputs {op<<()}
	const SimContext& state;
	ofstream file;
	list<list<FL_TYPE>> data;
public:
	Plot(const SimContext& sim);
	virtual ~Plot();

	virtual void fill() = 0;
	virtual void fillBefore() = 0;

	const list<list<FL_TYPE>>& getData() const {
		return data;
	}

	virtual string getType() const = 0;

};

class TimePlot : public Plot {
	FL_TYPE nextPoint;
	FL_TYPE dT;
public:
	TimePlot(const SimContext& sim);

	void fill() override;
	void fillBefore() override;

	virtual string getType() const override {
		return "Time";
	}
};

class EventPlot : public Plot {
	FL_TYPE nextPoint;
	FL_TYPE dE;
public:
	EventPlot(const SimContext& sim);

	void fill() override;
	void fillBefore() override;

	virtual string getType() const override {
		return "Event";
	}
};

} /* namespace simulation */

#endif /* SRC_SIMULATION_PLOT_H_ */
