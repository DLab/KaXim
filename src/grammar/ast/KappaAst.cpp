/*
 * KappaAst.cpp
 *
 *  Created on: Jan 27, 2016
 *      Author: naxo
 */

#include "KappaAst.h"
#include "../../simulation/Simulation.h"
#include "../../pattern/Environment.h"
#include "../../util/Warning.h"

namespace grammar {
namespace ast {

KappaAst::KappaAst()  {
	// TODO Auto-generated constructor stub
	useExpressions.emplace_front(new Use(0));
}

KappaAst::~KappaAst() {
	for(auto pert : perturbations)
		if(pert)
			delete pert;
	for(auto use : useExpressions)
		if(use)
			delete use;
}

void KappaAst::evaluateSignatures(pattern::Environment &env,const SimContext &context){
	env.reserve<pattern::Signature>(signatures.size());
	env.reserve<pattern::Mixture::Agent>(signatures.size());
	for(list<Agent>::iterator it = signatures.begin();it != signatures.end(); it++){
		it->eval(env,context);
	}
	for(auto it = tokens.begin();it != tokens.end(); it++)
		env.declareToken(*it);
}

void KappaAst::evaluateCompartments(pattern::Environment &env,const SimContext &context){
	if(compartments.size() == 0){
		//ADD_WARN("No compartment declared. Declaring default compartment 'volume'.",yy::location());
		env.declareCompartment(Id(yy::location(),"volume"));
		return;
	}
	env.reserve<pattern::Compartment>(compartments.size());
	for(list<Compartment>::iterator it = compartments.begin();it != compartments.end(); it++){
		it->eval(env,context);
	}
}
void KappaAst::evaluateUseExpressions(pattern::Environment &env,const SimContext &context){
	env.reserve<pattern::UseExpression>(useExpressions.size());
	for(auto use : useExpressions){
		use->eval(env,context);
	}
}
void KappaAst::evaluateChannels(pattern::Environment &env,const SimContext &context){
	for(list<Channel>::iterator it = channels.begin();it != channels.end(); it++){
		it->eval(env,context);
	}
}

vector<Variable*> KappaAst::evaluateDeclarations(pattern::Environment &env,SimContext &context, bool is_const){
	auto vars = is_const ? constants : variables;
	auto& var_vector = context.getVars();
	for(list<Declaration>::iterator it = vars.begin();it != vars.end(); it++){
		//delete &(*it);
		Variable* var = nullptr;
		if(is_const)
			var = it->evalConst(env,context);
		else
			var = it->evalVar(env,context);

		if(var)//var is not a param. DEPRECATED
			var_vector.push_back(var);
		if(it->isObservable())
			env.declareObservable(var);
	}
	return var_vector;
}
/** evaluate "%param(s):" kappa statements.
 * %params: allows to declare a list of parameter names with no values associated.
 *     The simulation cannot run if param values are still null.
 * %param: allows to declare and associate a param name with a value.
 *     Can be used several times in model to overwrite previous values.
 * --params: allows to associate values to model params at command line.
 *     Each float value will be associated to a parameter in the same order that
 *     they were declared in the model. This overwrite every previous value. */
void KappaAst::evaluateParams(pattern::Environment &env,SimContext &context,const vector<float>& po_params){
	if(params.size() > po_params.size())
		throw invalid_argument("Too many parameters given as command line argument.");
	unsigned i = 0;
	auto& vars = context.getVars();
	for(auto& param : params){
		if(i < po_params.size()){
			env.declareParam(param.first,new expressions::Constant<FL_TYPE>(po_params[i]));
			cout << "%param: '" << param.first.getString() << "' " << po_params[i] << endl;
		}
		else
			if(param.second)
				env.declareParam(param.first,param.second->eval(env,context));
			else
				throw SemanticError("Sim. parameter "+param.first.getString()+" has no value.",param.first.loc);
		//if(id == vars.size())
		//	vars.push_back(nullptr); else
		//if(id > vars.size())
		//	throw invalid_argument("Unexpected param id.");
		i++;
	}
	vars.resize(env.getParams().size(),nullptr);
	for(auto& param : env.getParams()){
		int id = env.getVarId(param.first);
		vars[id] = Variable::makeAlgVar(id,param.first,param.second->clone());
	}
}

void KappaAst::evaluateInits(pattern::Environment &env,simulation::SimContext &sim){
	for(auto& init : inits){
		init.eval(env,sim);
	}
}

void KappaAst::evaluateRules(pattern::Environment &env,SimContext &context){
	env.reserve<simulation::Rule>(Rule::getCount());
	for(auto& r : rules)
		r.eval(env,context);
}
void KappaAst::evaluatePerts(pattern::Environment &env,SimContext &context){
	env.reserve<simulation::Perturbation>(perturbations.size());
	for(auto p : perturbations)
		p->eval(env,context);
}

void KappaAst::add(const Id &name_loc,const Expression* value) {
	auto name = name_loc.getString();
	for(auto& param : params)
		if(name == param.first.getString()){
			if(value == nullptr)
				throw SemanticError("Redeclaration of model parameter "+name,name_loc.loc);
			else if(param.second != nullptr){
				ADD_WARN("Previous value of model parameter '"+name+"' overwritten.",value->loc);
			}
			delete param.second;
			param.second = value;
			return;
		}

	params.emplace_back(name_loc,value);
}
void KappaAst::add(const Declaration &d){
	if(d.isConstant())
		constants.push_back(d);
	else
		variables.push_back(d);
}
void KappaAst::add(const Agent &a){
	signatures.push_back(a);
}
void KappaAst::add(const Compartment &c){
	compartments.push_back(c);
}
void KappaAst::add(const Channel &c){
	channels.push_back(c);
}
void KappaAst::add(const Id &t){
	tokens.push_back(t);
}
void KappaAst::add(const Init &i){
	inits.push_back(i);
}
void KappaAst::add(const Use *u){
	useExpressions.push_back(u);
}
void KappaAst::add(const Rule &r){
	rules.push_back(r);
}

void KappaAst::add(const Pert *p){
	perturbations.push_back(p);
}

void KappaAst::show(){
	cout << endl << "Showing variables:" << endl;
	for(list<Declaration>::iterator it = variables.begin();it != variables.end(); it++){
		it->show();
	}

	cout << endl << "Showing constants:" << endl;
	for(list<Declaration>::iterator it = constants.begin();it != constants.end(); it++){
		it->show();
	}

	cout << endl << "Showing agents:" << endl;
	for(list<Agent>::iterator it = signatures.begin();it != signatures.end(); it++){
		it->show();
	}

	cout << endl << "Showing tokens:" << endl;
	for(list<Id>::iterator it = tokens.begin();it != tokens.end(); it++){
		it->show();
	}

	cout << endl << "Showing Perturbations:" << endl;
	short i = 0;
	for(auto pert : perturbations){
		cout << ++i << ") ";
		pert->show();
	}

	cout << endl;
}

} /* namespace ast */
}
