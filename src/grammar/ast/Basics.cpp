/*
 * Basics.cpp
 *
 *  Created on: May 19, 2016
 *      Author: naxo
 */

#include "Basics.h"
#include "../../util/Warning.h"
#include <regex>


namespace grammar {
namespace ast {

/****** Class Node *******************/
Node::Node() : loc(location()){}
Node::Node(const yy::location& l)
	:loc(l)	{/*cout << "construct Node at " << l << endl;*/}

Node::~Node(){};
void Node::show( string tabs ) const {
	tabs += "   ";
	cout << tabs << "Node" << endl;
}


/****** Class Id *********************/
Id::Id(){}
Id::Id(const Node &t,const string &s): Node(t),id(s){};
Id::Id(const location &l,const string &s): Node(l),id(s){};
Id::~Id(){};


string Id::evalLabel(const pattern::Environment &env, const simulation::SimContext& context) const {
	string ret,s;
	smatch m;
	std::regex r("(\")([^\"]+)(\")");
	s = id;
	try {
		auto& vars = context.getVars();
		while(regex_search(s,m,r)){
			ret += m.prefix().str() + vars[env.getVarId(m[2].str())]->getValue(context).toString();
			s = m.suffix().str();
		}
		ret += s;
	}
	catch(exception &e){
		ADD_WARN("Cannot replace sub-label "+m[2].str()+".",loc);
		ret += m[0].str() + m.suffix().str();
	}
	return ret;
}

const string& Id::getString() const {return id;};

void Id::show( string tabs ) const {
	tabs += "   ";
	cout << tabs << "Id: " << id;
}

/****** Class Expression *************/
Expression::Expression(){}
Expression::Expression(const yy::location& l):
		Node(l){}
Expression::~Expression(){};

bool Expression::isConstant(){
	return false;
}
/*state::BaseExpression* Expression::eval(const pattern::Environment& env,
		const VAR &vars,pattern::DepSet* deps,
		const char flags) const{
	cout << "do not call this function" << endl;
	throw;
	return nullptr;
}*/
void Expression::show( string tabs ) const {
	cout << "Expression : location: " << loc.begin << " " << loc.end;
}


/****** Class VarValue ***************/
VarValue::VarValue() : value(nullptr){}
VarValue::VarValue(const location &l,const Id &name,const Expression *exp) :
	Node(l),var(name),value(exp) {}
void VarValue::show( string tabs ) const{
	tabs += "   ";
	cout << tabs << "VarValue : ";
	var.show();
	if(value) value->show(tabs);
	cout << endl;
}

/****** Class StringExpression *******/
StringExpression::StringExpression():
	t(STR),str(),alg(nullptr){};

StringExpression::StringExpression(const location &l,const string s):
	Node(l),t(STR),str(s),alg(nullptr) {};

StringExpression::StringExpression(const location &l,const Expression *e):
	Node(l),t(ALG),alg(e),str() {};

string StringExpression::evalConst(const pattern::Environment &env,const simulation::SimContext& args) const {
	if(t)
		return alg->eval(env, args, nullptr, Expression::CONST)->getValue(args).toString();
	else
		return str;
}


void StringExpression::show(string tabs) const {
	tabs += "   ";
	if( t ) {
		alg->show(tabs+"   ");
	} else {
		cout << tabs << str;
	}
}

StringExpression::~StringExpression(){
	/*if(t)
		delete alg;*/
}


/****** Class Arrow **********//*
Arrow::Arrow(): type(RIGHT){}
Arrow::Arrow(const location &loc,ArrType t):
		Node(loc), type(t) {};
Arrow::ArrType Arrow::getType(){
	return type;
};
*/

} //namespace ast
}
