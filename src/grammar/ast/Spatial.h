/*
 * Spatial.h
 *
 *  Created on: May 12, 2016
 *      Author: naxo
 */

#ifndef GRAMMAR_AST_SPATIAL_H_
#define GRAMMAR_AST_SPATIAL_H_

#include <string>
#include <set>

#include "Dynamics.h"
#include "AlgebraicExpressions.h"

namespace grammar {
namespace ast {
using namespace std;


/** \brief Representation of a compartment expression.
 *
 * Stores the data of a compartment expression. It includes the name
 * and a list of integer alg. expressions and can represent a compartment
 * declaration or an expression of a set of cells using auxiliars.
 */
class CompExpression: public Node {
public:
	/** \brief Empty constructor.
	 * Just needed by the parser.
	 */
	CompExpression();
	/** \brief CompExpression constructor.
	 * @param l location of whole expression
	 * @param id Id of the compartment
	 * @param indexlist set of index expressions of the compartment.
	 */
	CompExpression(const location &l,const Id &id,const list<const Expression*> &indexlist);

	pattern::CompartmentExpr* eval(pattern::Environment& env,
			const unordered_map<string,state::Variable*> &vars,
			bool allowAux);

	/** \brief Evaluate this expression as if every index expression is a
	 * constant int.
	 *
	 * Force alg. expression of each index to have int values and return
	 * a vector of short ints representing the index list.
	 * @param env the environment of the simulation.
	 * @param consts the variable vector (only constant values can be used).
	 * @return a short int for every index as a vector.
	 */
	vector<short> evalDimensions(const pattern::Environment &env,
			const SimContext &context) const;

	/** \brief Tests if this compartment is declared and returns its Id.
	 *
	 * Tests if this compartment is declared only if 'declare' is false.
	 * then returns the Id.
	 * @param env the environment of the simulation.
	 * @param declare true if this compartment has not ben declared.
	 * @returns The Id of this compExpression.
	 */
	const Id& evalName(const pattern::Environment& env,bool declare=false) const;

	/** \brief Evaluate this expression as it can represent several cells.
	 *
	 *	Evaluate alg. expressions for each index of compartment and returns
	 * a list of BaseExpression* for index.
	 * @param env the environment of the simulation.
	 * @param consts the variable vector (only constants vars can be accessed).
	 * @return a list of BaseExpression* for each index of compExpression by copy.
	 */
	list<const state::BaseExpression*> evalExpression(const pattern::Environment &env,
			small_id comp_id,const SimContext &context,char flags = Expression::AUX_ALLOW) const;


protected:
	Id name;
	list<const Expression*> indexList;
};

/** \brief The AST of a compartment declaration. */
class Compartment : public Node {
	CompExpression comp;
	Expression* volume;
public:
	Compartment(const location& l,const CompExpression& comp_exp,
			Expression* exp);
	unsigned eval(pattern::Environment &env,const SimContext &context);
};

/** \brief The AST of a channel declaration */
class Channel : public Node {
	Id name;
	CompExpression source,target;
	bool bidirectional;
	const Expression* filter;//bool expr?
	const Expression* delay;
public:
	Channel(const location& l,const Id& nme, const CompExpression& src,
			const CompExpression& trgt, bool bckwrds, const Expression* where=nullptr,
			const Expression* delay=nullptr);
	void eval(pattern::Environment &env,
			const SimContext &context);
	tuple<int,int,bool> evalLink(pattern::Environment &env,
				const SimContext &context) const;

};

/** \brief The AST of the \%use statement. */
class Use : public Node {
	friend class KappaAst;
	static unsigned short count;
	const unsigned short id;
	list<CompExpression> compartments;
	const Expression* filter;

public:
	static unsigned short getCount();
	Use(const location &l,
		const list<CompExpression> &comps = list<CompExpression>(),
		const Expression* where = nullptr);
	Use(short _id);
	Use(const Use& u);
	~Use();

	void eval(pattern::Environment &env,const SimContext &context) const;
};

class Transport : public Node {
	friend class KappaAst;
	Id link;
	const Channel* channel;
	RuleSide lhs;
	Rate rate;
	bool join;

public:
	Transport(Id lnk,RuleSide mix,Rate r,bool j = true) :
		link(lnk),channel(nullptr),lhs(mix),rate(r),join(j){ }
	Transport(const Channel* chnl,RuleSide mix,Rate r,bool j = true) :
		channel(chnl),lhs(mix),rate(r),join(j){ }

	void eval(SimContext& context) const;
};



} /* namespace ast */
}

#endif /* GRAMMAR_AST_SPATIAL_H_ */
