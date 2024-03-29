/*
 * Variable.cpp
 *
 *  Created on: Apr 29, 2016
 *      Author: naxo
 */

#include "Variable.h"
#include "State.h"
#include "../pattern/mixture/Component.h"
#include "../expressions/Vars.h"


namespace state {

/************ Variable *****************/

Variable::Variable
	(const short var_id, const std::string &nme,const bool is_obs):
		id(var_id),name(nme),isObservable(is_obs),updated(false){}
Variable::~Variable(){}

bool Variable::isConst() const {
	return false;
}

short Variable::getId() const {
	return id;
}

void Variable::update(SomeValue val){
	throw std::invalid_argument("update(SomeValue) can only be called in AlebraicVar");
}

Variable* Variable::makeAlgVar(short id, const string& name, BaseExpression *expr){
	Variable* var = nullptr;
	switch(expr->getType()){
	case FLOAT:
		var = new state::AlgebraicVar<FL_TYPE>(id,name,false,
			dynamic_cast<AlgExpression<FL_TYPE>*>(expr));
		break;
	case INT:
		var = new state::AlgebraicVar<int>(id,name,false,
			dynamic_cast<AlgExpression<int>*>(expr));
		break;
	case BOOL:
		var = new state::AlgebraicVar<bool>(id,name,false,
			dynamic_cast<AlgExpression<bool>*>(expr));
		break;
	case NONE:
	default:
		throw invalid_argument("Cannot return None value.");
	}
	return var;
}

//const std::string& Variable::getName() const {return name;};
std::string Variable::toString() const {return name;};

/*****************************************/
/******* class AlgebraicVar **************/
/*****************************************/
template <typename T>
AlgebraicVar<T>::AlgebraicVar(const short var_id, const std::string &nme,
		const bool is_obs,AlgExpression<T> *exp):
		BaseExpression(), Variable(var_id,nme,is_obs),expression(exp) {}

template <typename T>
AlgebraicVar<T>::~AlgebraicVar(){
	if(!dynamic_cast<const state::Variable*>(expression))
		delete expression;
}

template <typename T>
void AlgebraicVar<T>::update(const Variable& var){
	try{
		auto& alg_var = dynamic_cast<const AlgebraicVar<T>&>(var);
		delete expression; //TODO
		expression = dynamic_cast<AlgExpression<T>*>(alg_var.expression->clone());
	}
	catch(bad_cast &ex){
		throw invalid_argument("Cannot update an AlgebraicVar to another type of expression (try to change to Int/Float).");
	}
}

template <typename T>
void AlgebraicVar<T>::update(SomeValue val){
	delete expression; //TODO
	expression = new Constant<T>(val.valueAs<T>());
}

template <typename T>
T AlgebraicVar<T>::evaluate(const SimContext& context) const{
	return expression->evaluate(context);
}
template <typename T>
T AlgebraicVar<T>::evaluateSafe(const SimContext& context) const{
	return expression->evaluateSafe(context);
}
template <typename T>
FL_TYPE AlgebraicVar<T>::auxFactors(std::unordered_map<std::string,FL_TYPE> &aux_values) const{
	return expression->auxFactors(aux_values);
}
template <typename T>
BaseExpression::Reduction AlgebraicVar<T>::factorize(const std::map<std::string,small_id> &aux_cc) const {
	BaseExpression::Reduction r;
	r.factor = this->clone();
	return r;
}

template <typename T>
BaseExpression* AlgebraicVar<T>::reduce(SimContext& context) {
	auto r = expression->reduce(context);
	if(expression != r)
		delete expression;
	expression = dynamic_cast<AlgExpression<T>*>(r);
	return this;
}

template <typename T>
BaseExpression* AlgebraicVar<T>::clone() const {
	return new AlgebraicVar<T>(id,name,isObservable,dynamic_cast<AlgExpression<T>*>(expression->clone()));
}

template <typename T>
BaseExpression* AlgebraicVar<T>::makeVarLabel() const{
	return new VarLabel<T>(id,name);
}

template <typename T>
bool AlgebraicVar<T>::operator==(const BaseExpression& exp) const {
	return *expression == exp;
}


template class AlgebraicVar<FL_TYPE>;
template class AlgebraicVar<int>;
template class AlgebraicVar<bool>;


/******* class ConstantVar *************/
template <typename T>
ConstantVar<T>::ConstantVar(const short var_id, const std::string &nme,T value):
		Variable(var_id,nme),Constant<T>(value) {}

template <typename T>
void ConstantVar<T>::update(const Variable& var){
	throw invalid_argument("Cannot update a ConstantVar.");
}

template <typename T>
bool ConstantVar<T>::isConst() const {
	return true;
}

template <typename T>
BaseExpression* ConstantVar<T>::reduce(SimContext& context) {
	return this;
}
template <typename T>
BaseExpression* ConstantVar<T>::clone() const {
	return new ConstantVar<T>(*this);
}

template <typename T>
BaseExpression* ConstantVar<T>::makeVarLabel() const{
	return new VarLabel<T>(id,name);
}

template <typename T>
std::string ConstantVar<T>::toString() const {
	return "const("+Constant<T>::toString()+")";
}
/*
template <typename T>
T ConstantVar<T>::evaluate(const std::unordered_map<std::string,int> *aux_values) const{
	return val;
}
template <typename T>
T ConstantVar<T>::evaluate(const state::State& state,const AuxMap& aux_values) const{
	return val;
}
template <typename T>
FL_TYPE ConstantVar<T>::auxFactors(std::unordered_map<std::string,FL_TYPE> &aux_values) const{
	return val;
}*/

template class ConstantVar<FL_TYPE>;
template class ConstantVar<int>;
template class ConstantVar<bool>;



/******* class KappaVar ****************/
KappaVar::KappaVar(const short id,const std::string &nme,const bool is_obs,
		const pattern::Mixture &kappa) :
				AlgExpression<int>(),
				Variable(id,nme,is_obs),
				mixture(&kappa) {}


void KappaVar::update(const Variable& var){
	try{
		auto kappa_var = dynamic_cast<const KappaVar&>(var);
		mixture = kappa_var.mixture;
	}
	catch(bad_cast &ex){
		throw invalid_argument("Cannot update a KappaVar to another type of var.");
	}
}

//TODO
FL_TYPE KappaVar::auxFactors(std::unordered_map<std::string,FL_TYPE> &factor) const {
	return 0;
}

BaseExpression::Reduction KappaVar::factorize(const std::map<std::string,small_id> &aux_cc) const {
	BaseExpression::Reduction r;
	r.factor = this->clone();
	return r;
}

BaseExpression* KappaVar::reduce(SimContext& context) {
	return this;
}

BaseExpression* KappaVar::makeVarLabel() const{
	return new VarLabel<int>(id,name);
}

BaseExpression* KappaVar::clone() const {
	return new KappaVar(*this);
}
int KappaVar::evaluate(const SimContext& context) const {
	UINT_TYPE  count = 1;
	for(const auto cc : *mixture){
		count *= context.getInjContainer(cc->getId()).count();
	}
	return count;
}
int KappaVar::evaluateSafe(const SimContext& context) const {
	UINT_TYPE  count = 1;
	for(const auto cc : *mixture){
		count *= context.getInjContainer(cc->getId()).count();
	}
	return count;
}

const pattern::Mixture& KappaVar::getMix() const {
	return *mixture;
}

bool KappaVar::operator==(const BaseExpression& exp) const {
	try{
		auto& kappa_exp = dynamic_cast<const KappaVar&>(exp);
		return kappa_exp.mixture == mixture;
	}
	catch(bad_cast &ex){	}
	return false;
}



/******* class DistributionVar ****************/

template <typename T>
DistributionVar<T>::DistributionVar(const short id,const std::string &nme,const bool is_obs,
		const pattern::Mixture &kappa,const pair<N_ary,const BaseExpression*>& exp) :
				AlgExpression<T>(),Variable(id,nme,is_obs),
				mixture(&kappa),op(exp.first),auxFunc(exp.second) {
	for(auto aux : mixture->getAuxMap())
		//check if aux is in expr
		auxMap[aux.first] = make_pair(aux.second.ag_pos,aux.second.st_id);
}

template <typename T>
DistributionVar<T>::~DistributionVar(){
	if(auxFunc && !dynamic_cast<const state::Variable*>(auxFunc))
		delete auxFunc;
	//else todo***
}

template <typename T>
void DistributionVar<T>::update(const Variable& var){
	try{
		auto distr_var = dynamic_cast<const DistributionVar<T>&>(var);
		delete auxFunc;
		mixture = distr_var.mixture;
		auxFunc = distr_var.auxFunc;
	}
	catch(bad_cast &ex){
		throw invalid_argument("Cannot update an AlgebraicVar to another type of expression (try to change to Int/Float).");
	}
}

template <typename T>
FL_TYPE DistributionVar<T>::auxFactors(std::unordered_map<std::string,FL_TYPE> &factor) const {
	throw std::invalid_argument("Cannot call DistributionVar::auxFactors().");
}

template <typename T>
BaseExpression::Reduction DistributionVar<T>::factorize(const std::map<std::string,small_id> &aux_cc) const {
	BaseExpression::Reduction r;
	r.factor = this->clone();
	return r;
}

template <typename T>
BaseExpression* DistributionVar<T>::reduce(SimContext& context) {
	return this;
}

template <typename T>
BaseExpression* DistributionVar<T>::clone() const {
	return new DistributionVar(id,name,isObservable,*mixture,make_pair(op,auxFunc->clone()));
}
template <typename T>
T DistributionVar<T>::evaluate(const SimContext& context) const {
	auto& injs = context.getInjContainer(mixture->getComponent(0).getId());
	return injs.sumInternal(auxFunc,context)
			/ (op? injs.count() : 1);
	//throw invalid_argument("DistributionVar::evaluate(): invalid");
}
template <typename T>
T DistributionVar<T>::evaluateSafe(const SimContext& context) const {
	auto& injs = context.getInjContainer(mixture->getComponent(0).getId());
	return injs.sumInternal(auxFunc,context)
			/ (op? injs.count() : 1);
}

template <typename T>
BaseExpression* DistributionVar<T>::makeVarLabel() const{
	return new VarLabel<T>(id,name);
}

template <typename T>
const pattern::Mixture& DistributionVar<T>::getMix() const {
	return *mixture;
}

template <typename T>
bool DistributionVar<T>::operator==(const BaseExpression& exp) const {
	try{
		auto& kappa_exp = dynamic_cast<const DistributionVar&>(exp);
		return kappa_exp.mixture == mixture;
	}
	catch(bad_cast &ex){	}
	return false;
}

template class DistributionVar<int>;
template class DistributionVar<FL_TYPE>;
/******* class RateVar ****************/
/*RateVar::RateVar(const short id,const std::string &nme,const bool is_obs,
		const AlgExpression<FL_TYPE> *exp ) :
				AlgExpression<FL_TYPE>(),
				Variable(id,nme,is_obs),
				expression(exp) {}

//TODO
FL_TYPE RateVar::auxFactors(std::unordered_map<std::string,FL_TYPE> &factor) const {
	return 0;
}

FL_TYPE RateVar::evaluate(const VarVector& consts,const unordered_map<string,int> *aux_values) const {
	throw std::invalid_argument("Cannot call RateVar::evaluate() without state.");
}
FL_TYPE RateVar::evaluate(const state::State& state,const AuxMap& aux_values) const {
	if(aux_values.size())
		return expression->evaluate(state,aux_values);
	else{
		FL_TYPE a = 1.0;
		for(auto dep : cc_deps)
			a *= state.injections[dep]->count();
		return a;
	}
}

bool RateVar::operator ==(const BaseExpression& exp) const {
	try{
		auto& rate_exp = dynamic_cast<const RateVar&>(exp);
		return *(rate_exp.expression) == *expression;
	}
	catch(bad_cast &ex){	}
	return false;
}
*/

/**************************/
/****** class Token *******/
/**************************/

TokenVar::TokenVar(unsigned _id) :
		id(_id) {
}
FL_TYPE TokenVar::evaluate(const SimContext& args) const {
	//throw invalid_argument("todo: TokenVar::evaluate");
	return args.getTokenValue(id);
}
FL_TYPE TokenVar::evaluateSafe(const SimContext& args) const {
	return args.getTokenValue(id);
	//throw invalid_argument("todo: TokenVar::evaluate");
}
FL_TYPE TokenVar::auxFactors(
		std::unordered_map<std::string, FL_TYPE> &factor) const {
	throw std::invalid_argument("TokenVar::auxFactors():Cannot use tokens in this expression.");
}

bool TokenVar::operator==(const BaseExpression& exp) const {
	try {
		auto& tok_exp = dynamic_cast<const TokenVar&>(exp);
		return tok_exp.id == id;
	} catch (std::bad_cast &ex) {
	}
	return false;
}

BaseExpression::Reduction TokenVar::factorize(const std::map<std::string,small_id> &aux_cc) const {
	BaseExpression::Reduction r;
	r.factor = this->clone();
	return r;
}

BaseExpression* TokenVar::reduce(SimContext& context) {
	return this;
}

BaseExpression* TokenVar::clone() const {
	return new TokenVar(*this);
}

} /* namespace state */
