/*
 * UnaryOperation.h
 *
 *  Created on: Jan 15, 2019
 *      Author: lukas
 */

#ifndef SRC_EXPRESSIONS_UNARYOPERATION_H_
#define SRC_EXPRESSIONS_UNARYOPERATION_H_

#include "AlgExpression.h"


namespace expressions {


template<typename R, typename T>
struct UnaryOperations {
	static R (*funcs[])(T);
};


template<typename R, typename T>
class UnaryOperation: public AlgExpression<R> {
	AlgExpression<T>* exp;
	R (*func)(T);
	const char op;
public:
	R evaluate(const SimContext<true>& args) const override;
	R evaluate(const SimContext<false>& args) const override;
	FL_TYPE auxFactors(std::unordered_map<std::string, FL_TYPE> &factor) const
				override;
	//std::set<std::string> getAuxiliars() const override;
	BaseExpression::Reduction factorize(const std::map<std::string,small_id> &aux_cc) const override;
	BaseExpression* reduce(VarVector &vars) override;
	BaseExpression* clone() const override;
	bool operator==(const BaseExpression& exp) const override;
	virtual ~UnaryOperation();
	UnaryOperation(BaseExpression *ex,
			const short op);

	virtual char getVarDeps() const override;

	virtual std::string toString() const override;
};

} /* namespace expression */



#endif /* SRC_EXPRESSIONS_UNARYOPERATION_H_ */
