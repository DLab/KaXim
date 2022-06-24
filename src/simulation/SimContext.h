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
#include "Parameters.h"
#include "../state/Variable.h"
#include "../pattern/Environment.h"
//#include "../data_structs/ValueMap.h"

namespace expressions {
template <typename T> class Auxiliar;
}

namespace matching {
template <typename T>
class InjRandContainer;
class CcInjection;
class MixInjection;
}

typedef matching::InjRandContainer<matching::CcInjection> CcInjRandContainer;
typedef matching::InjRandContainer<matching::MixInjection> MixInjRandContainer;

namespace simulation {

using namespace std;

class SimContext {
protected:
	int id;
public:
	const Parameters* params;
protected:
	pattern::Environment &env;
	VarVector vars;
	Counter& counter;
	//bool safe;	///< set to true when next evaluation will be safe.


	mutable RNG rng;
	mutable const expressions::AuxMap* auxMap;
	mutable const expressions::AuxIntMap* auxIntMap;

	mutable CcEmbMap<expressions::Auxiliar,FL_TYPE,state::Node> cc_map;

	mutable NamesMap<expressions::Auxiliar,int> int_map;

	SimContext* parent;

public:
	SimContext(pattern::Environment& _env, Parameters* _params) :
			id(-1),params(_params),env(_env),counter(*new GlobalCounter()),
			rng(_params->seed),auxMap(nullptr),
			auxIntMap(&int_map),parent(nullptr) {	}
	SimContext(int _id,SimContext* parent_context,Counter* cntr = nullptr) :
			id(_id),params(parent_context->params),
			env(parent_context->getEnv()),
			vars(parent_context->getVars()),
			counter(cntr? *cntr : *new LocalCounter(parent_context->getCounter()) ),
			auxMap(nullptr),auxIntMap(nullptr),
			parent(parent_context) {
		if(parent){
			rng.seed(uniform_int_distribution<long>()(parent_context->getRandomGenerator()));
		}
		else
			throw invalid_argument("SimContext cannot be constructed with a null parent.");
		auxIntMap = &int_map;
	}
	//SimContext(SimContext* parent_context) : parent(parent_context) {}
	virtual ~SimContext() {
		if(!id && !parent){
			delete params;
			delete &env;//TODO its ok?
			delete &counter;
		}
	};

	inline int getId() const {
		return id;
	}

	virtual string getName() const {
		return "SimContext["+to_string(id)+"]";
	}

	inline auto& getParams() const {
		return *params;
	}

	inline const VarVector& getVars() const {
		return vars;
	}
	inline VarVector& getVars() {
		return vars;
	}
	inline SomeValue getVarValue(short_id var_id) const {
		return vars[var_id]->getValue(*this);
	}
	void updateVar(const state::Variable& var,bool by_value){
		//delete vars[var.getId()];
		auto upd_var = vars[var.getId()];
		if(upd_var->isConst())
			ADD_WARN_NOLOC("Updating a const declaration leads to undefined behavior.");
		if(by_value)
			upd_var->update(var.getValue(*this));
		else
			upd_var->update(var);
		//updateDeps(pattern::Dependency(Deps::VAR,var.getId()));
	}

	inline const pattern::Environment& getEnv() const {
			return env;
	}
	inline pattern::Environment& getEnv() {
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
	inline void setAuxMap(const std::vector<state::Node*>& _emb) const {
		cc_map.setEmb(_emb);
		auxMap = &cc_map;
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

	virtual int count(int id) const {
		return 0;
	}

	virtual CcInjRandContainer& getInjContainer(int cc_id) {
		throw invalid_argument("SimContext::getInjContainer(): invalid context.");
	}
	virtual const CcInjRandContainer& getInjContainer(int cc_id) const {
		throw invalid_argument("SimContext::getInjContainer(): invalid context.");
	}
	virtual MixInjRandContainer& getMixInjContainer(int cc_id) {
		throw invalid_argument("SimContext::getMixInjContainer(): invalid context.");
	}
	virtual const MixInjRandContainer& getMixInjContainer(int cc_id) const {
		throw invalid_argument("SimContext::getMixInjContainer(): invalid context.");
	}
	virtual FL_TYPE getTokenValue(unsigned id) const {
		throw invalid_argument("SimContext::getTokenValue(): invalid context.");
	}
	virtual void setTokenValue(unsigned id,FL_TYPE val) const {
		throw invalid_argument("SimContext::getTokenValue(): invalid context.");
	}


	const SimContext& getParentContext() const {
		return *parent;
	}

	mutable string log_msg;
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
