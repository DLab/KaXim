/*
 * UnaryOperation.cpp
 *
 *  Created on: Jan 15, 2019
 *      Author: lukas
 */

#include "UnaryOperation.h"
#include "../state/Variable.h"
#include <math.h>       /* sqrt, exp, log, sin, cos, tan, atan, abs */

#include <iostream>
#include <typeinfo>
namespace expressions {

/***********************************************/
/************ UnaryOperations ******************/
/***********************************************/

std::string ops[] = {"sqrt", "exp", "log", "sin", "cos", "tan",
		"atan", "abs", "next", "prev", "coin", "rand_n"};

template<typename R, typename T>
R (*UnaryOperations<R, T>::funcs[])(T)= {
	[](T v) {return (R)std::sqrt(v);}, // SQRT
	[](T v) {return (R)std::exp(v);}, // EXPONENT
	[](T v) {return (R)std::log(v);}, // LOG
	[](T v) {return (R)std::sin(v);}, // SINE
	[](T v) {return (R)std::cos(v);}, // COSINE
	[](T v) {return (R)std::tan(v);}, // TAN
	[](T v) {return (R)std::atan(v);}, // ATAN
	[](T v) {return (R)std::abs(v);}, // ABS
	[](T v) {return (R)std::nextafter(v,std::numeric_limits<float>::infinity());}, // NEXT
	[](T v) {return (R)std::nextafter(v,-std::numeric_limits<float>::infinity());}, // PREV
	[](T v) {return ((rand()%10)/10.0) > v ? (R)1 : (R)0;}, // COIN
	[](T v) {return (R)std::fmod(std::rand(), v);}, // RAND_N //TODO use better rand function
	[](T v) {return (R)!v;}
};

template<typename R, typename T>
R (*UnaryOperations<R, T>::funcs_safe[])(T)= {
	[](T v) { // SQRT
		if(v < 0)
			throw std::invalid_argument("Root of negative not defined.");
		return (R)std::sqrt(v);
	},[](T v) {return (R)std::exp(v);}, // EXPONENT
	[](T v) {return (R)std::log(v);}, // LOG
	[](T v) {return (R)std::sin(v);}, // SINE
	[](T v) {return (R)std::cos(v);}, // COSINE
	[](T v) {return (R)std::tan(v);}, // TAN
	[](T v) {return (R)std::atan(v);}, // ATAN
	[](T v) {return (R)std::abs(v);}, // ABS
	[](T v) {return (R)std::nextafter(v,std::numeric_limits<float>::infinity());}, // NEXT
	[](T v) {return (R)std::nextafter(v,-std::numeric_limits<float>::infinity());}, // PREV
	[](T v) {return ((rand()%10)/10.0) > v ? (R)1 : (R)0;}, // COIN
	[](T v) {return (R)std::fmod(std::rand(), v);}, // RAND_N //TODO use better rand function
	[](T v) {return (R)!v;}
};

template<typename R, typename T>
R UnaryOperation<R, T>::evaluate(const SimContext& context) const {
	auto a = exp->evaluate(context);
	return func(a);
}
template<typename R, typename T>
R UnaryOperation<R, T>::evaluateSafe(const SimContext& context) const {
	auto a = exp->evaluate(context);
	return func(a);
}

template<typename R, typename T>
UnaryOperation<R, T>::UnaryOperation(BaseExpression *ex,
		const short op) :
		op(op) {
	exp = dynamic_cast<AlgExpression<T>*>(ex);
	if(!exp)
		throw std::invalid_argument("Cannot create UnaryOperation: argument type mismatch.");
	func = UnaryOperations<R, T>::funcs[op];
}

template<typename R, typename T>
UnaryOperation<R, T>::~UnaryOperation() {
	if (!dynamic_cast<const state::Variable*>(exp))
		1;//delete exp;TODO
}

template<typename R, typename T>
FL_TYPE UnaryOperation<R, T>::auxFactors(
		std::unordered_map<std::string, FL_TYPE> &var_factors) const {return 0.0;}

template<typename R, typename T>
BaseExpression::Reduction UnaryOperation<R, T>::factorize(const std::map<std::string,small_id> &aux_cc) const {
	using Unfactorizable = BaseExpression::Unfactorizable;
	auto VARDEP = BaseExpression::VarDep::VARDEP;
	auto MULT = BaseExpression::AlgebraicOp::MULT;
	auto make_binary = BaseExpression::makeBinaryExpression<false>;
	auto make_unary = BaseExpression::makeUnaryExpression;
	BaseExpression::Reduction res;
	BaseExpression::Reduction r(exp->factorize(aux_cc));
	switch(op){
	case BaseExpression::SQRT:
		res.factor = *r.factor == *ONE_FL_EXPR ?
				make_unary(r.factor,op) :
				r.factor;
		for(auto& aux_f : r.aux_functions)
			res.aux_functions[aux_f.first] = make_unary(aux_f.second,op);
		break;
	case BaseExpression::ABS:
		ADD_WARN_NOLOC("Factorizing the function 'absolute value' has unexpected behavior.");
		//no break;
	case BaseExpression::EXPONENT:
	case BaseExpression::LOG:
	case BaseExpression::SINE:
	case BaseExpression::COSINE:
	case BaseExpression::TAN:
	case BaseExpression::ATAN:
		if(r.aux_functions.size() > 1)
			throw Unfactorizable("Cannot factorize: Applying "+ops[op]+" to more than one cc-aux function.");
		if(r.aux_functions.size() == 1){
			auto& aux_f = *r.aux_functions.begin();
			if(*r.factor == *ONE_FL_EXPR){
				res.factor = r.factor;
				res.aux_functions[aux_f.first] = make_unary(aux_f.second,op);
			}
			else{
				res.factor = ONE_FL_EXPR->clone();
				res.aux_functions[aux_f.first] = make_unary(
						make_binary(r.factor,aux_f.second,MULT),op);
			}
			if(r.factor->getVarDeps() & VARDEP)
				ADD_WARN_NOLOC("Rate factorization will depend on "+ops[op]+"(aux,var) and this will lead to inexact results.");
		}
		else
			res.factor = make_unary(r.factor,op);
		break;
	case BaseExpression::COIN:
	case BaseExpression::RAND_N:
	case BaseExpression::NOT:
	default:
		throw Unfactorizable("Cannot factorize: Applying "+ops[op]+" is not possible.");
	}

	return res;
}

template <typename R, typename T>
BaseExpression* UnaryOperation<R,T>::reduce(SimContext& context){
	if(op >= BaseExpression::COIN)//random constant
		return this;
	auto r = exp->reduce(context);
	auto cons_r = dynamic_cast<Constant<T>*>(r);
	if(cons_r){
		delete exp;
		return new Constant<R>(func(cons_r->evaluate(context)));
	}
	return this;
}

template<typename R, typename T>
BaseExpression* UnaryOperation<R, T>::clone() const {
	return new UnaryOperation<R, T>(*this);
}

template<typename R, typename T>
bool UnaryOperation<R, T>::operator==(const BaseExpression& expr) const {
	try {
		auto& unary_exp = dynamic_cast<const UnaryOperation<R, T>&>(expr);
		if (unary_exp.op == op)
			return (*unary_exp.exp) == *exp;
	} catch (std::bad_cast &ex) {
	}
	return false;
}

template <typename R,typename T>
char UnaryOperation<R,T>::getVarDeps() const {
	return exp->getVarDeps();
}


template<typename R, typename T>
std::string UnaryOperation<R,T>::toString() const {
	return ops[(int)op] + "(" + exp->toString() +")";
}



template class UnaryOperation<int, int> ;

template class UnaryOperation<FL_TYPE, FL_TYPE> ;
template class UnaryOperation<FL_TYPE,int>;

//template class UnaryOperation<bool, bool> ;

} /* namespace expression */
