/*
 * NullaryOperation.cpp
 *
 *  Created on: Jan 15, 2019
 *      Author: lukas
 */

#include "NullaryOperation.h"
#include "../state/State.h"
#include "../simulation/Simulation.h"
#include <math.h>


namespace expressions {

/***********************************************/
/************ NullaryOperations ******************/
/***********************************************/


std::string n_ops[] = {"RAND_1","SIM_TIME","CPUTIME","ACTIVITY","RUN_ID","RUNS_COUNT"
		"SIM_EVENT","NULL_EVENT","PROD_EVENT","END_SIM"};

template<>
FL_TYPE (*NullaryOperations<FL_TYPE>::funcs[4])(const SimContext&)= {
	[](const SimContext& context) {return uniform_real_distribution<FL_TYPE>(0.0,1.0)(context.getRandomGenerator());},
	[](const SimContext& context) {return context.getCounter().getTime();},
	[](const SimContext& context) {return /*TODO cpu_time*/0.0;},
	[](const SimContext& context) {return context.getTotalActivity();}
};

template<>
int (*NullaryOperations<int>::funcs[5])(const SimContext& context)= {
	[](const SimContext& context) {return int(context.getId());},
	[](const SimContext& context) {return int(context.params->runs);},
	[](const SimContext& context) {return int(context.getCounter().getEvent());},
	[](const SimContext& context) {return int(context.getCounter().getNullEvent());},
	[](const SimContext& context) {return int(context.getCounter().getProdEvent());}
};
template<>
bool (*NullaryOperations<bool>::funcs[4])(const SimContext& context)= {
	[](const SimContext& context) {return context.isDone();}
};

template<typename R>
R NullaryOperation<R>::evaluate(const SimContext& context) const {
	return func(context);
}
template<typename R>
R NullaryOperation<R>::evaluateSafe(const SimContext& context) const {
	return func(context);
}

template<typename R>
NullaryOperation<R>::NullaryOperation(const short op):
		op(op) {
	func = NullaryOperations<R>::funcs[op];
}

template<typename R>
NullaryOperation<R>::~NullaryOperation() {}

template<typename R>
FL_TYPE NullaryOperation<R>::auxFactors(
		std::unordered_map<std::string, FL_TYPE> &var_factors) const {return 0.0;}

template<typename R>
BaseExpression::Reduction NullaryOperation<R>::factorize(const std::map<std::string,small_id> &aux_cc) const {
	BaseExpression::Reduction red;
	if(op == BaseExpression::RUN_ID)
		red.factor = this->clone();
	else
		throw BaseExpression::Unfactorizable("Cannot factorize: operation "+n_ops[op]+" is not factorizable.");
	return red;
}

template <typename T>
BaseExpression* NullaryOperation<T>::reduce(simulation::SimContext &context) {
	return this;
}

template<typename R>
BaseExpression* NullaryOperation<R>::clone() const {
	return new NullaryOperation<R>(*this);
}

template<typename R>
bool NullaryOperation<R>::operator==(const BaseExpression& exp) const {
	auto nl_exp = dynamic_cast<const NullaryOperation<R>*>(&exp);
	if(nl_exp)
		return nl_exp->op == op;
	return false;
}



template <typename R>
char NullaryOperation<R>::getVarDeps() const{
	switch(op){
	case BaseExpression::END_SIM:
	case BaseExpression::SIM_TIME:
	case BaseExpression::CPUTIME:
	case BaseExpression::ACTIVITY:
		return BaseExpression::TIME;
	case BaseExpression::NULL_EVENT:
	case BaseExpression::SIM_EVENT:
	case BaseExpression::PROD_EVENT:
		return BaseExpression::EVENT;
	case BaseExpression::RAND_1:
		return BaseExpression::RAND;
	case BaseExpression::RUN_ID:
		return BaseExpression::RUN;
	}
	return '\0';
}


template<typename R>
std::string NullaryOperation<R>::toString() const {
	int op_txt = op % (sizeof(NullaryOperations<FL_TYPE>::funcs)+sizeof(NullaryOperations<int>::funcs))
		% sizeof(NullaryOperations<FL_TYPE>::funcs);
	return "(" + n_ops[(int)op_txt] +")";
}

/*template<bool, typename T1, typename T2>
std::string UnaryOperation<bool,T1,T2>::toString() const {
	return exp1->toString() + BoolOpChar[op] + exp2->toString();
}*/

template class NullaryOperation<FL_TYPE> ;
template class NullaryOperation<int> ;
template class NullaryOperation<bool> ;


} /* namespace expression */









