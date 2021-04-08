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
	R evaluate(const SimContext& args)const override;
	R evaluateSafe(const SimContext& args)const override;
	FL_TYPE auxFactors(std::unordered_map<std::string, FL_TYPE> &factor) const override;

	BaseExpression::Reduction factorize(const std::map<std::string,small_id> &aux_cc) const override;
	BaseExpression* clone() const override;


	bool operator==(const BaseExpression& exp) const override;
	char getVarDeps() const override;
	BaseExpression* reduce(SimContext& context) override;
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
	R evaluate(const SimContext& args) const override;
	R evaluateSafe(const SimContext& args) const override;
	FL_TYPE auxFactors(std::unordered_map<std::string, FL_TYPE> &factor) const
			override;
	BaseExpression::Reduction factorize(const std::map<std::string,small_id> &aux_cc) const
			override;
	BaseExpression* clone() const
			override;
	bool operator==(const BaseExpression& exp) const;
	//std::set<std::string> getAuxiliars() const override;
	BaseExpression* reduce(SimContext& context) override;

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



} /* namespace expressio */

#endif /* SRC_EXPRESSIONS_VARS_H_ */
