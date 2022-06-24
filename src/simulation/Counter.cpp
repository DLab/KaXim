/*
 * Counter.cpp
 *
 *  Created on: Apr 19, 2016
 *      Author: naxo
 */

#include "Counter.h"

namespace simulation {

Counter::Counter() : events(0),null_events(0),cons_null_events(0),
		perturbation_events(0),null_actions(0),stat_null({}),zero_reactivity(false),
		initialized(false),dT(.0),dE(.0),next_sync_at(-1.),max_time(-1.),max_events(0),
		sync_count(0),sync_time(0),ticks(0),init_event(0),diffusion_ev(0),reaction_ev(0),
		init_time(0),progress_step(0),stop(false),sync_event_dt(.0){}

Counter::~Counter() {}


string Counter::toString() const {
	string ret;
	ret += "Simulated time: " + to_string(getTime()) + "\tEvents: " + to_string(events);
	ret += "\nNull-Events: " + to_string(null_events) +
			" ["+to_string(stat_null[0])+","+to_string(stat_null[1])+","+
			to_string(stat_null[2])+","+to_string(stat_null[3])+","+to_string(stat_null[4])+"]\n";
	ret += "Null-actions = " + to_string(null_actions);
	return ret;
}





LocalCounter::LocalCounter(const Counter& counter) : Counter(),
		time(counter.getTime()) {}


GlobalCounter::GlobalCounter() : Counter(),time(0.) {}



} /* namespace pattern */
