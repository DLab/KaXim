/*
 * Function.h
 *
 *  Created on: Dec 30, 2019
 *      Author: naxo100
 */

#ifndef SRC_EXPRESSIONS_FUNCTION_H_
#define SRC_EXPRESSIONS_FUNCTION_H_

#include "AlgExpression.h"

#include <vector>
#include <list>


namespace expressions {

using namespace std;

template<typename T>
struct FunctionPointers {
	static T (*funcs[])(const vector<SomeValue>&,const simulation::SimContext&);
};


template <typename T>
class Function : public AlgExpression<T> {
	BaseExpression::Funcs f_name;
	T (*func) (const vector<SomeValue>&,const simulation::SimContext& context);
	list<BaseExpression*> args;
public:
	Function(BaseExpression::Funcs f,const list<BaseExpression*>& l);
	virtual ~Function();

	T evaluate(const SimContext& args) const override;
	T evaluateSafe(const SimContext& args) const override;
	FL_TYPE auxFactors(std::unordered_map<std::string, FL_TYPE> &factor) const
			override;
	BaseExpression::Reduction factorize(const std::map<std::string,small_id> &aux_cc) const override;
	BaseExpression* clone() const override;

	BaseExpression* reduce(SimContext& context);

	//std::set<std::string> getAuxiliars() const override;
	bool operator==(const BaseExpression& exp) const override;

	//return a flag of VarDep
	char getVarDeps() const override;

	string toString() const override;

};

} /* namespace pattern */

#endif /* SRC_EXPRESSIONS_FUNCTION_H_ */
