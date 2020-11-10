/*
 * Vars.h
 *
 *  Created on: Nov 12, 2018
 *      Author: naxo100
 */

#ifndef SRC_EXPRESSIONS_VARS_H_
#define SRC_EXPRESSIONS_VARS_H_

#include "AlgExpression.h"
#include "../pattern/mixture/Mixture.h"


namespace state { class Node; }

namespace expressions {

using namespace std;

template<typename R>
class VarLabel: public AlgExpression<R> {
	int varId;
	string name;

public:
	VarLabel(int id,const string& name);
	R evaluate(const SimContext<true>& args)const override;
	R evaluate(const SimContext<false>& args)const override;
	FL_TYPE auxFactors(std::unordered_map<std::string, FL_TYPE> &factor) const override;

	BaseExpression::Reduction factorize(const std::map<std::string,small_id> &aux_cc) const override;
	BaseExpression* clone() const override;


	bool operator==(const BaseExpression& exp) const override;
	char getVarDeps() const override;
	BaseExpression* reduce(VarVector &vars) override;
	std::string toString() const override;
};


template<typename R>
class Auxiliar: public AlgExpression<R> {
	friend class pattern::Mixture;
	std::string name;
	/// mix_id,cc_id,ag_id,site_id
	pattern::Mixture::AuxCoord coords;
public:
	//enum CoordInfo {/*MIX_ID,*/CC_POS,AG_POS,ST_ID};
	Auxiliar(const std::string &nme);
	Auxiliar(const std::string &nme,pattern::Mixture::AuxCoord coords);
	virtual ~Auxiliar();
	R evaluate(const SimContext<true>& args) const override;
	R evaluate(const SimContext<false>& args) const override;
	FL_TYPE auxFactors(std::unordered_map<std::string, FL_TYPE> &factor) const
			override;
	BaseExpression::Reduction factorize(const std::map<std::string,small_id> &aux_cc) const
			override;
	BaseExpression* clone() const
			override;
	bool operator==(const BaseExpression& exp) const;
	//std::set<std::string> getAuxiliars() const override;
	BaseExpression* reduce(VarVector &vars) override;

	inline pattern::Mixture::AuxCoord getCoords() const {
		return coords;
	}

	char getVarDeps() const override;
	bool isAux() const override;

	/*inline const tuple<small_id&,small_id&,small_id>& getCoords() const {
		return coords;
	}*/

	std::string toString() const override;
};


class AuxCoords : public AuxMap {
	unordered_map<int,FL_TYPE> m;
public:
	FL_TYPE& operator[](const Auxiliar<FL_TYPE>& a) override;
	FL_TYPE& operator[](const two<small_id>& ag_st);
	FL_TYPE at(const Auxiliar<FL_TYPE>& a) const override;
	void clear() override;
	size_t size() const;
};
template <typename T>
class AuxNames : public AuxValueMap<T> {
	unordered_map<string,T> m;
public:
	T& operator[](const Auxiliar<T>& a) override;
	T& operator[](const string &s);
	T at(const Auxiliar<T>& a) const override;
	T at(const string& s) const;
	void clear() override;
	size_t size() const;
	auto begin() const {
		return m.begin();
	}
	auto end() const {
		return m.end();
	}
};


class AuxCcEmb : public AuxMap {
	const vector<state::Node*>* emb;
public:
	AuxCcEmb();
	AuxCcEmb(const vector<state::Node*>& emb);
	void setEmb(const vector<state::Node*>& _emb);
	FL_TYPE& operator[](const Auxiliar<FL_TYPE>& a) override;
	FL_TYPE at(const Auxiliar<FL_TYPE>& a) const override;
	void clear() override;
	size_t size() const;
};

class AuxMixEmb : public AuxMap {
	const vector<state::Node*>* emb;
public:
	AuxMixEmb() : emb(nullptr){}
	AuxMixEmb(const vector<state::Node*>* emb);
	void setEmb(const vector<state::Node*>* _emb);
	FL_TYPE& operator[](const Auxiliar<FL_TYPE>& a) override;
	FL_TYPE at(const Auxiliar<FL_TYPE>& a) const override;
	void clear() override;
	size_t size() const;
};

} /* namespace expressio */

#endif /* SRC_EXPRESSIONS_VARS_H_ */
