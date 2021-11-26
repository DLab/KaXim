/*
 * Dynamics.cpp
 *
 *  Created on: May 12, 2016
 *      Author: naxo
 */

#include "Dynamics.h"
#include "../../util/Warning.h"
#include "../../expressions/BinaryOperation.h"
#include "../../expressions/UnaryOperation.h"
#include "../../expressions/NullaryOperation.h"
#include "../../pattern/mixture/Agent.h"
#include "../../pattern/mixture/Component.h"

namespace grammar {
namespace ast {

using Deps = pattern::Dependency;

/****** Class Link ***********/
Link::Link() : type(LinkType::FREE),value(0){}
Link::Link(const location &l,LinkType t):
	Node(l), type(t), value()  {};
Link::Link(const location &l,LinkType t,int val):
	Node(l), type(t), value(val) {};
Link::Link(const location &l,LinkType t,const Id &ag,const Id &s):
	Node(l), type(t), value(), ag_site{ag,s}{	};
Link::Link(const Link &l):Node(l){
	type = l.type;
	switch(type){
	case LinkType::BND:
		value = l.value;
		break;
	case LinkType::BND_PTRN:
		ag_site = l.ag_site;
		break;
	default:
		break;
	}
};
Link& Link::operator=(const Link &l){
	loc = l.loc;
	type = l.type;
	switch(type){
	case LinkType::BND:
		value = l.value;
		break;
	case LinkType::BND_PTRN:
		ag_site = l.ag_site;
		break;
	default:
		break;
	}
	return *this;
};
Link::~Link(){};
const Link::LinkType& Link::getType() const{
	return type;
}

void Link::eval(const pattern::Environment &env,pattern::Pattern::Site& mix_site,
		pattern::Mixture& mix,short site_id,bool allow_pattern) const {
	short ag_pos = mix.size()-1;
	mix_site.setBindPattern(type);
	switch(type){
	case LinkType::BND:{
		mix.addBindLabel(value,ag_pos,site_id);
		break;
	}
	case LinkType::BND_PTRN:
		if(!allow_pattern)
			throw SemanticError("Patterns are not allowed here.",loc);
		try{
			int bind_to_ag(env.getSignatureId(ag_site.first.getString()));
			int bind_to_site(env.getSignature(bind_to_ag).getSiteId(ag_site.second.getString()));
			mix_site.setBindPattern(type,bind_to_ag,bind_to_site);
		}catch(std::out_of_range &e){
			throw SemanticError("Link pattern (site,agent) '"+ag_site.second.getString()+
					"."+ag_site.first.getString()+"' is not defined.",
					ag_site.second.loc+ag_site.first.loc);
		}
		break;
	case LinkType::FREE:
		break;
	default://WILD,BIND_ANY
		if(!allow_pattern)
			throw SemanticError("Patterns are not allowed here.",loc);
		/*if(mix_site.isEmptySite())
			ADD_WARN("A site declared as 'bind to any' and 'any value' can be omitted.",loc);*/
		break;
	}
}


/****** Class SiteState ***********/

SiteState::SiteState() :Node(),type(EMPTY),val(nullptr),flag(0) {}
SiteState::~SiteState(){};
SiteState::SiteState(const location& loc, const list<Id> &labs) :
		Node(loc),type(labs.size()>0?LABEL:EMPTY),labels(labs),
		  val(nullptr),flag(0){}

SiteState::SiteState(const location& loc, const Expression* min,
		const Expression* max,const Expression* def) : Node(loc),
				type(RANGE),val(nullptr),range{min,def,max},flag(0){
	if(!def)
		range[1] = min;
}
SiteState::SiteState(const location& loc, const Expression* value) :Node(loc),
		type(EXPR),val(value),flag(0){}
SiteState::SiteState(const location& loc, const Id &_aux,
		const Expression* equal) : Node(loc),
		type(AUX),aux(_aux),val(nullptr),range{nullptr,equal,nullptr},flag(0){}
SiteState::SiteState(const location& loc, const Id &_aux,char _flag,
		const Expression* min,const Expression* max) : Node(loc),
		type(AUX),aux(_aux),val(nullptr),range{min,nullptr,max},flag(_flag){}

void SiteState::evalLabels(pattern::Signature::LabelSite& site) const{
	for(const auto& lab : labels)
		site.addLabel(lab);
	return;
}

bool SiteState::evalRange(pattern::Environment &env,const simulation::SimContext& context,
		expressions::BaseExpression** expr_values) const{
	//using ast::Expression::VAR;
	//using ast::Expression::FLAGS;
	expr_values[0] = range[0]->eval(env,context,nullptr,
			Expression::FORCE | Expression::CONST);
	expr_values[2] = range[2]->eval(env,context,nullptr,
			Expression::FORCE | Expression::CONST);
	if(range[1] != nullptr)
		expr_values[1] = range[1]->eval(env,context,nullptr,
					Expression::FORCE | Expression::CONST);
	else
		expr_values[1] = expr_values[0];
	//Test
	bool isInt = true;
	for(int i = 0; i < 3; i++)
		switch(expr_values[i]->getType()){
		case expressions::BOOL:
			throw SemanticError("Not a valid value to define a range.",range[i]->loc);
		case expressions::FLOAT:
			isInt = false;
			break;
		default:break;
		}
	return isInt;
}
void SiteState::show( string tabs ) const {
	cout << " ";
	for(list<Id>::const_iterator it = labels.cbegin(); it != labels.cend(); it++) {
		if( it == labels.cbegin() ) cout << "estados: ";

		if( it != labels.cend() && it != labels.cbegin() ) cout << ", ";

		it->show();
	}
}


/****** Class Site ***********/
Site::Site(){}
Site::Site(const location &l,const Id &id) : Node(l), name(id){}
Site::Site(const location &l,const Id &id,const SiteState &s,const Link &lnk):
	Node(l), name(id), stateInfo(s), link(lnk) {};

pattern::Signature::Site& Site::eval(pattern::Environment &env,const simulation::SimContext& context,
		pattern::Signature &sign) const{
	if(name.getString() == "...")
		throw SemanticError("Auto complete site pattern (...) can only be used in RHS.",loc);
	if(link.getType() != Link::LinkType::FREE)
		throw SemanticError("Link status in an Agent signature.",link.loc);
	//short id;
	pattern::Signature::Site* site;
	switch(stateInfo.type){
	case SiteState::EMPTY:
		site = &sign.addSite<pattern::Signature::EmptySite>(name);
		break;
	case SiteState::LABEL:
		site = &sign.addSite<pattern::Signature::LabelSite>(name);
		stateInfo.evalLabels(*static_cast<pattern::Signature::LabelSite*>(site));
		break;
	case SiteState::RANGE:
		try{
			BaseExpression* range[3];
			if(!stateInfo.evalRange(env,context,range)){
				site = &sign.addSite<pattern::Signature::RangeSite<FL_TYPE> >(name);
				static_cast<pattern::Signature::RangeSite<FL_TYPE>*>
					(site)->setBoundaries(range[0]->getValueSafe(context).valueAs<FL_TYPE>(),
						range[2]->getValueSafe(context).valueAs<FL_TYPE>(),range[1]->getValueSafe(context).valueAs<FL_TYPE>());
			}
			else{
				site = &sign.addSite<pattern::Signature::RangeSite<int> >(name);
				static_cast<pattern::Signature::RangeSite<int>*>
					(site)->setBoundaries(range[0]->getValueSafe(context).valueAs<int>(),
						range[2]->getValueSafe(context).valueAs<int>(),range[1]->getValueSafe(context).valueAs<int>());
			}
		}
		catch(std::out_of_range &e){
			throw SemanticError(e.what(),stateInfo.loc);
		}
		break;
	default:
		throw SemanticError("Not a valid value for a site in agent's signature.",loc);
	}
	return *site;
}

pattern::Mixture::Site* Site::eval(pattern::Environment &env,
		const SimContext &context,pattern::Mixture& mix,
		char ptrn_flag) const{
	const pattern::Signature* sign;
	short ag_pos = mix.size()-1,site_id;//mix.size() gives the actual agent-id.
	pattern::Pattern::Agent& agent = mix.getAgent(ag_pos);
	pattern::Pattern::Site* mix_site;
	DepSet deps;

	if(name.getString() == "..."){			//Auto-complete was set for this agent
		if(!(ptrn_flag & Mixture::Info::RHS))
			throw SemanticError("Auto complete site pattern (...) can only be used in RHS.",loc);
		else{
			agent.setAutoFill();
			return nullptr;
		}
	}
	sign = &env.getSignature(agent.getId());
	try{
		site_id = sign->getSiteId(name.getString());
		mix_site = new pattern::Pattern::Site(site_id);
	}
	catch(std::out_of_range &ex){
		throw SemanticError("Site "+name.getString()
				+" is not declared in agent signature of "+
				sign->getName()+"().",loc);
	}
	catch(SemanticError& ex){
		ex.setLocation(loc);throw ex;
	}
	auto& site_sign = sign->getSite(site_id);
	state::BaseExpression* values[3] = {nullptr,nullptr,nullptr};

	switch(stateInfo.type){
	case SiteState::EMPTY:
		break;
	case SiteState::LABEL:
		if(stateInfo.labels.size() > 1)
			throw SemanticError("List of labels are only allowed in agent declaration.",loc);
		if(stateInfo.labels.size() == 1){
			small_id lbl_id;
			try{
				lbl_id = dynamic_cast<const pattern::Signature::LabelSite&>(site_sign)
						.getLabelId(stateInfo.labels.front().getString());
				mix_site->setValue(lbl_id);
			}
			catch(std::out_of_range &e){
				throw SemanticError("Label "+stateInfo.labels.front().getString()+
						" is not defined for site "+name.getString()+
						" of agent "+sign->getName()+"().",loc);
			}
			catch(std::bad_cast &e){
				throw SemanticError("Site "+name.getString()+" is not defined as a labeled site.",loc);
			}
		}
		else{//empty list ? this should have been an SiteState::EMPTY
			throw invalid_argument("Site::eval(): empty list? this should have been an SiteState::EMPTY");
		}
		break;
	case SiteState::RANGE:
		throw SemanticError("Ranges for sites are only allowed in agent declaration.",loc);
		break;
	case SiteState::AUX:
		if(!(ptrn_flag & Mixture::Info::PATTERN) && (stateInfo.range[0] ||
				stateInfo.range[1] || stateInfo.range[2]))
			throw SemanticError("Can't use a pattern here (maybe RHS?).",stateInfo.loc);
		try{
			site_sign.getLimits();
		}
		catch(invalid_argument& e){
			throw SemanticError("Only valued sites can assign auxiliar.",stateInfo.loc);
		}
		if(stateInfo.range[0]){//TODO infinity expressions
			if(stateInfo.flag & SiteState::MIN_EQUAL)
				values[0] = stateInfo.range[0]->eval(env, context, &deps, ptrn_flag,&mix);//TODO reduce these expressions
			else
				values[0] = BaseExpression::makeUnaryExpression(
						stateInfo.range[0]->eval(env, context, &deps, ptrn_flag,&mix),
						BaseExpression::PREV);
		}
		if(stateInfo.range[2]){
			if(stateInfo.flag & SiteState::MAX_EQUAL)
				values[2] = stateInfo.range[2]->eval(env, context, &deps, ptrn_flag,&mix);
			else
				values[2] = BaseExpression::makeUnaryExpression(
						stateInfo.range[2]->eval(env, context, &deps, ptrn_flag,&mix),
						BaseExpression::PREV);
		}
		if(!mix.addAuxCoords(stateInfo.aux.getString(),ag_pos,site_id))
			throw SemanticError("Auxiliary variable "+stateInfo.aux.getString()+
					" already declared in Mixture.",stateInfo.aux.loc);

		if(stateInfo.range[1]){				//case s~{aux = val} in the lhs
			if(values[0] || values[2])
				throw invalid_argument("Site::eval(): cannot declare 'equality' and 'inequality' patterns.");
			values[1] = stateInfo.range[1]->eval
					(env,context,&deps,ptrn_flag|Expression::FORCE,&mix);
		}
		for(auto dep : deps){
			if(dep.type == Dependency::AUX){
				if(dep.aux == stateInfo.aux.getString())
					throw SemanticError("The site pattern-value cannot depends on itself.",loc);
				mix.setPatternAux(dep.aux);
			}
		}
		//If values[1] && values[0|2] throw...
		mix_site->setValuePattern(values);
		break;
	case SiteState::EXPR:
		try{
			site_sign.getLimits();
		}catch(std::invalid_argument &e){
			throw SemanticError("Only valued sites can assign expressions.",stateInfo.loc);
		}
		if(ptrn_flag & Mixture::Info::RHS)
			values[1] = stateInfo.val->eval(env,context,nullptr,ptrn_flag);
		else //LHS or pattern
			values[1] = stateInfo.val->eval(env,context,nullptr,Expression::CONST | ptrn_flag);
		try{
			//cout << agent.toString(env) << " loc = " << loc << endl;
			auto val = values[1]->getValueSafe(context);
			if(! site_sign.isPossibleValue(val))
				throw SemanticError("The value "+val.toString()+
						" is out of the site's bounds.",loc);
		}
		catch(out_of_range &e){/* value can not be obtained */}//TODO check which exceptions
		mix_site->setValuePattern(values);
		break;
	}

	if(site_sign.getType() & pattern::Signature::Site::BIND)//to avoid set link state to FREE
		link.eval(env,*mix_site,mix,site_id,true);
	return mix_site;
}

void Site::show( string tabs ) const {
	cout << tabs << "Site " << name.getString();

	stateInfo.show( tabs );
}

//SiteState::~SiteState(){}

/****** Class Agent **********/

set<two<string>> Agent::binding_sites;

Agent::Agent(){}
Agent::Agent(const location &l,const Id &id,const list<Site> s):
	Node(l), name(id),sites(s) {
	for(auto& site : sites){
		if(site.link.type >= Link::LinkType::BND_ANY)
			Agent::binding_sites.emplace(id.getString(),site.name.getString());
	}
};

void Agent::eval(pattern::Environment &env,const SimContext &context) const{
	//using namespace pattern;
	pattern::Signature& sign = env.declareSignature(name);
	//sign.setId(id);

	for(auto& site : sites){
		auto& site_sign = site.eval(env,context,sign);
		if(binding_sites.count(make_pair(name.getString(),site.name.getString())))
			site_sign.enableBinding();
	}
	return;
}

pattern::Mixture::Agent* Agent::eval(pattern::Environment &env,
		const SimContext &context,pattern::Mixture &mix,
		char ptrn_flag) const {
	short sign_id;
	try{
		sign_id = env.getSignatureId(name.getString());
	}
	catch(std::out_of_range &e){
		throw SemanticError("Agent "+name.getString()+"() has not been declared.",loc);
	}

	pattern::Mixture::Agent* agent = new pattern::Mixture::Agent(sign_id);
	mix.addAgent(agent);
	for(auto &ast_site : sites){
		auto site = ast_site.eval(env,context,mix,ptrn_flag);
		if(site){
			/*if(site->getValueType() == Pattern::EMPTY && site->getLinkType() == Pattern::WILD){
				if(ast_site.stateInfo.aux.getString() == "")
					ADD_WARN("The site "+ast_site.name.getString()+" is useless for the agent"
							" pattern and will be omitted.",ast_site.loc);
				delete site;//useless site
			}
			else*/
				agent->addSite(site);//site destructor called
		}
		else
			agent->setAutoFill();
	}
	return agent;
}

void Agent::show( string tabs ) const {
	tabs += "   ";
	cout << tabs << "Agent " << name.getString() << " {";

	for(list<Site>::const_iterator it = sites.cbegin(); it != sites.cend(); it++){
		cout << endl << "   ";
		it->show( tabs );
	}

	cout << endl << tabs <<  "}" << endl;
}


/****** Class Mixture ************/
Mixture::Mixture(){}

Mixture::Mixture(const location &l,const list<Agent> &m):
	Node(l), agents(m) {};

Mixture::~Mixture(){};

pattern::Mixture* Mixture::eval(pattern::Environment &env,
		const SimContext &context,char ptrn_flag) const{
	pattern::Mixture* mix = new pattern::Mixture(agents.size());
	for(auto& ag_ast : agents){
		auto agent = ag_ast.eval(env,context,*mix,ptrn_flag);
	}
	mix->setComponents();
	if(ptrn_flag & Mixture::Info::PATTERN){
		env.declareMixture(mix);
	}
	return mix;
}

void Mixture::show(string tabs) const {
	for(list<Agent>::const_iterator it = agents.cbegin() ; it != agents.cend() ; it++) {
		it->show(tabs);
	}
}


/****** Class Effect *************/
Effect::Effect():
	action(),n(nullptr),mix(nullptr),set(VarValue()) {};
//INTRO,DELETE
Effect::Effect(const location& l,const Action& a,const Expression* n ,list<Agent>& mix):
	Node(l),action(a),n(n), mix(new Mixture(l,mix)) {};
//UPDATE,UPDATE_TOK
Effect::Effect(const location& l,const Action& a,const VarValue& dec):
	Node(l),action(a),set(dec),n(nullptr),mix(nullptr){};
//CFLOW,CFLOW_OFF
Effect::Effect(const location& l,const Action& a,const Id& id):
	Node(l),action(a),names(1,id),n(nullptr),mix(nullptr) {};
//STOP,SNAPSHOT,FLUX,FLUXOFF,PRINT
Effect::Effect(const location& l,const Action& a,const list<StringExpression>& str):
	Node(l),action(a),string_expr(str),n(nullptr),mix(nullptr) {};
//PRINTF
Effect::Effect(const location& l,const Action& a,const list<StringExpression>& str1,const list<StringExpression>& str2):
	Node(l),action(a),filename(str1),string_expr(str2),n(nullptr),mix(nullptr) {};
//HISTOGRAM
Effect::Effect(const location &l,const list<StringExpression> &str,
		const VarValue &bins,const list<Id>& _vars,const Expression* aux_expr) :
	Node(l),action(HISTOGRAM),n(aux_expr),names(_vars),set(bins),filename(str),mix(nullptr){}

/*Effect::Effect(const Effect &eff):
	Node(eff.loc),action(eff.action),n(nullptr),mix(nullptr) {

	n = eff.n;
	mix = eff.mix;

	switch(action){
	case INTRO:case DELETE:
		//multi_exp = eff.multi_exp;
		break;
	case UPDATE:case UPDATE_TOK:
		set = eff.set;
		break;
	case CFLOW:case CFLOW_OFF:
		name = eff.name;
		break;
	case STOP:case SNAPSHOT:case FLUX:case FLUX_OFF:case PRINT:
		string_expr = eff.string_expr;
		break;
	case PRINTF:
		string_expr = eff.string_expr;
		filename = eff.filename;
		break;
	}
}*/

/*Effect& Effect::operator=(const Effect& eff){
	loc = eff.loc;
	action = eff.action;

	n = eff.n;
	mix = eff.mix;

	switch(action){
	//case INTRO:case DELETE:
	//	multi_exp = eff.multi_exp;
	//	break;
	case UPDATE:case UPDATE_TOK:
		set = eff.set;
		break;
	case CFLOW:case CFLOW_OFF:
		name = eff.name;
		break;
	case STOP:case SNAPSHOT:case FLUX:case FLUX_OFF:case PRINT:
		string_expr = eff.string_expr;
		break;
	case PRINTF:
		string_expr = eff.string_expr;
		filename = eff.filename;
		break;
	}
	return *this;
}*/

simulation::Perturbation::Effect* Effect::eval(pattern::Environment& env,
		const SimContext &context,const BaseExpression* trigger) const {
	auto& vars = context.getVars();
	try{
	switch(action){
	case INTRO:{
		auto mixture = mix->eval(env,context,0);
		for(auto cc : *mixture)
			for(auto ag : *cc)
				for(auto st_sign : env.getSignature(ag->getId()).getSites()){
					auto def_site = new pattern::Pattern::Site(st_sign->getId());
					def_site->setValue(st_sign->getDefaultValue());
					ag->getSite(def_site);
				}
		return new simulation::Intro(n->eval(env, context, nullptr, 0),mixture);
	}
	case DELETE:{
		auto mixture = mix->eval(env,context,Expression::PATTERN | Expression::AUX_ALLOW);
		return new simulation::Delete(n->eval(env, context, nullptr, 0),*mixture);
	}
	case UPDATE:{
		auto id = env.getVarId(set.var.getString());
		if(id > vars.size())
			throw SemanticError("Update rule's rates directly is not implemented yet.",loc);
		if(vars[id]->isConst())
			throw SemanticError("Cannot update constants.",loc);
		pattern::DepSet deps;
		auto set_expr = set.value->eval(env, context, &deps);
		auto upd_eff = new simulation::Update(*vars[id],set_expr);
		if(deps.count(pattern::Dependency(pattern::Dependency::VAR,id)))
			upd_eff->setValueUpdate();
		return upd_eff;
	}
	case UPDATE_TOK:{
		auto id = env.getTokenId(set.var.getString());
		pattern::DepSet deps;
		auto set_expr = set.value->eval(env, context, nullptr);
		auto upd_eff = new simulation::UpdateToken(id,set_expr);
		//if(deps.count(pattern::Dependency(pattern::Dependency::VAR,id)))
		//	upd_eff->setValueUpdate();
		return upd_eff;
	}
	case STOP:
	case SNAPSHOT:
		break;
	case HISTOGRAM:{
		auto bins = 0;
		if(set.value)
			bins = set.value->eval(env, context, nullptr, Expression::PATTERN | Expression::AUX_ALLOW)
					->getValueSafe(context).valueAs<int>();
		else if(!trigger || !dynamic_cast<const expressions::NullaryOperation<bool>*>(trigger))
			ADD_WARN("Use $HISTOGRAM(BINS) to select how many bins to plot.",loc);
		string file;
		list<const state::KappaVar*> k_vars;
		for(auto& var_name : names){
			auto var_id = Id(var_name.loc,var_name.evalLabel(env,context));
			auto k_var = dynamic_cast<const state::KappaVar*>(vars[env.getVarId(var_id)]);
			if(!k_var)
				throw SemanticError("Variable "+var_id.getString()+" is not a Kappa mixture.",var_id.loc);
			k_vars.emplace_back(k_var);
		}
		auto aux_func = n ? n->eval(env, context, nullptr,
				Expression::PATTERN | Expression::AUX_ALLOW,& k_vars.front()->getMix())
				: nullptr;
		for(auto& se : filename)
			file += se.evalConst(env, context);
		simulation::Histogram* eff;
		try{
			eff = new simulation::Histogram(env,bins,file,k_vars,aux_func);
		}
		catch(exception &e){
			throw SemanticError(e.what(),loc);
		}
		return eff;
	}
	case PRINT:
	case PRINTF:
	case CFLOW:
	case CFLOW_OFF:
	case FLUX:
	case FLUX_OFF:
		break;
	}
	}
	catch(invalid_argument &e){
		throw SemanticError(e.what(),loc);
	}
	return nullptr;
}


void Effect::show(string tabs) const {
	tabs += "   ";
	cout << tabs << "effect :" << endl ;
	cout << tabs << "   " << "action:" << action << endl;

	if(n) {
		cout << tabs << "   " << "expression:";
		n->show(tabs+"   ");
		cout << endl;
	}

	if(mix) {
		cout << tabs << "   " << "mixture:" << endl;
		mix->show(tabs+"   ");
	}

	cout << tabs << "   " << "set:" << endl;
	set.show(tabs+"   ");

	cout << tabs << "   " << "name:" << endl;
	//name.show(tabs+"   ");TODO

	// show  string_expr and string_expr2 StringExpression class
	cout << endl;
	cout << tabs << "   " << "string_expr:" << endl;
	short i = 0;
	for(list<StringExpression>::const_iterator it = string_expr.cbegin(); it != string_expr.cend() ; it++) {
		cout << tabs << "      " << ++i << ")" ;
		it->show(tabs);
		cout << endl;
	}

	cout << tabs << "   " << "filename:" << endl;
	i = 0;
	for(list<StringExpression>::const_iterator it = filename.cbegin(); it != filename.cend() ; it++) {
		cout << tabs << "      " << ++i << ")" ;
		it->show();
		cout << endl;
	}
}

Effect::~Effect(){
	switch(action){
	case INTRO: case DELETE:
		//delete n;
		//delete mix;
		break;
	default:
		break;
	}
}


/****** Class Pert *******/
Pert::Pert() : condition(nullptr),until(nullptr){};
Pert::Pert(const location &l,const Expression *cond,const list<Effect> &effs,const Expression* rep):
	Node(l), condition(cond), until(rep), effects(effs){};


void Pert::eval(pattern::Environment& env,const SimContext &context) const {
	expressions::BaseExpression *cond = nullptr,*rep = nullptr;
	pattern::DepSet deps;
	if(condition)
		cond  = condition->eval(env, context, &deps, 0);
	else
		cond = new Constant<bool>(true);

	if(until)
		rep = until->eval(env, context, nullptr, 0);

	auto pert = new simulation::Perturbation(cond,rep,loc,context);

	for(auto &eff : effects){
		pert->addEffect(eff.eval(env,context,cond),context,env);
	}

	env.declarePert(pert);
	for(auto& dep : deps)
		env.addDependency(dep,Deps::PERT, pert->getId());

}

void Pert::show(string tabs) const {
	tabs += "   ";
	cout << "Perturbation {" << endl;

	cout << "condition :" ;
	condition->show(tabs);

	if(until) {
		cout << "until :";
		until->show(tabs);
	}

	for(list<Effect>::const_iterator it = effects.cbegin(); it != effects.cend(); it++){
		cout << endl;
		it->show(tabs);
	}

	cout << endl << "}" << endl;
}

Pert::~Pert() {
	if(until)
		delete until;
};



/****** Class Rate ***************/
Rate::Rate():
	Node(), base(nullptr), reverse(nullptr), fixed(nullptr), unary(nullptr,nullptr) {};
Rate::Rate(const Rate& r) : Node(r.loc),
		base(r.base ? r.base->clone() : nullptr),
		reverse(r.reverse ? r.reverse->clone() : nullptr),
		unary(r.unary),
		volFixed(r.volFixed),fixed(r.fixed){}
Rate& Rate::operator=(const Rate& r) {
	loc = r.loc;
	base = r.base ? r.base->clone() : nullptr;
	reverse = r.reverse ? r.reverse->clone() : nullptr;
	unary = r.unary;
	volFixed = r.volFixed;
	fixed = r.fixed;
	return *this;
}

Rate::Rate(const location &l,const Expression *def,const bool fix):
	Node(l), base(def), reverse(nullptr), fixed(fix), unary(nullptr,nullptr) {};
Rate::Rate(const location &l,const Expression *def,const bool fix,two<const Expression*>un):
	Node(l), base(def), reverse(nullptr), fixed(fix), unary(un) {};
Rate::Rate(const location &l,const Expression *def,const bool fix,const Expression *ope):
	Node(l), base(def), reverse(ope), fixed(fix), unary(nullptr,nullptr) {};
Rate::~Rate(){
	if(base)
		delete base;
	/*if(unary.first)
		delete unary.first;
	if(unary.second)
		delete unary.second;*/
	if(reverse)
		delete reverse;
}

const state::BaseExpression* Rate::eval(pattern::Environment& env,simulation::Rule& r,
		const SimContext &context,two<pattern::DepSet> &deps,bool is_bi) const {
	if(!base)
		throw std::invalid_argument("Base rate cannot be null.");
	auto& lhs = r.getLHS();
	auto base_rate = base->eval(env,context,&deps.first,
			Expression::AUX_ALLOW+Expression::RHS,&lhs);//TODO enum rate?
	r.setRate(base_rate);
	if(is_bi)
		throw SemanticError("Bidirectional rules are deprecated. Please write 2 rules instead.",loc);
	/*	if(unary)  BIDIRECTIONAL RULES ARE DEPRECATED
			throw SemanticError("Cannot define a bidirectional rule with a rate for unary cases.",loc);
		if(deps.first.count(Deps::AUX))
			throw SemanticError("Cannot define a bidirectional rule with a rate depending on auxiliars.",loc);
		if(!reverse){
			ADD_WARN("Assuming same rate for both directions of bidirectional rule.",loc);
		}
		else{
			return reverse->eval(env,context,&deps.second,Expression::AUX_ALLOW);
		}
	}
	else{*/
	if(reverse)
		throw SemanticError("Reverse rate defined for unidirectional rule.",loc);
	if(unary.first){
		auto un_rate = unary.first->eval(env,context,&deps.first,
				Expression::AUX_ALLOW+Expression::RHS,&lhs);
		int radius = 10000;
		if(unary.second){
			radius = unary.second->eval(env,context,&deps.first,Expression::CONST + Expression::AUX_ALLOW)->
					getValueSafe(context).valueAs<int>();//TODO check if vars is only consts
		}
		r.setUnaryRate(make_pair(un_rate,radius));
		env.declareUnaryMix(lhs,radius);
	}
	return nullptr;

}

/****** Class Token **************/
Token::Token() : exp(nullptr) {}
Token::Token(const location &l,const Expression *e,const Id &id):
	Node(l), exp(e), id(id) {};
pair<unsigned,const BaseExpression*> Token::eval(const pattern::Environment& env,
			const SimContext& context,bool neg) const {
	unsigned i;
	try{
		i = env.getTokenId(id.getString());
	}
	catch(std::out_of_range& ex){
		throw SemanticError("Token "+id.getString()+" has not been declared",loc);
	}
	const Expression* expr = neg ? new AlgBinaryOperation(
			location(),exp,new Const(location(),-1),BaseExpression::MULT)
			: exp;
	//pattern::DepSet deps;
	return make_pair(i,expr->eval(env,context,nullptr,Expression::RHS + Expression::AUX_ALLOW));//TODO flags
}

/****** Class RuleSide ***********/
RuleSide::RuleSide() : agents(location(),list<Agent>()){}
RuleSide::RuleSide(const location &l,const Mixture &agents,const list<Token> &tokens):
	Node(l), agents(agents), tokens(tokens) {};

/****** Class Rule ***************/

size_t Rule::count = 0;

Rule::Rule() : bi(false){}//,filter(nullptr){}
Rule::Rule(	const location &l,
		const Id          &label,
		const RuleSide    &lhs,
		const RuleSide    &rhs,
		const bool       arrow,
		/*const Expression* where,*/
		const Rate 		  &rate):
	Node(l), label(label), lhs(lhs), rhs(rhs),
	bi(arrow),/*filter(where),*/rate(rate) {count += bi ? 2 : 1;};
Rule::~Rule() {};

Rule::Rule(const Rule& r) : Node(r.loc),label(r.label),lhs(r.lhs),rhs(r.rhs),
		bi(r.bi)/*,filter(r.filter ? r.filter->clone() : nullptr)*/,rate(r.rate){}

Rule& Rule::operator=(const Rule& r){
	loc = r.loc;
	label = r.label;
	lhs = r.lhs;
	rhs = r.rhs;
	bi = r.bi;
	/*if(r.filter)
		filter = r.filter->clone();
	else
		filter = nullptr;*/
	rate = r.rate;
	return *this;
}

void Rule::eval(pattern::Environment& env,
		const SimContext &context) const{
	//TODO eval name...
	auto& lhs_mix = *lhs.agents.eval(env,context,
			Expression::PATTERN + Expression::LHS + Expression::AUX_ALLOW);//TODO do not allow aux?
	auto& rule = env.declareRule(label,lhs_mix,loc);

	lhs_mix.addInclude(rule.getId());//TODO check if only addinclude in mix on unary rules
	two<pattern::DepSet> deps;//first-> | second<-
	rate.eval(env,rule,context,deps,bi);
	for(auto dep : deps.first){
		if(dep.type == Deps::AUX){
			auto cc_id = lhs_mix.getAuxMap().at(dep.aux).cc_pos;
			lhs_mix.getComponent(cc_id).addRateDep(rule,cc_id);
		}
	}

	auto rhs_mix_p = rhs.agents.eval(env,context,
			Expression::RHS + Expression::AUX_ALLOW);
	for(auto& t : lhs.tokens)
		rule.addTokenChange(t.eval(env,context,true));
	for(auto& t : rhs.tokens)
		rule.addTokenChange(t.eval(env,context));
	if(bi)
		throw SemanticError("Bidirectional rules are deprecated.",loc);
		/*rhs_mix_p->declareAgents(env);
		rhs_mask = rhs_mix_p->setAndDeclareComponents(env);
		auto& rhs_mix = env.declareMixture(*rhs_mix_p);
		delete rhs_mix_p;
		rule.setRHS(&rhs_mix,bi);
		auto reverse_label = label.getString()+" @bckwrds";
		auto& inverse_rule = env.declareRule(Id(label.loc,reverse_label),rhs_mix,loc);
		inverse_rule.setRHS(&lhs_mix,bi);
		inverse_rule.difference(env,rhs_mask,lhs_mask,vars);
		inverse_rule.setRate(reverse);
		rhs_mix.addInclude(inverse_rule.getId());
		for(auto& t : rhs.tokens)
			inverse_rule.addTokenChange(t.eval(env,vars,true));
		for(auto& t : lhs.tokens)
			inverse_rule.addTokenChange(t.eval(env,vars));
		for(auto& dep : deps.second)
			env.addDependency(dep,Deps::RULE,inverse_rule.getId());
	}
	else{*/
		rule.setRHS(rhs_mix_p,bi);
	//}
	//DEBUG cout << lhs_mix.toString(env,lhs_mask) << " -> " << rhs_mix_p->toString(env,rhs_mask) << endl;
	rule.difference(env,context);
	for(auto& dep : deps.first)
		env.addDependency(dep,Deps::RULE,rule.getId());
}

size_t Rule::getCount(){
	return count;
}


} /* namespace ast */
}






