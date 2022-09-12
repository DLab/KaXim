/*
 * AlgExpression.h
 *
 *  Created on: Jul 22, 2016
 *      Author: naxo
 */

#ifndef STATE_ALGEXPRESSION_H_
#define STATE_ALGEXPRESSION_H_

#include <unordered_map>
#include <vector>
#include <list>
#include <string>
#include <set>
#include "BaseExpression.h"
//#include "../util/params.h"

/** \brief The family of classes to evaluate algebraic and boolean expressions. */

namespace expressions {

template<typename T>
class AlgExpression: public virtual BaseExpression {
public:
	AlgExpression();
	virtual ~AlgExpression() = 0;
	virtual T evaluate(const SimContext& context) const = 0;
	virtual T evaluateSafe(const SimContext& context) const = 0;
	virtual FL_TYPE auxFactors(
			std::unordered_map<std::string, FL_TYPE> &factor) const override = 0;
	inline virtual SomeValue getValue(const SimContext& context) const override;
	inline virtual SomeValue getValueSafe(const SimContext& context) const override;
	virtual bool operator==(const BaseExpression& exp) const override = 0;

	virtual void update(SomeValue val,AlgExpression<T>** _this);
	virtual void add(SomeValue val) {
		throw std::invalid_argument("invalid call to AlgExpression::add(). Maybe trying to tok-update a normal var?");
	}

	//virtual void getNeutralAuxMap(
	//		std::unordered_map<std::string, FL_TYPE>& aux_map) const = 0;
	//virtual Reduction reduce(const state::State& state,const AuxMap&& aux_values = AuxMap()) const;
	//virtual T auxPoly(std::list<AlgExpression<T>>& poly) const = 0;
};


template<typename T>
SomeValue AlgExpression<T>::getValue(const SimContext& context) const {
	return SomeValue(this->evaluate(context));
}
template<typename T>
SomeValue AlgExpression<T>::getValueSafe(const SimContext& context) const {
	return SomeValue(this->evaluateSafe(context));
}

template class AlgExpression<FL_TYPE> ;
template class AlgExpression<int> ;
template class AlgExpression<bool> ;


} /* namespace expression */


#endif /* STATE_ALGEBRAICEXPRESSION_H_ */

