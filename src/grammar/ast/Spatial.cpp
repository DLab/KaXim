/*
 * Spatial.cpp
 *
 *  Created on: May 12, 2016
 *      Author: naxo
 */

#include "Spatial.h"

namespace grammar {
namespace ast {

/****** Class CompExpression *********/
CompExpression::CompExpression(){}
CompExpression::CompExpression(const location &l,const Id &id,const list<const Expression*> &dim):
	Node(l),name(id), indexList(dim) {};

pattern::CompartmentExpr* CompExpression::eval
		(pattern::Environment& env,
		const unordered_map<string,state::Variable*> &vars,bool isDeclaration){
	//short id =
	evalName(env,isDeclaration);

	return nullptr;
}

vector<short> CompExpression::evalDimensions(const pattern::Environment &env,
		const SimContext &context) const {
	vector<short> ret;
	for(auto& index : indexList) {
		int tmp = 0;

		switch( (index->eval(env,context))->getType() ) {
		case FLOAT://TODO check if vars is only consts
			tmp = (index->eval(env,context))->getValueSafe(context).fVal;
			break;
		case INT:
			tmp = (index->eval(env,context))->getValueSafe(context).iVal;
			break;
		default:break;
		}

		//tmp = (index->eval(env,vars))->getValue().iVal;
		ret.push_back(tmp);
	}

	return ret;
}

const Id& CompExpression::evalName(const pattern::Environment &env,bool declare) const {
	if(!declare){
		try{
			env.getCompartmentId(name.getString());
		}
		catch(const std::out_of_range &ex){
			throw SemanticError("No compartment called "+name.getString(),loc);
		}
	}
	return name;
}
list<const BaseExpression*> CompExpression::evalExpression(const pattern::Environment &env,
			small_id comp_id,const SimContext &context) const {
	list<const state::BaseExpression*> ret;
	auto& dims = env.getCompartment(comp_id).getDimensions();
	int i = 0;
	for(auto index : indexList){
		auto expr = index->eval(env,context,nullptr,Expression::AUX_ALLOW);
		try{//TODO check if vars is only consts
			int d = expr->getValueSafe(context).valueAs<int>();
			if(d < 0 || d >= dims[i])
				throw SemanticError("Index out of limits for compartments expression.",index->loc);
		}
		catch(std::out_of_range &e){
			//DO NOTHING
		}
		ret.push_back(expr);
	}
	return ret;
}




/****** Class Compartment ***********/

Compartment::Compartment(const location& l,const CompExpression& comp_exp,
		Expression* exp) : Node(l), comp(comp_exp), volume(exp) {}

//TODO
void Compartment::eval(pattern::Environment &env,const SimContext &context){
	const Id& name = comp.evalName(env,true);
	pattern::Compartment& c = env.declareCompartment(name);
	vector<short> dims = comp.evalDimensions(env,context);
	BaseExpression* vol = volume->eval(env,context,nullptr,Expression::CONST);
	c.setDimensions(dims);
	c.setVolume(vol);
	return;
}


/****** Class Channel ***************/

Channel::Channel(const location& l,const Id& nme, const CompExpression& src,
			const CompExpression& trgt, bool bckwrds, const Expression* where,
			const Expression* wait) :
		Node(l),name(nme),source(src),target(trgt),bidirectional(bckwrds),
		filter(where),delay(wait) {}


void Channel::eval(pattern::Environment &env,
		const SimContext &context){
	short src_id,trgt_id;
	pattern::Channel& channel = env.declareChannel(name);

	src_id = env.getCompartmentId(source.evalName(env,false).getString());
	trgt_id = env.getCompartmentId(target.evalName(env,false).getString());

	list<const BaseExpression*> src_index,trgt_index;
	src_index = source.evalExpression(env,src_id,context);
	trgt_index = target.evalExpression(env,trgt_id,context);

	pattern::CompartmentExpr *c_exp_src,*c_exp_trgt;
	try{
		c_exp_src = new pattern::CompartmentExpr(env.getCompartment(src_id),src_index);
	}catch(std::invalid_argument &e){
		throw SemanticError(e.what(),source.loc);
	}
	try{
		c_exp_trgt = new pattern::CompartmentExpr(env.getCompartment(trgt_id),trgt_index);
	}catch(std::invalid_argument &e){
		throw SemanticError(e.what(),target.loc);
	}
	try{
		c_exp_src->setEquation();
		if(bidirectional)
			c_exp_trgt->setEquation();
	}catch(std::invalid_argument &e){
		throw SemanticError(e.what(),loc);
	}
	channel.setCompExpressions(c_exp_src,c_exp_trgt);
	if(filter)
		channel.setFilter(filter->eval(env,context));
	channel.setDelay(delay->eval(env,context));
	return;
}



/*********** class Use ***********/
unsigned short Use::count = 0;
unsigned short Use::getCount(){
	return count;
}

Use::Use(const location &l, const list<CompExpression> &comps,
		const Expression* where) :
	Node(l),id(++count),compartments(comps),filter(where){}
Use::Use(const Use &u) : Node(u.loc),id(u.id),
		compartments(u.compartments),filter(u.filter->clone()) {}
Use::Use(short _id) : id(_id),filter(nullptr) {};
Use::~Use(){
	if(filter) delete filter;
}


void Use::eval(pattern::Environment &env, const SimContext &context) const {
	list<pattern::CompartmentExpr> comp_exprs;
	auto& use_expr = env.declareUseExpression(id,compartments.size());
	for(auto& comp : compartments){
		short comp_id = env.getCompartmentId(comp.evalName(env,false).getString());
		list<const BaseExpression*> index_list = comp.evalExpression(env,comp_id,context);
		try{
			use_expr.emplace_back(env.getCompartment(comp_id),index_list);
		}catch(std::invalid_argument &e){
			throw SemanticError(e.what(),comp.loc);
		}
	}
	try{
		use_expr.evaluateCells(context);
	}
	catch(std::invalid_argument& e){
		throw SemanticError(e.what(),loc);
	}
}




} /* namespace ast */
}
