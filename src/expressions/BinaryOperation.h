/*
 * BinaryOperation.h
 *
 *  Created on: Oct 29, 2018
 *      Author: naxo100
 */

#ifndef SRC_EXPRESSIONS_BINARYOPERATION_H_
#define SRC_EXPRESSIONS_BINARYOPERATION_H_

#include "AlgExpression.h"
#include <vector>


namespace simulation {
	class Perturbation;
}
namespace expressions {


template<typename R, typename T1, typename T2>
struct BinaryOperations {
	static R (*funcs[])(T1, T2);
	static R (*funcs_safe[])(T1, T2);
};

template<typename T1, typename T2>
struct BinaryOperations<bool, T1, T2> {
	static bool (*funcs[])(T1, T2);
	static bool (*funcs_safe[])(T1, T2);
};


template<typename R, typename T1, typename T2>
class BinaryOperation: public AlgExpression<R> {
	AlgExpression<T1>* exp1;
	AlgExpression<T2>* exp2;
	//static R (*operations[]) (T1,T2);
	R (*func)(T1, T2);
	typedef typename std::conditional<std::is_same<bool,R>::value,
		BaseExpression::BoolOp,BaseExpression::AlgebraicOp>::type Op_Type;
	Op_Type op;

	friend class simulation::Perturbation;
public:
	R evaluate(const SimContext& args) const override;
	R evaluateSafe(const SimContext& args) const override;
	FL_TYPE auxFactors(std::unordered_map<std::string, FL_TYPE> &factor) const
			override;
	BaseExpression::Reduction factorize(const std::map<std::string,small_id> &aux_cc) const override;
	BaseExpression* clone() const override;

	BaseExpression* reduce(SimContext& args);

	//std::set<std::string> getAuxiliars() const override;
	bool operator==(const BaseExpression& exp) const override;
	~BinaryOperation();
	BinaryOperation(BaseExpression *ex1, BaseExpression *ex2,
			Op_Type op);

	void getNeutralAuxMap(
			std::unordered_map<std::string, FL_TYPE>& aux_map) const;

	//return a flag of VarDep
	char getVarDeps() const override;

	std::string toString() const override;
};

} /* namespace expression */

#endif /* SRC_EXPRESSIONS_BINARYOPERATION_H_ */
