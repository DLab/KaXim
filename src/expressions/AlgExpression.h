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
	virtual T evaluate(const EvalArguments<true>& args) const = 0;
	virtual T evaluate(const EvalArguments<false>& args) const = 0;
	virtual FL_TYPE auxFactors(
			std::unordered_map<std::string, FL_TYPE> &factor) const override = 0;
	virtual SomeValue getValue(const EvalArguments<true>& args) const override;
	virtual SomeValue getValue(const EvalArguments<false>& args) const override;
	virtual bool operator==(const BaseExpression& exp) const override = 0;

	//virtual void getNeutralAuxMap(
	//		std::unordered_map<std::string, FL_TYPE>& aux_map) const = 0;
	//virtual Reduction reduce(const state::State& state,const AuxMap&& aux_values = AuxMap()) const;
	//virtual T auxPoly(std::list<AlgExpression<T>>& poly) const = 0;
};

} /* namespace expression */


#endif /* STATE_ALGEBRAICEXPRESSION_H_ */

