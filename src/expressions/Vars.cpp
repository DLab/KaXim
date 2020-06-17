/*
 * Vars.cpp
 *
 *  Created on: Nov 12, 2018
 *      Author: naxo100
 */

#include "Vars.h"
#include "Constant.h"
#include <iostream>
#include <typeinfo>
#include "../state/Variable.h"
#include "../state/State.h"
#include "../state/Node.h"
#include "SomeValue.h"
namespace expressions {


/*********** Class Auxiliar ***********/
template<typename R>
Auxiliar<R>::Auxiliar(const std::string &nme) :
	name(nme),coords{small_id(-1),small_id(-1),small_id(-1)} {}
template<typename R>
Auxiliar<R>::Auxiliar(const std::string &nme,
		pattern::Mixture::AuxCoord _coords) :
	name(nme),coords(_coords) {}


template<typename R>
Auxiliar<R>::~Auxiliar() {}

template<typename R>
R Auxiliar<R>::evaluate(const EvalArguments<true>& args) const {
#ifdef DEBUG
	try {
#endif
	return args.getAuxMap().at(*this);
#ifdef DEBUG
		} catch (std::out_of_range& ex) {
		throw std::out_of_range(
			"Cannot find value for auxiliar "+name+". Try to reorder agent-signature sites if there is dependency.");
	}
#endif
}
template<typename R>
R Auxiliar<R>::evaluate(const EvalArguments<false>& args) const {
#ifdef DEBUG
	try {
#endif
	return args.getAuxMap().at(*this);
#ifdef DEBUG
		} catch (std::out_of_range& ex) {
		throw std::out_of_range(
			"Cannot find value for auxiliar "+name+". Try to reorder agent-signature sites if there is dependency.");
	}
#endif
}

template <>
int Auxiliar<int>::evaluate(const EvalArguments<true>& args) const {
#ifdef DEBUG
	try {
#endif
	return args.getAuxIntMap().at(*this);
#ifdef DEBUG
		} catch (std::out_of_range& ex) {
		throw std::out_of_range(
			"Cannot find value for auxiliar "+name+". Try to reorder agent-signature sites if there is dependency.");
	}
#endif
}
template <>
int Auxiliar<int>::evaluate(const EvalArguments<false>& args) const {
#ifdef DEBUG
	try {
#endif
	return args.getAuxIntMap().at(*this);
#ifdef DEBUG
		} catch (std::out_of_range& ex) {
		throw std::out_of_range(
			"Cannot find value for auxiliar "+name+". Try to reorder agent-signature sites if there is dependency.");
	}
#endif
}

template<typename R>
FL_TYPE Auxiliar<R>::auxFactors(
		std::unordered_map<std::string, FL_TYPE> &var_factors) const {
	var_factors[name] = 1;
	return 0;
}

template<typename R>
BaseExpression::Reduction Auxiliar<R>::factorize(const std::map<std::string,small_id> &aux_cc) const {
	BaseExpression::Reduction r;
	Auxiliar<R>* aux = new Auxiliar<R>(*this);
	r.aux_functions[aux_cc.at(aux->toString())] = aux;
	r.factor = ONE_FL_EXPR->clone();
	return r;
}

template<typename R>
BaseExpression* Auxiliar<R>::clone() const {
	return new Auxiliar<R>(*this);
}

template<typename R>
BaseExpression* Auxiliar<R>::reduce(VarVector &vars) {
	return this;
}

template<typename T>
bool Auxiliar<T>::operator==(const BaseExpression& exp) const {
	try {
		auto& aux_exp = dynamic_cast<const Auxiliar<T>&>(exp);
		if (coords.st_id == small_id(-1))
			return aux_exp.name == name;
		else
			return (aux_exp.coords == coords);
	} catch (std::bad_cast &ex) { }
	return false;
}

template <typename T>
char Auxiliar<T>::getVarDeps() const{
	return BaseExpression::AUX;
}

template <typename T>
bool Auxiliar<T>::isAux() const {
	return true;
}

template <typename T>
std::string Auxiliar<T>::toString() const {
	return name;
}

template class Auxiliar<int> ;
template class Auxiliar<FL_TYPE> ;

/********************************************/
/************** class VarLabel **************/
/********************************************/
template<typename R>
VarLabel<R>::VarLabel(int id,const string &_name) :
		varId(id),name(_name) {}

template<typename R>
R VarLabel<R>::evaluate(const EvalArguments<true>& args) const {
	//throw std::invalid_argument("This should never been used");
	return args.getVars()[varId]->getValue(args).valueAs<R>();
}
template<typename R>
R VarLabel<R>::evaluate(const EvalArguments<false>& args) const {
	//throw std::invalid_argument("This should never been used");
	return args.getVars()[varId]->getValue(args).valueAs<R>();
}

template<typename R>
FL_TYPE VarLabel<R>::auxFactors(
		std::unordered_map<std::string, FL_TYPE> &factor) const {
	throw std::invalid_argument("VarLabel::auxFactor(): This should never been used");
}

template<typename R>
BaseExpression* VarLabel<R>::reduce(VarVector &vars ) {
	EvalArgs args(0,&vars);
	auto cons_var = dynamic_cast<state::ConstantVar<R>*>(vars[varId]);
	if(cons_var)
		return new Constant<R>(cons_var->evaluate(args));
	return vars[varId];//this var should has been reduced
}

template <typename R>
BaseExpression::Reduction VarLabel<R>::factorize(const std::map<std::string,small_id> &aux_cc) const {
	throw invalid_argument("You should have reduced this expression before factorize! (var-name/id: "+name+"/"+to_string(varId));
}
template <typename R>
BaseExpression* VarLabel<R>::clone() const {
	return new VarLabel<R>(*this);
}

template<typename T>
bool VarLabel<T>::operator==(const BaseExpression& exp) const {
	try {
		auto& var_exp = dynamic_cast<const VarLabel<T>&>(exp);
		return var_exp.varId == varId;
	} catch (std::bad_cast &ex) {
	}
	return false;
}

template <typename T>
char VarLabel<T>::getVarDeps() const{
	return BaseExpression::VARDEP;
}

template <typename T>
std::string VarLabel<T>::toString() const{
	return name;
}

template class VarLabel<int> ;
template class VarLabel<FL_TYPE> ;
template class VarLabel<bool> ;


/************** AuxMaps ******************************/

template <typename T>
AuxValueMap<T>::~AuxValueMap() {}

template <typename T>
T& AuxNames<T>::operator[](const Auxiliar<T>& a){
	return m.unordered_map<string,T>::operator [](a.toString());
}
template <typename T>
T& AuxNames<T>::operator[](const string &s){
	return m.unordered_map<string,T>::operator [](s);
}
template <typename T>
T AuxNames<T>::at(const Auxiliar<T>& a) const {
	return m.unordered_map<string,T>::at(a.toString());
}
template <typename T>
T AuxNames<T>::at(const string& s) const {
	return m.unordered_map<string,T>::at(s);
}
template <typename T>
void AuxNames<T>::clear() {
	m.unordered_map<string,T>::clear();
}
template <typename T>
size_t AuxNames<T>::size() const {
	return m.unordered_map<string,T>::size();
}



FL_TYPE& AuxCoords::operator[](const Auxiliar<FL_TYPE>& a){
	const pattern::Mixture::AuxCoord coords(a.getCoords());
	return m.unordered_map<int,FL_TYPE>::operator [](coords.ag_pos+coords.st_id*sizeof(small_id));
}
FL_TYPE& AuxCoords::operator[](const two<small_id>& ag_st){
	return m.unordered_map<int,FL_TYPE>::operator [](ag_st.first+ag_st.second*sizeof(small_id));
}
FL_TYPE AuxCoords::at(const Auxiliar<FL_TYPE>& a) const {
	const pattern::Mixture::AuxCoord coords(a.getCoords());
	return m.unordered_map<int,FL_TYPE>::at(coords.ag_pos+coords.cc_pos*sizeof(small_id));
}
void AuxCoords::clear() {
	m.unordered_map<int,FL_TYPE>::clear();
}
size_t AuxCoords::size() const {
	return m.unordered_map<int,FL_TYPE>::size();
}


AuxCcEmb::AuxCcEmb(const vector<state::Node*>& _emb) : emb(&_emb) {}
AuxCcEmb::AuxCcEmb() : emb(nullptr){}

void AuxCcEmb::setEmb(const vector<state::Node*>& _emb){
	emb = &_emb;
}

FL_TYPE& AuxCcEmb::operator[](const Auxiliar<FL_TYPE>& a){
	throw invalid_argument("Cannot call [] on AuxEmb.");
}
FL_TYPE AuxCcEmb::at(const Auxiliar<FL_TYPE>& a) const {
	//const pattern::Mixture::AuxCoord coord(a.getCoords());
#ifdef DEBUG
	//cout << "sdfsdf" << endl;
	if(emb)
		if(emb->at(a.getCoords().ag_pos)){
			if(!emb->at(a.getCoords().ag_pos)->getInternalState(a.getCoords().st_id))
				throw invalid_argument("AuxMixEmb::at(): not a valid site coordinate.");
		}else
			throw invalid_argument("AuxMixEmb::at(): agent is null in embedding.");
	else
		throw invalid_argument("AuxMixEmb::at(): emb is null.");
#endif
	return emb->operator[](a.getCoords().ag_pos)->getInternalValue(a.getCoords().st_id).valueAs<FL_TYPE>();
}
void AuxCcEmb::clear() {
	//nothing for now
}
size_t AuxCcEmb::size() const {
	invalid_argument("Cannot call size() on AuxEmb.");
	return 0;
}


AuxMixEmb::AuxMixEmb(const vector<state::Node*>* _emb) : emb(_emb) {}

void AuxMixEmb::setEmb(const vector<state::Node*>* _emb){
	emb = _emb;
}

FL_TYPE& AuxMixEmb::operator[](const Auxiliar<FL_TYPE>& a){
	throw invalid_argument("Cannot call [] on AuxEmb.");
}
FL_TYPE AuxMixEmb::at(const Auxiliar<FL_TYPE>& a) const {
	const pattern::Mixture::AuxCoord coord(a.getCoords());
#ifdef DEBUG
	if(emb)
		if(emb[coord.cc_pos].size())
			if(emb[coord.cc_pos].at(coord.ag_pos)){
				if(!emb[coord.cc_pos].at(coord.ag_pos)->getInternalState(coord.st_id))
					throw invalid_argument("AuxMixEmb::at(): not a valid site coordinate.");
			}else
				throw invalid_argument("AuxMixEmb::at(): agent is null in embedding.");
		else
			throw invalid_argument("AuxMixEmb::at(): CC is null in embedding.");
	else
		throw invalid_argument("AuxMixEmb::at(): embedding is null.");
#endif
	return emb[coord.cc_pos][coord.ag_pos]->getInternalState(coord.st_id)->getValue().valueAs<FL_TYPE>();
}
void AuxMixEmb::clear() {
	emb = nullptr;
}
size_t AuxMixEmb::size() const {
	invalid_argument("Cannot call size() on AuxEmb.");
	return 0;
}

template class AuxValueMap<FL_TYPE>;
template class AuxValueMap<int>;
template class AuxNames<int>;



} /* namespace expressio */
