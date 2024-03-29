/*
 * Expression.cpp
 *
 *  Created on: May 2, 2016
 *      Author: naxo
 */

#include "AlgebraicExpressions.h"
#include "../../state/Variable.h"
#include "../../expressions/Constant.h"
#include "../../expressions/Vars.h"
#include "../../expressions/Function.h"
#include "../../pattern/mixture/Component.h"

#include <limits>
#include <type_traits>

namespace grammar {
namespace ast {

/****** Class Const **********/
Const::Const (const location &loc,const float f):
		Expression(loc),type(FLOAT),f(f){}
Const::Const (const location &loc,const int i):
		Expression(loc),type(INTEGER),n(i){}
Const::Const (const location &loc,const bool b):
		Expression(loc),type(BOOL),b(b){}
Const::Const (const location &loc,const ConstType t):
		Expression(loc),type(t){}

bool Const::isConstant(){
	return true;
}

void Const::show( string tabs ) const {
	string ar_types[] = {"FLOAT","INTEGER","BOOL","INF","INF_NEG","TMAX,EMAX"};
	tabs += "   ";

	cout << endl << tabs << "Constant : (" << ar_types[type] << ", ";

	switch(type){
	case FLOAT:
		cout << f;
		break;
	case INTEGER:
		cout << n;
		break;
	case BOOL:
		cout << b;
		break;
	case INF:
		cout << "infinity";
		break;
	case INF_NEG:
		cout << "negative infinity";
		break;
	case TMAX:
		cout << "time max";
		break;
	case ConstType::EMAX:
		cout << "event max";
		break;
	}

	cout << ")";
}

BaseExpression* Const::eval(const pattern::Environment& env,
		const simulation::SimContext& context,pattern::DepSet* deps,const char flags,
		const pattern::Mixture* mix) const{
	BaseExpression* cons = nullptr;
	switch(type){
	case FLOAT:
		cons = new Constant<FL_TYPE>(f);
		break;
	case INTEGER:
		cons = new Constant<int>(n);
		break;
	case BOOL:
		cons = new Constant<bool>(b);
		break;
	case INF:
		cons = new Constant<FL_TYPE>(std::numeric_limits<FL_TYPE>::infinity());
		break;
	case INF_NEG:
		cons = new Constant<FL_TYPE>(-std::numeric_limits<FL_TYPE>::infinity());
		break;
	case TMAX:
		cons = new Constant<int>(-1);
		break;
	case ConstType::EMAX:
		cons = new Constant<int>(-1);
		break;
	}
	return cons;
}
Const* Const::clone() const{
	return new Const(*this);
}

/****** Class BoolBinaryOperation ***/
BoolBinaryOperation::BoolBinaryOperation(const location &l,const Expression *e1,
		const Expression *e2,BaseExpression::BoolOp o):
		Expression(l),exp1(e1),exp2(e2),op(o){}

BoolBinaryOperation::~BoolBinaryOperation(){
	delete exp1;
	delete exp2;
}

BaseExpression* BoolBinaryOperation::eval(const pattern::Environment& env,
		const simulation::SimContext& context,pattern::DepSet* deps,const char flags,
		const pattern::Mixture* mix) const{
	BaseExpression* ex1 = exp1->eval(env,context,deps,flags,mix);
	BaseExpression* ex2 = exp2->eval(env,context,deps,flags,mix);

	return BaseExpression::makeBinaryExpression<true>(ex1,ex2,op);
}
BoolBinaryOperation* BoolBinaryOperation::clone() const{
	return new BoolBinaryOperation(loc,exp1->clone(),exp2->clone(),op);
}
void BoolBinaryOperation::show( string tabs ) const {
	string ar_types[] = {"AND","OR","GREATER","SMALLER","EQUAL","DIFF"};
	tabs += "   ";

	cout << endl << tabs << "boolBinaryOperation: (";
	exp1->show( tabs );
	cout << endl << tabs+"   " << "operator: " << ar_types[op];
	exp2->show( tabs );
	cout << endl << tabs << ")";
}


/****** Class AlgBinaryOperation ****/
AlgBinaryOperation::AlgBinaryOperation(const location &l,const Expression *e1,
		const Expression *e2,BaseExpression::AlgebraicOp o):
		Expression(l),exp1(e1),exp2(e2),op(o){};

AlgBinaryOperation::~AlgBinaryOperation(){
	delete exp1;
	delete exp2;
}

BaseExpression* AlgBinaryOperation::eval(const pattern::Environment& env,
		const simulation::SimContext& context,pattern::DepSet* deps,const char flags,
		const pattern::Mixture* mix) const{
	BaseExpression* ex1 = exp1->eval(env,context,deps,flags,mix);
	BaseExpression* ex2 = exp2->eval(env,context,deps,flags,mix);
	if(op == BaseExpression::MULT){
		if(*ex1 == *INF_EXPR){
			delete ex1;
			ex1 = MAX_FL_EXPR->clone();
			ADD_WARN("Replacing [inf] expr for max-float in multiplication to avoid NaN (infinity times zero)",loc);
		}
		else if(*ex2 == *INF_EXPR){
			delete ex2;
			ex2 = MAX_FL_EXPR->clone();
			ADD_WARN("Replacing [inf] expr for max-float in multiplication to avoid NaN (infinity times zero)",loc);
		}
	}


	return BaseExpression::makeBinaryExpression<false>(ex1,ex2,op);
}
AlgBinaryOperation* AlgBinaryOperation::clone() const{
	return new AlgBinaryOperation(loc,exp1->clone(),exp2->clone(),op);
}
void AlgBinaryOperation::show( string tabs ) const {
	string ar_types[] = {"SUM","MINUS","MULT","DIV","POW","MODULO","MAX","MIN"};
	tabs += "   ";

	cout << endl << tabs << "algBinaryOperation: (";
	exp1->show( tabs );
	cout << endl << tabs+"   " << "operator: " << ar_types[op];
	exp2->show( tabs );
	cout << endl << tabs << ")";
}


/****** Class UnaryOperation ********/
UnaryOperation::UnaryOperation(const location &l,const Expression *e,
		const BaseExpression::Unary f):
		Expression(l),exp(e),func(f){};

UnaryOperation::~UnaryOperation(){
	delete exp;
}

BaseExpression* UnaryOperation::eval(const pattern::Environment& env,
		const simulation::SimContext& context,pattern::DepSet* deps,const char flags,
		const pattern::Mixture* mix) const{
	BaseExpression* ex = exp->eval(env,context,deps,flags,mix);
	return BaseExpression::makeUnaryExpression(ex, func);
}
UnaryOperation* UnaryOperation::clone() const{
	return new UnaryOperation(loc,exp->clone(),func);
}
void UnaryOperation::show( string tabs ) const {
	string ar_type[] = {"SQRT","EXPONENT","LOG","SINUS","COSINUS","TAN","ATAN","ABS","COIN","RAND_N","NOT"};
	tabs += "   ";

	cout << endl << tabs << "UnaryOperation: (";
	exp->show( tabs );
	cout << endl << tabs+"   " << "operation: " << ar_type[func];
	cout << endl << tabs << ")";
}


/****** Class NullaryOperation ******/
BaseExpression* NullaryOperation::eval(const pattern::Environment& env,
		const simulation::SimContext& context,pattern::DepSet* deps,const char flags,
		const pattern::Mixture* mix) const{
	switch(func){
	case BaseExpression::Nullary::SIM_TIME:
		if(deps)
			deps->emplace(pattern::Dependency::TIME);
		break;
	case BaseExpression::Nullary::CPUTIME:
	case BaseExpression::Nullary::ACTIVITY:
	case BaseExpression::Nullary::SIM_EVENT:
	case BaseExpression::Nullary::NULL_EVENT:
	case BaseExpression::Nullary::PROD_EVENT:
		if(deps)
			deps->emplace(pattern::Dependency::EVENT);
		break;
	case BaseExpression::Nullary::END_SIM:
		if(deps)
			deps->emplace(pattern::Dependency::NONE);
		break;
	default:break;
	}
	return BaseExpression::makeNullaryExpression(func);
}
NullaryOperation::NullaryOperation(const location &l,const BaseExpression::Nullary f):
		 Expression(l), func(f){};
NullaryOperation* NullaryOperation::clone() const{
	return new NullaryOperation(*this);
}
void NullaryOperation::show( string tabs ) const {
	static string n_ops[] = {"Rand(1)","TIME","EVENT","NULL_EVENT","PROD_EVENT","CPUTIME","ACTIVITY"};
	tabs += "   ";

	cout << endl << tabs << "NullaryOperation: (" << n_ops[func] << ")";
}


/****** Class Var ***/
Var::Var(const location &l,const VarType &t,const Id &label):
	Expression(l),name(label),type(t){};

BaseExpression* Var::eval(const pattern::Environment& env,
		const simulation::SimContext& context,pattern::DepSet* deps,const char flags,
		const pattern::Mixture* mix) const {
	BaseExpression* expr =nullptr;
	auto& vars  = context.getVars();
	switch(type){
	case VAR:{
		unsigned id = env.getVarId(Id(name.loc,name.evalLabel(env,context)));
		expr = vars[id]->makeVarLabel();
		if(deps){
			auto* kappa = dynamic_cast<state::KappaVar*>(vars[id]);
			if(kappa)
				for(auto cc : kappa->getMix())
					deps->emplace(pattern::Dependency::KAPPA,cc->getId());
			else
				deps->emplace(pattern::Dependency::VAR,id);
		}
		break;
	}
	case AUX:
		if(flags & AUX_ALLOW){
			if( (flags & LHS) || (flags & RHS) || (flags & PATTERN) ){
				//TODO maybe change to Aux<type> depending on valued_site type?
				auto aux_expr = new Auxiliar<FL_TYPE>(name.getString());
				if(mix)
					mix->addAuxRef(aux_expr);
				else
					env.getMixtures().back()->addAuxRef(aux_expr);
				expr = aux_expr;
				if(deps)//TODO set deps
					deps->emplace(name.getString());
			}
			else//is compartment
				expr = new Auxiliar<int>(name.getString());//comp aux || aux_map->at(name.getString()));
		}
		else
			throw SemanticError("The auxiliar '"+name.getString()+"' can not be used here.", loc);
		break;
	case TOKEN:
		try {
			auto id = env.getTokenId(name.getString());
			if(deps)
				deps->emplace(pattern::Dependency::TOK,id);
			expr = new state::TokenVar(id);
		}
		catch(const std::out_of_range &e){
			throw SemanticError("Token '"+name.getString()+
					"' was not defined.",loc);
		}
		break;
	}
	return expr;
}
Var* Var::clone() const{
	return new Var(*this);
}

void Var::show( string tabs ) const {
	string ar_type[]= {"VAR","TOKEN","AUX","TIME","EVENT","NULL_EVENT","PROD_EVENT","CPUTIME","ACTIVITY"};
	tabs += "   ";

	cout << endl << tabs << "Variable: " << name.getString() << ", type: " << ar_type[type];
}







Func::Func(const location &loc,BaseExpression::Funcs f,const list<Expression*>& _args)
		: Expression(loc),type(f),args(_args){}

Func::~Func(){
	for(auto arg : args)
		delete arg;
}


BaseExpression* Func::eval(const pattern::Environment& env,
		const simulation::SimContext& context,pattern::DepSet* deps,const char flags,
		const pattern::Mixture* mix) const {
	BaseExpression* expr =nullptr;
	list<BaseExpression*> expr_list;
	for(auto arg : args)
		expr_list.push_back(arg->eval(env,context,deps,flags,mix));
	expr = new Function<FL_TYPE>(type,expr_list);
	return expr;
}
Func* Func::clone() const{
	list<Expression*> new_args;
	for(auto arg : args)
		new_args.push_back(arg->clone());
	return new Func(loc,type,new_args);
}

void Func::show( string tabs ) const {
	string ar_type[]= {"VAR","TOKEN","AUX","TIME","EVENT","NULL_EVENT","PROD_EVENT","CPUTIME","ACTIVITY"};
	tabs += "   ";

	cout << endl << tabs << "Beta (?)";
}










} /* namespace ast */
}
