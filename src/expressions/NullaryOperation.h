/*
 * NullaryOperation.h
 *
 *  Created on: Jan 15, 2019
 *      Author: lukas
 */

#ifndef SRC_EXPRESSIONS_NULLARYOPERATION_H_
#define SRC_EXPRESSIONS_NULLARYOPERATION_H_

#include "AlgExpression.h"


namespace expressions {


template<typename R>
struct NullaryOperations {
	static R (*funcs[])(const SimContext&);
};


template<typename R>
class NullaryOperation: public AlgExpression<R> {
	R (*func)(const SimContext&);
	const char op;
public:
	R evaluate(const SimContext& args) const override;
	R evaluateSafe(const SimContext& args) const override;
	FL_TYPE auxFactors(std::unordered_map<std::string, FL_TYPE> &factor) const
				override;
	BaseExpression::Reduction factorize(const std::map<std::string,small_id> &aux_cc) const override;
	BaseExpression* reduce(SimContext& context) override;
	BaseExpression* clone() const override;
	//std::set<std::string> getAuxiliars() const override;
	bool operator==(const BaseExpression& exp) const override;
	~NullaryOperation();
	NullaryOperation(const short op);


	char getVarDeps() const override;

	std::string toString() const override;
};

} /* namespace expression */



#endif /* SRC_EXPRESSIONS_NULLARYOPERATION_H_ */
