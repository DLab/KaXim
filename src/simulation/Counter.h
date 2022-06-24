/*
 * Counter.h
 *
 *  Created on: Apr 19, 2016
 *      Author: naxo
 */

#ifndef SIMULATION_COUNTER_H_
#define SIMULATION_COUNTER_H_

#include <iostream>
#include <utility>
#include "../util/params.h"

namespace state{
	class State;
}

namespace simulation {
class Simulation;

class Plot;

using namespace std;

class Counter {
	friend class state::State;
	friend class simulation::Simulation;
	friend class Plot;
	friend class TimePlot;
	friend class EventPlot;

	UINT_TYPE events;
	UINT_TYPE null_events;
	UINT_TYPE cons_null_events;
	UINT_TYPE perturbation_events;
	UINT_TYPE null_actions;
	pair<UINT_TYPE,FL_TYPE> last_tick;
	bool initialized;
	unsigned int ticks;
	unsigned int stat_null[6];
	FL_TYPE init_time;
	UINT_TYPE init_event;
	FL_TYPE max_time;
	UINT_TYPE max_events;
	int dE;
	FL_TYPE dT;
	bool stop;

	uint progress_step;
	/* [events|react_evts|diff_evts|nl_evts|cns_nl_evts|pert_evts|nl_action] */
	UINT_TYPE total_events[7];
	UINT_TYPE reaction_ev;
	UINT_TYPE diffusion_ev;

	uint sync_count;
	FL_TYPE next_sync_at;
	FL_TYPE sync_time;
	FL_TYPE sync_event_dt;
	bool zero_reactivity;
public:
	Counter();
	virtual ~Counter();

	virtual const FL_TYPE& getTime() const = 0;
	virtual void setTime(FL_TYPE t) = 0;
	virtual void advanceTime(FL_TYPE t) = 0;
	FL_TYPE getNextSync() const {return next_sync_at;}
	inline UINT_TYPE getEvent() const;
	inline UINT_TYPE getNullEvent() const;
	inline UINT_TYPE getProdEvent() const;
	inline void incEvents();
	inline void incNullEvents(unsigned type);
	inline void incNullActions(UINT_TYPE n = 1);

	virtual string toString() const;

};


class LocalCounter : public Counter {
	const FL_TYPE& time;
	//friend class State;
public:
	LocalCounter(const Counter& glob_counter);
	virtual const FL_TYPE& getTime() const {
		return time;
	};
	inline void advanceTime(FL_TYPE t) {
		throw invalid_argument("Cannot advance LocalCounter time.");
	}
	inline void setTime(FL_TYPE t) {
		throw invalid_argument("Cannot set LocalCounter time.");
	}
	//~LocalCounter();
};

class GlobalCounter : public Counter {
	FL_TYPE time;

public:
	GlobalCounter();
	virtual const FL_TYPE& getTime() const {
		return time;
	}
	inline void advanceTime(FL_TYPE t) {
		time += t;
	}
	inline void setTime(FL_TYPE t) {
		time = t;
	}
};



UINT_TYPE Counter::getEvent() const{
	return events;
}
UINT_TYPE Counter::getNullEvent() const{
	return null_events;
}
UINT_TYPE Counter::getProdEvent() const{
	return events - null_events;
}

void Counter::incEvents(){
	events++;
	cons_null_events = 0;
}
void Counter::incNullEvents(unsigned type){
	stat_null[type]++;
	null_events++;
	cons_null_events++;
}
void Counter::incNullActions(UINT_TYPE n){
	null_actions += n;
}






} /* namespace simulation */

#endif /* SIMULATION_COUNTER_H_ */
