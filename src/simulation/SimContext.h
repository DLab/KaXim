/*
 * SimContext.h
 *
 *  Created on: Oct 13, 2020
 *      Author: naxo100
 */

#ifndef SRC_SIMULATION_SIMCONTEXT_H_
#define SRC_SIMULATION_SIMCONTEXT_H_

#include "../state/Variable.h"


/** \brief Class used to map aux to values in different ways */
template <typename T>
class AuxValueMap {
public:
	virtual ~AuxValueMap();
	virtual T& operator[](const Auxiliar<T>& a) = 0;
	virtual T at(const Auxiliar<T>& a) const = 0;
	virtual void clear() = 0;
	virtual size_t size() const = 0;
};

typedef AuxValueMap<FL_TYPE> AuxMap;


class SimContext {
protected:
	int id;
	VarVector vars;
public:
	SimContext(VarVector& _vars) : vars(_vars){}
	virtual ~SimContext() {};

	virtual const VarVector& getVars() const {
		return vars;
	}
	virtual VarVector& getVars() {
		return vars;
	}

	inline const AuxMap& getAuxMap() const;
	inline const AuxValueMap<int>& getAuxIntMap() const;
};

class EmptyContext()



#endif /* SRC_SIMULATION_SIMCONTEXT_H_ */
