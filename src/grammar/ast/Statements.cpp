/*
 * Statements.cpp
 *
 *  Created on: May 12, 2016
 *      Author: naxo
 */

#include "Statements.h"
#include "../../util/Exceptions.h"
#include "../../util/Warning.h"
#include "../../pattern/mixture/Component.h"
#include "../../simulation/Simulation.h"

namespace grammar::ast {

/****** Class Statement *********/
Statement::Statement() : useId(Use::getCount()) {}

short Statement::getUseId() const {
	return useId;
}

/****** Class Declaration *******/
Declaration::Declaration() : type(ALG),constant(false),observable(false),expr(nullptr),mixture(nullptr),op(){};
Declaration::Declaration(const location &l,const Id &lab,const Expression *e):
	Node(l),name(lab),type(ALG),constant(false),observable(false),expr(e),mixture(nullptr),op() {};

Declaration::Declaration(const location &l,const Id &lab,const Mixture &m):
	Node(l),name(lab),type(KAPPA),constant(false),observable(false),expr(nullptr),mixture(new Mixture(m)),op() {
};

Declaration::Declaration(const location &l,const Id &lab,
		pair<BaseExpression::N_ary,const Expression*> e,const Mixture &m):
	Node(l),name(lab),type(AUX_EXPR),constant(false),observable(false),
	expr(e.second),mixture(new Mixture(m)),op(e.first) {
};


Declaration::Declaration(const Declaration &d) :
		Node(d.loc),name(d.name),type(d.type),constant(d.constant),observable(d.observable),op(d.op){
	if(type)
		mixture = new Mixture(*(d.mixture));

	if(d.expr) expr = d.expr->clone();
	else expr = nullptr;
}

Declaration& Declaration::operator=(const Declaration &d){
	name = d.name;
	loc = d.loc;
	type = d.type;
	constant = d.constant;
	observable = d.observable;
	op = d.op;
	if(expr)
		delete expr;
	if(mixture)
		delete mixture;

	if(type)
		mixture = new Mixture(*(d.mixture));
	else
		mixture = nullptr;

	if(d.expr)
		expr = d.expr->clone();
	else expr = nullptr;

	return *this;
}
//int Declaration::count = 0;

/*
Declaration::Declaration(const Declaration &&d) : type(d.type){
	cout << "const-copying decl.expr = " << d.expr << endl;
	if(type)
		mixture = new list<Agent>(*(d.mixture));
	else
		if(d.expr) expr = d.expr->clone();
		else expr=NULL;
	count++;
}

Declaration& Declaration::operator =(const Declaration &&d){
	cout << "op-copying decl.expr = " << d.expr << endl;
	//count++;
	loc = d.loc;
	type = d.type;
	if(type)
		mixture = new list<Agent>(*(d.mixture));
	else
		if(d.expr) expr = d.expr->clone();
		else expr=NULL;
	return *this;
}*/

Declaration::~Declaration(){
	if(type)
		delete mixture;
	if (expr) delete expr;
};

Variable* Declaration::evalVar(pattern::Environment &env,
		SimContext &context) const{
	Variable* var = nullptr;
	auto& vars = context.getVars();
	auto eval_name = name.evalLabel(env,context);
	short id = env.declareVariable(Id(name.loc,eval_name),type);
	//if(id == -1)//var was declared before as a param. DEPRECATED
	//	return nullptr;
	if(type){
		if(constant)
			throw SemanticError("Constants cannot be Kappa Expressions.",loc);
		char flag = Expression::AUX_ALLOW | Expression::PATTERN;
		auto mix = mixture->eval(env,context,flag);
		map<string,tuple<int,small_id,small_id>> aux_map;
		if(type == KAPPA)
			var = new state::KappaVar(id,eval_name,observable,*mix);
		else if(type == AUX_EXPR){
			BaseExpression* b_expr = expr->eval(env,context,nullptr,flag,mix);
			if(mix->compsCount() != 1)
				throw SemanticError("Distribution expressions can't have more than one Connected Component.",loc);
			//if(op == BaseExpression::SUMATORY && ) //TODO maybe make DistrVar<int> if expr is int?
			var = new state::DistributionVar<FL_TYPE>(id,eval_name,observable,*mix,make_pair(op,b_expr));
		}

	}
	else {
		char flag = constant ? Expression::CONST : 0;
		BaseExpression* b_expr = expr->eval(env,context,nullptr,flag);
		switch(b_expr->getType()){
		case FLOAT:
			var = new state::AlgebraicVar<FL_TYPE>(id,eval_name,observable,
				dynamic_cast<AlgExpression<FL_TYPE>*>(b_expr));
			break;
		case INT:
			var = new state::AlgebraicVar<int>(id,eval_name,observable,
				dynamic_cast<AlgExpression<int>*>(b_expr));
			break;
		case BOOL:
			var = new state::AlgebraicVar<bool>(id,eval_name,observable,
				dynamic_cast<AlgExpression<bool>*>(b_expr));
			break;
		case NONE:default:
			throw invalid_argument("Declaration::evalVar(): Cannot return None value.");
		}
	}
	return var;
}

Variable* Declaration::evalConst(pattern::Environment &env,
		SimContext &context) const{
	Variable* var;
	short id = env.declareVariable(name,type);
	//if(id == -1)//var was declared before as a param. DEPRECATED
	//	return nullptr;
	if(type)
		throw SemanticError("Constants can not depend on agent mixtures.",loc);
	else {
		char flag = constant ? Expression::CONST : 0;
		BaseExpression* b_expr = expr->eval(env,context,nullptr,flag);
		switch(b_expr->getType()){
		case FLOAT:
			var = new state::ConstantVar<FL_TYPE>(id,name.getString(),
				b_expr->getValue(context).valueAs<FL_TYPE>());
			break;
		case INT:
			var = new state::ConstantVar<int>(id,name.getString(),
					b_expr->getValue(context).valueAs<int>());
			break;
		case BOOL:
			var = new state::ConstantVar<bool>(id,name.getString(),
					b_expr->getValue(context).valueAs<bool>());
			break;
		default:
			throw invalid_argument("Declaration::evalVar(): Cannot return None value.");
		}
	}
	return var;
}

bool Declaration::isKappa() const{
	return type;
}

bool Declaration::isConstant() const{
	return constant;
}

void Declaration::setConstant(bool b){
	constant = b;
}

bool Declaration::isObservable() const{
	return observable;
}
void Declaration::setObservable(bool v){
	observable = v;
}
void Declaration::show( string tabs ) {
	string typeName[] = { "Variable", "Kappa" };

	if( type ) {
		cout << "Mixture ";
	} else {
		cout << "Variable/Constant  ";
	}
	cout << typeName[ type ] << ", name: " << name.getString() << " {";
	expr->show();
	cout << endl << "}" << endl << endl;

}


/****** Class Init ***********/
Init::Init() : type(),alg(nullptr){}

Init::~Init(){};

Init::Init(const location &l,const Expression *e, const Mixture &mix):
		Node(l),type(MIXTURE),alg(e),mixture(mix) {};

Init::Init(const location &l,const Expression *e, const Id &tok):
		Node(l),type(TOKEN),alg(e),token(tok) {};

Init::Init(const Init &init) :
		Node(init.loc),type(init.type),alg(init.alg) {
	if(init.alg)
		alg = init.alg->clone();
	if(type)
		token = init.token;
	else
		mixture = init.mixture;
}

Init& Init::operator=(const Init &init) {
	loc = init.loc;
	type = init.type;
	if(init.alg)
		alg = init.alg->clone();
	if(type)
		token = init.token;
	else
		mixture = init.mixture;
	return *this;
}

void Init::eval(pattern::Environment &env,simulation::SimContext &sim){
	auto& use_expr = env.getUseExpression(this->getUseId());
	auto &cells = use_expr.getCells();//cells to distribute tokens/agents
	if(type){ //TOKEN
		short tok_id;
		if(alg == nullptr)
			throw std::invalid_argument("Null value for token init.");
		auto expr = alg->eval(env,sim,nullptr,Expression::CONST);
		tok_id = env.getTokenId(token.getString());
		env.declareTokInit(this->getUseId(),expr,tok_id);
	}
	else { //MIX
		if(alg == nullptr)
			throw std::invalid_argument("Null value for mix init.");
		auto expr = alg->eval(env,sim,nullptr,Expression::CONST);
		auto mix = mixture.eval(env,sim,false);
		env.declareMixInit(this->getUseId(),expr,mix);
	}
}









} /* namespace ast */
