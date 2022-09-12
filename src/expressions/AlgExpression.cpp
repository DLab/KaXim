/*
 * AlgExpression.cpp
 *
 *  Created on: Jul 22, 2016
 *      Author: naxo
 */

#include "../expressions/AlgExpression.h"

#include <type_traits>	//std::conditional
#include "../state/State.h"
#include "AlgExpression.h"

namespace expressions {

//template <> struct BaseExpression::TypeDef<BaseExpression::INT> { typedef int t;};
//template <> struct BaseExpression::TypeDef<BaseExpression::BOOL> { typedef bool t;};




/*************************************/
/********** AlgExpression ************/
/*************************************/
template<typename T>
AlgExpression<T>::AlgExpression() {
	if (std::is_same<T, FL_TYPE>::value)
		t = FLOAT;
	else if (std::is_same<T, int>::value)
		t = INT;
	else if (std::is_same<T, bool>::value)
		t = BOOL;
	else
		throw std::invalid_argument(
				"Algebraic Expression can only be float, int or bool.");
}
template<typename T>
AlgExpression<T>::~AlgExpression() {}

template<typename T>
void AlgExpression<T>::update(SomeValue val,AlgExpression<T>** _this) {
	delete this;
	*_this = new Constant<T>(val.valueAs<T>());
	//throw std::invalid_argument("invalid call to AlgExpression::update(). Maybe trying to tok-update a normal var?");
}


} /* namespace ast */

