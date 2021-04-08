/*
 * SimContext.h
 *
 *  Created on: Oct 13, 2020
 *      Author: naxo100
 */

#ifndef SRC_SIMULATION_SIMCONTEXT_H_
#define SRC_SIMULATION_SIMCONTEXT_H_

#include <cmath>
#include "Counter.h"
#include "../state/Variable.h"
#include "../pattern/Environment.h"
//#include "../data_structs/ValueMap.h"

namespace expressions {
template <typename T> class Auxiliar;
}

namespace matching {
class InjRandContainer;
}

namespace simulation {



class SimContext {
protected:
	int id;
	const pattern::Environment &env;
	VarVector vars;
	Counter counter;
	//bool safe;	///< set to true when next evaluation will be safe.


	mutable RNG rng;
	mutable const expressions::AuxMap* auxMap;
	mutable const expressions::AuxIntMap* auxIntMap;

	const SimContext* parent;

public:
	SimContext(const pattern::Environment& _env,int seed) :
			id(-1),env(_env),rng(seed),auxMap(nullptr),
			auxIntMap(nullptr),parent(nullptr) {	}
	SimContext(int _id,SimContext* parent_context) :
			id(_id),env(parent_context->getEnv()),
			vars(parent_context->getVars()),
			auxMap(nullptr),auxIntMap(nullptr),
			parent(parent_context) {
		if(parent){
			rng.seed(uniform_int_distribution<long>()(parent_context->getRandomGenerator()));
		}
		else
			throw invalid_argument("SimContext cannot be constructed with a null parent.");
	}
	//SimContext(SimContext* parent_context) : parent(parent_context) {}
	virtual ~SimContext() {};

	inline int getId() const {
		return id;
	}

	inline const VarVector& getVars() const {
		return vars;
	}
	inline VarVector& getVars() {
		return vars;
	}
	inline const pattern::Environment& getEnv() const {
		return env;
	}
	/*inline pattern::Environment& getEnv() {
		return env;
	}*/
	inline const Counter& getCounter() const {
		return counter;
	}

	inline void setAuxMap(const expressions::AuxMap* aux) const {
		auxMap = aux;
	}
	inline void setAuxIntMap(const expressions::AuxIntMap* aux) const {
		auxIntMap = aux;
	}

	inline const expressions::AuxMap& getAuxMap() const;
	inline const expressions::AuxIntMap& getAuxIntMap() const;

	inline RNG& getRandomGenerator() const {
		return rng;
	}

	virtual bool isDone() const {
		return false;
	}
	virtual FL_TYPE getTotalActivity() const {
		return FL_TYPE(0.0);
	}

	virtual matching::InjRandContainer& getInjContainer(int cc_id) {
		throw invalid_argument("SimContext::getInjContainer(): invalid context.");
	}
	virtual const matching::InjRandContainer& getInjContainer(int cc_id) const {
		throw invalid_argument("SimContext::getInjContainer(): invalid context.");
	}
	virtual FL_TYPE getTokenValue(unsigned id) const {
		throw invalid_argument("SimContext::getTokenValue(): invalid context.");
	}

	const SimContext& getParentContext() const {
		return *parent;
	}
};




inline const expressions::AuxMap& SimContext::getAuxMap() const {
#ifdef DEBUG
	if(!auxMap)
		throw std::invalid_argument("SimContext::getAuxMap(): attribute is null.");
#endif
	return *auxMap;
}


inline const expressions::AuxIntMap& SimContext::getAuxIntMap() const {
#ifdef DEBUG
	if(!auxIntMap)
		throw std::invalid_argument("SimContext::getAuxIntMap(): attribute is null.");
#endif
	return *auxIntMap;
}







}

#endif /* SRC_SIMULATION_SIMCONTEXT_H_ */
