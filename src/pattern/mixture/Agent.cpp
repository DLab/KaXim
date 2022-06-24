/*
 * Agent.cpp
 *
 *  Created on: Jan 21, 2019
 *      Author: naxo100
 */

#include "Agent.h"
#include "../Environment.h"
#include "../../expressions/Vars.h"
#include "../../state/Node.h"

namespace pattern {

using namespace expressions;
/*************** Class Agent ********************/

Pattern::Agent::Agent(short_id sign_id) : signId(sign_id),autoFill(false){}
Pattern::Agent::~Agent(){};


const Pattern::Site& Pattern::Agent::addSite(Site* site){
	auto it_ins = interface.emplace(*site);
	if(!it_ins.second)
		throw SemanticError("The site is already declared in this Agent.",yy::location());
	site->id = -1;
	delete site;
	return *it_ins.first;
}

const Pattern::Site& Pattern::Agent::getSite(Site* site) {
	auto& new_site = *interface.emplace(*site).first;
	site->id = -1;
	delete site;
	return new_site;
}
const Pattern::Site& Pattern::Agent::getSiteSafe(small_id id) const {
	static Site empty_site(-1);
	static Site autofill_site(AUTOFILL);
	auto it = interface.find(id);
	if(it != interface.end())
		return *it;
	else if(autoFill)
		return autofill_site;
	else
		return empty_site;
}

void Pattern::Agent::setAutoFill() {
	autoFill = true;
}

short_id Pattern::Agent::getId() const {
	return signId;
}


const set<Pattern::Site>::const_iterator Pattern::Agent::begin() const{
	return interface.begin();
}
const set<Pattern::Site>::const_iterator Pattern::Agent::end() const{
	return interface.end();
}

bool Pattern::Agent::operator ==(const Agent &a) const {
	if(this == &a)
		return true;
	if(signId == a.signId && interface.size() == a.interface.size()){
		for(auto &site : interface){
			try{
				if(! (site == a.getSite(site.getId())) )
					return false;
			}
			catch(std::out_of_range &e){
				return false;//TODO may be similar
			}
		}
	}
	else
		return false;
	return true;
}

bool Pattern::Agent::testEmbed(const Agent &rhs_ag,
		const simulation::SimContext &context,list<small_id>& sites_to_test) const {
	if(this == &rhs_ag)
		return true;
	if(signId != rhs_ag.signId)
		return false;

	for(auto &site : rhs_ag.interface){//every site in RHS-ag must be tested
		auto& lhs_site = getSiteSafe(site.getId());
		if(lhs_site.testEmbed(site,context)){
			if(site.getLinkType() == BND &&
					lhs_site.getLinkType() == BND )
				sites_to_test.emplace_back(site.getId());
		}
		else
			return false;
	}
	return true;
}


void Pattern::Agent::addCc(const Mixture::Component* cc,small_id id) const{
	includedIn.emplace_back(cc,id);
}
const list<pair<const Mixture::Component*,small_id> >& Pattern::Agent::getIncludes() const{
	return includedIn;
}


string Pattern::Agent::toString(const Environment& env, short mixAgId,
		map<ag_st_id,short> bindLabels  ) const {
	string out = "", glue = ",";

	const Signature& sign = env.getSignature(signId);
	out += sign.getName() + "(";

	// inspect interface
	for(auto& site : interface) {
		out += site.toString(signId,mixAgId,bindLabels,env);
	}

	// remove last glue
	if( out.substr(out.size()-glue.size(), out.size()) == glue )
		out = out.substr(0, out.size()-glue.size());

	out += ")";

	return out;
}


/*******************************************************/
/******************* class Site ************************/
/*******************************************************/


Pattern::Site::~Site() {
	if(id == small_id(-1))
		return;
	if(values[0])
		delete values[0];
	if(values[1])
		delete values[1];
}

/*Mixture::Site::Site(expressions::SomeValue val,LinkType lt) : label(0),link_type(lt),lnk_ptrn(-1,-1),
		values{nullptr,nullptr,nullptr}{
	switch(val.t){
	case expressions::SMALL_ID:
		label = val.smallVal;break;
	case expressions::FLOAT:
		values[1] = new expressions::Constant<FL_TYPE>(val.fVal);break;
	case expressions::INT:
		values[1] = new expressions::Constant<int>(val.iVal);break;
	default:
		throw invalid_argument("Not a valid value type for agent site.");
	}
}*/
/*
Pattern::Site::Site(const Site& site) : value_type(site.value_type),
		label(site.label),link_type(site.link_type),lnk_ptrn(site.lnk_ptrn){
	for(int i = 0; i < 2; i++)
		if(site.values[i]){
			if(dynamic_cast<const state::Variable*>(site.values[i]))
				values[i] = site.values[i];
			else
				values[i] = site.values[i]->clone();
		}
		else
			values[i] = nullptr;
}

Pattern::Site::~Site(){
	for(int i = 0; i < 2; i++)
		if(values[i] && !dynamic_cast<const state::Variable*>(values[i]))
			delete values[i];
}

Pattern::Site& Pattern::Site::operator=(const Site& site) {
	label = site.label;
	link_type = site.link_type;
	lnk_ptrn = site.lnk_ptrn;
	for(int i = 0; i < 2; i++)
		if(site.values[i]){
			if(dynamic_cast<const state::Variable*>(values[i]))
				values[i] = site.values[i];
			else
				values[i] = site.values[i]->clone();
		}
		else
			values[i] = nullptr;
	return *this;
}
*/

void Pattern::Site::setValue(SomeValue val){
	//auto f = &(testMatchOpt<EMPTY,WILD>);
	switch(val.t){
	case Type::SMALL_ID:
		label = val.smallVal;
		value_type = LABEL;
		break;
	case Type::FLOAT:
		if(values[0])
			delete values[0];
		values[0] = new Constant<FL_TYPE>(val.fVal);
		value_type = EQUAL;
		break;
	case Type::INT:
		if(values[0])
			delete values[0];
		values[0] = new Constant<int>(val.iVal);
		value_type = EQUAL;
		break;
	case Type::NONE:
		value_type = EMPTY;
		break;
	default:
		throw invalid_argument("Mixture::Site::setValue(): invalid value type.");
	}
	setMatchFunction();
}

void Pattern::Site::setValuePattern(BaseExpression** vals){
	values[0] = vals[0];
	values[1] = vals[2];
	if(vals[0]){
		if(vals[2]){
			if(vals[1])
				value_type = DIFF;
			else
				value_type = BETWEEN;
		} else
			value_type = GREATER;
	}
	else{
		if(vals[2])
			value_type = SMALLER;
		else if(vals[1]){
			value_type = EQUAL;
			values[0] = vals[1];
		}
		else
			value_type = EMPTY;
	}
	setMatchFunction();
}

void Pattern::Site::setBindPattern(LinkType type,small_id ag_id,small_id site_id){
	link_type = type;
	if(ag_id != small_id(-1))
		lnk_ptrn = ag_st_id(ag_id,site_id);
	setMatchFunction();
}

/*
bool Mixture::Site::isEmptySite() const{
	return value_type == EMPTY ;//&& values[1] == nullptr;
}
bool Mixture::Site::isExpression() const{
	throw invalid_argument("this method is wrong");//return label == AUX || values[1];
}*/

//test if mix_site match with value or inequation
bool Pattern::Site::testValue(const state::SomeValue& val,const simulation::SimContext &context) const {
	//TODO ignore vars when using for check influence
	if(val.t == NONE)
		return true;
	switch(value_type){
	case EMPTY:
		return true;
	case LABEL:
		if(val.t == expressions::SMALL_ID)
			return val.smallVal == label;
		else
			throw invalid_argument("Site::testValue: incompatible types.");
		break;
	case EQUAL:
		try {
			return values[0]->getValueSafe(context) == val;
		}
		//cannot evaluate so may be true
		catch(std::exception &e){/*invalid_argument,out_of_range*/}
		break;
	case GREATER:case SMALLER:case BETWEEN:{
		auto fl_val = val.valueAs<FL_TYPE>();
		try {
			if(values[0])
				if(values[0]->getValueSafe(context).valueAs<FL_TYPE>() > fl_val)
					return false;
		}catch(std::exception &e){/*invalid_argument,out_of_range*/}
		try {
			if(values[1])
				if(values[1]->getValueSafe(context).valueAs<FL_TYPE>() < fl_val)
					return false;
		} catch(std::exception &e){/*invalid_argument,out_of_range*/}
		return true;
	}
	}
	return true;
}

/*
//test if mix_site match with value or inequation
bool Mixture::Site::testValueOpt(const state::SomeValue& val,const state::State& state,
		const expressions::AuxMap& aux_map) const {
	//TODO ignore vars when using for check influence
	if(val.t == expressions::SMALL_ID)
		return val.smallVal == label;
	else{
		if(values[1]){
			if(!values[1]->isAux())//TODO only if aux expression are stored
				try{//TODO faster comparison??
					return values[1]->getValue(state,std::move(aux_map)).valueAs<FL_TYPE>() == val.valueAs<FL_TYPE>();
				}
				catch(std::out_of_range &e){
					if(aux_map.size())//aux_values is not empty so we miss some aux
						throw out_of_range("testValueOpt(): Auxiliar or Var was not found.");
					else
						throw out_of_range("testValueOpt(): no auxiliars given.");//no aux?? something wrong here
				}
		}
	}
	auto fl_val = val.valueAs<FL_TYPE>();
	if(values[0])
		if(values[0]->getValue(state,move(aux_map)).valueAs<FL_TYPE>() > fl_val)
			return false;
	if(values[2])
		if(values[2]->getValue(state,move(aux_map)).valueAs<FL_TYPE>() < fl_val)
			return false;
	return true;
}*/

bool Pattern::Site::operator ==(const Site &s) const{
	if(value_type != s.value_type)
		return false;
	switch(value_type){
	case BETWEEN:
	case SMALLER:
	case GREATER:
		for(int i = 0; i < 2; i++)
			if(values[i] != s.values[i]){
				if(values[i] && s.values[i]){
					if(*values[i] != *s.values[i])
						return false;
				}
				else
					return false;
			}
		break;
	case EQUAL:
		if(*values[0] != *s.values[0])
			return false;
		break;
	case EMPTY:
		break;
	case LABEL:
		if(label != s.label)
			return false;
		break;
	default:
		throw invalid_argument("Pattern::Site::operator==(): not a valid value type.");
	}
	if(s.link_type == link_type){
		if(link_type == LinkType::BND_PTRN && s.lnk_ptrn != lnk_ptrn)//bind or bind_to //TODO PATH
			return false;
	}
	else
		return false;

	return true;
}

/*test if this site-pattern may embed the rhs_site*/
bool Pattern::Site::testEmbed(const Site &rhs_site,const simulation::SimContext &context) const{
	try{
		switch(rhs_site.value_type){//the possible values of an RHS site
		case EQUAL:
			if(!testValue(rhs_site.values[0]->getValueSafe(context),context))
				return false;
			break;
		case LABEL:
			if(!testValue(SomeValue(rhs_site.label),context))
				return false;
			break;
		case EMPTY:
			break;
		default:
			throw invalid_argument("Mixture::Site::testEmbed(): not a valid RHS site value.");
		}
	}
	catch(out_of_range &e){/*cannot calculate exact values, so maybe yes*/}

	if(link_type == WILD)
		return true;
	switch(rhs_site.link_type){//the possible values of RHS site
	case FREE:
		if(link_type != FREE)
			return false;
		break;
	case BND:
		if(link_type == FREE ||( link_type == BND && lnk_ptrn != rhs_site.lnk_ptrn))
			return false;
		break;
	case BND_PTRN://cannot be a changing RHS
		if(lnk_ptrn != rhs_site.lnk_ptrn)
			return false;
		break;
	case BND_ANY://cannot be a changing RHS TODO implementation if calling in other circumstances
		if(link_type == FREE)
			return false;
		break;
	default:break;//WILD | PATH
	}
	return true;
}

/*
int Mixture::Site::compareLinkPtrn(ag_st_id ptrn) const{
	if(isBindToAny()){
		if(ptrn.first == small_id(-1))
			return 0;
		else
			return 1;
	}
	else{
		if(ptrn.first == small_id(-1))
			return -1;
		else
			if(lnk_ptrn == ptrn)
				return link_type == LinkType::BIND_TO ? 1 : 0;//need to compare with other type
			else
				throw False();
	}
}

bool Mixture::Site::isBindToAny() const{
	return link_type == LinkType::BIND_ANY;
}*/

list<Pattern::Site::Diff> Pattern::Site::difference(const Pattern::Site& rhs) const {
	list<Diff> actions;
	Diff d{},d2{};
	switch(rhs.value_type){
	case AUTOFILL:
		return actions;
	case LABEL:
		if(value_type == EMPTY)
			d.warning = "Application of rule '%s' will induce a null.action when "
				"applied to an agent(site) %s(%s) that has the same label.";
		else if(label == rhs.label)
			break;
		d.t = CHANGE; d.new_label = rhs.label;
		actions.emplace_back(d);
		break;
	case EQUAL:
		if(value_type == EMPTY)
			d.warning = "Application of rule '%s' will induce a null-action when "
				"applied to an agent(site) %s(%s) that has the same value.";
		else if(value_type == EQUAL && *values[0] == *rhs.values[0])
			break;
		d.t = ASSIGN; d.new_value = rhs.values[0];
		actions.emplace_back(d);
		break;
	case EMPTY:
		if(value_type == rhs.value_type)
			break;
		//no break
	case SMALLER:case GREATER:case BETWEEN:
		d.warning = "In rule '%s': invalid pattern of %s(%s) "
			"in the right side.";
		d.t = ERROR;
		actions.emplace_back(d);
	}
	switch(rhs.link_type){
	case FREE:
		if(link_type == FREE)
			break;
		if(link_type == WILD){
			d2.warning = "Application of rule '%s' will induce side-effect/null-action "
				"when applied to a bind/free site %s(%s).";
			d2.side_effect = true;
		} else if(link_type == BND_ANY){
			d2.warning = "Application of rule '%s' will induce side-effect when breaking "
				"the semi-link in site %s(%s).";
			d2.side_effect = true;
		} else if(link_type == BND_PTRN)
			d2.new_label = 1;//this unbind will induce new injection on target pattern
		d2.t = UNBIND;
		actions.emplace_back(d2);
		break;
	case BND:
		switch(link_type){
		case BND://this Action need to be approved using graph.
			d2.new_label = 1;
			break;
		case WILD:case BND_ANY:case BND_PTRN://TODO BND_PTRN effects
			d2.side_effect = true;
			d2.warning = "Application of rule '%s' will induce side-effect/"
					"null-action when site %s(%s) is bind.";
			break;
		default:break;
		}
		d2.t = LINK;
		actions.emplace_back(d2);
		break;
	default://BIND_PTRN:BND_ANY:WILD:
		if(link_type != rhs.link_type || lnk_ptrn != rhs.lnk_ptrn){
			d2.warning = "In rule '%s': invalid use of binding pattern in RHS agent %s(%s).";
			d2.t = ERROR;
			actions.emplace_back(d2);
		}
	}
	return actions;
}



#define ADD_DEP(ID,POS) port_list.emplace(ID,st->getDeps().POS)

#define TEST_EMPTY
#define TEST_LABEL \
	if(st->getValue().smallVal != label) return nullptr; \
	ADD_DEP(id,first)
#define TEST_EQUAL \
	if(values[0]->getValueSafe(context).valueAs<FL_TYPE>() != st->getValue().valueAs<FL_TYPE>()) \
		return nullptr; \
	ADD_DEP(id,first)
#define TEST_DIFF\
	if(values[0]->getValueSafe(context).valueAs<FL_TYPE>() == st->getValue().valueAs<FL_TYPE>()) \
		return nullptr; \
	ADD_DEP(id,first)
#define TEST_GREATER \
	if(values[0]->getValueSafe(context).valueAs<FL_TYPE>() > st->getValue().valueAs<FL_TYPE>()) \
		return nullptr; \
	ADD_DEP(id,first)
#define TEST_SMALLER \
	if(values[1]->getValueSafe(context).valueAs<FL_TYPE>() < st->getValue().valueAs<FL_TYPE>()) \
		return nullptr; \
	ADD_DEP(id,first)
#define TEST_BETWEEN \
	if(values[0]->getValueSafe(context).valueAs<FL_TYPE>() > st->getValue().valueAs<FL_TYPE>() || \
			values[1]->getValueSafe(context).valueAs<FL_TYPE>() < st->getValue().valueAs<FL_TYPE>()) \
		return nullptr; \
	ADD_DEP(id,first)


#define TEST_WILD
#define TEST_FREE \
	if(st->getLink().first) return nullptr; \
	ADD_DEP(lnk_id,second)
#define TEST_BND \
	auto lnk = st->getLink().first; \
	if(lnk) { ADD_DEP(lnk_id,second); return lnk;} \
	else return nullptr
#define TEST_BND_PTRN \
	auto lnk = st->getLink(); \
	if(lnk.first) { if(lnk.first->getId() != lnk_ptrn.first || lnk.second != lnk_ptrn.second) \
		return nullptr; \
	}else return nullptr; \
	ADD_DEP(lnk_id,second)
#define TEST_BND_ANY \
	if(!st->getLink().first) return nullptr; \
	ADD_DEP(lnk_id,second)

state::Node* empty_node = new state::Node();

#define TEST_EMBED_OPT(VT,LT)												\
template <>																	\
state::Node* Pattern::Site::testMatchOpt<Pattern::VT,Pattern::LT>	\
		(const state::InternalState* st,map<int,InjSet*>& port_list,			\
		const simulation::SimContext &context) const							\
	{ TEST_##VT;TEST_##LT;return empty_node; }


TEST_EMBED_OPT(EMPTY,WILD)
TEST_EMBED_OPT(EMPTY,FREE)
TEST_EMBED_OPT(EMPTY,BND)
TEST_EMBED_OPT(EMPTY,BND_PTRN)
TEST_EMBED_OPT(EMPTY,BND_ANY)
TEST_EMBED_OPT(LABEL,WILD)
TEST_EMBED_OPT(LABEL,FREE)
TEST_EMBED_OPT(LABEL,BND)
TEST_EMBED_OPT(LABEL,BND_PTRN)
TEST_EMBED_OPT(LABEL,BND_ANY)
TEST_EMBED_OPT(EQUAL,WILD)
TEST_EMBED_OPT(EQUAL,FREE)
TEST_EMBED_OPT(EQUAL,BND)
TEST_EMBED_OPT(EQUAL,BND_PTRN)
TEST_EMBED_OPT(EQUAL,BND_ANY)
TEST_EMBED_OPT(DIFF,WILD)
TEST_EMBED_OPT(DIFF,FREE)
TEST_EMBED_OPT(DIFF,BND)
TEST_EMBED_OPT(DIFF,BND_PTRN)
TEST_EMBED_OPT(DIFF,BND_ANY)
TEST_EMBED_OPT(GREATER,WILD)
TEST_EMBED_OPT(GREATER,FREE)
TEST_EMBED_OPT(GREATER,BND)
TEST_EMBED_OPT(GREATER,BND_PTRN)
TEST_EMBED_OPT(GREATER,BND_ANY)
TEST_EMBED_OPT(SMALLER,WILD)
TEST_EMBED_OPT(SMALLER,FREE)
TEST_EMBED_OPT(SMALLER,BND)
TEST_EMBED_OPT(SMALLER,BND_PTRN)
TEST_EMBED_OPT(SMALLER,BND_ANY)
TEST_EMBED_OPT(BETWEEN,WILD)
TEST_EMBED_OPT(BETWEEN,FREE)
TEST_EMBED_OPT(BETWEEN,BND)
TEST_EMBED_OPT(BETWEEN,BND_PTRN)
TEST_EMBED_OPT(BETWEEN,BND_ANY)




#define CASE2(VT,LT) case LT:testMatchFunc = \
		&Pattern::Site::testMatchOpt<Pattern::VT,Pattern::LT>;break;

#define CASE(VT)				\
	case VT:					\
		switch(link_type){		\
		CASE2(VT,WILD)			\
		CASE2(VT,FREE)			\
		CASE2(VT,BND_ANY)		\
		CASE2(VT,BND_PTRN)		\
		CASE2(VT,BND)			\
		default:break;			\
		}break;

void Pattern::Site::setMatchFunction(){
	set<InjSet*> s;
	switch(value_type){
		CASE(EMPTY)
		CASE(LABEL)
		CASE(EQUAL)
		CASE(DIFF)
		CASE(GREATER)
		CASE(SMALLER)
		CASE(BETWEEN)
	default:
		break;
	}
}


string Pattern::Site::toString(small_id sign_id, small_id ag_pos,
		const map<ag_st_id,short>& bindLabels, const Environment &env) const {
	string out;
	auto& sign = env.getSignature(sign_id).getSite(id);
	out += sign.getName(); //site name

	switch(value_type){
	case EMPTY:break;
	case EQUAL:
		out += "~{ ";
		if(values[0])
			out += values[0]->toString();
		out += " }";
		break;
	case LABEL:{
		auto labelSite = dynamic_cast<const Signature::LabelSite*>(&sign);
		if(labelSite)
			out += "~" + labelSite->getLabel(label);
		else
			throw invalid_argument("Pattern::Site::toString(): not a valid label site.");
		break;}
	default:
		out += "~{ ";
		if(values[0]){
			out +=values[0]->toString() + " <= AUX";
			if(values[1])
				out += " <= " + values[1]->toString();
		}
		else if(values[1])
			out += "AUX <= " + values[1]->toString();
		out += " }";
		break;
	}

	switch(link_type) {
	case LinkType::FREE :
		break;
	case LinkType::BND :
		if ( bindLabels.size() > 0 && bindLabels.count(ag_st_id(ag_pos,id) ) )
			out += "!" + to_string( bindLabels.at(ag_st_id(ag_pos,id)) );
		else{
			auto& signature2bind = env.getSignature( lnk_ptrn.first );
			auto& site2bind = signature2bind.getSite( lnk_ptrn.second );
			out += "!{" + site2bind.getName() + "." + signature2bind.getName()+"}";
			}
		break;
	case LinkType::BND_PTRN :{
		auto& signature2bind = env.getSignature( lnk_ptrn.first );
		auto& site2bind = signature2bind.getSite( lnk_ptrn.second );
		out += "!"+ site2bind.getName() + "." + signature2bind.getName();
		break;}
	case LinkType::BND_ANY:
		out += "!_";
		break;
	case LinkType::PATH :
		out += "!*";
		break;
	case LinkType::WILD :
		if(sign.getType() & Signature::Site::BIND)
			out += "?";
		break;
	}

	return out + ",";
}


/*template <>
const state::Node* Mixture::Site::testEmbedOpt<Pattern::EMPTY,Pattern::LinkType::FREE>(
		state::InternalState* st,set<InjSet*> port_list,
		const state::State& state,expressions::AuxMap& aux_map) const {}*/






/******************* class LabelSite *******************/

/*const state::Node* LabelSite::testEmbedOpt(const state::Node* node,set<InjSet*> port_list,
			const state::State& state,expressions::AuxMap& aux_map) const {
	if(node->getInternalState(id).smallVal != label)
		return nullptr;
	return node;
}*/

/******************* class ValueSite *******************/

/*const state::Node* ValueSite::testEmbedOpt(const state::Node* node,set<InjSet*> port_list,
			const state::State& state,expressions::AuxMap& aux_map) const {
	if(node->getInternalState(id).smallVal != label)
		return nullptr;
	return node;
}*/

/******************* class LinkSite ********************/

/*const state::Node* LabelSite::testEmbedOpt(const state::Node* node,set<InjSet*> port_list,
			const state::State& state,expressions::AuxMap& aux_map) const {
	if(node->getInternalState(id).smallVal != label)
		return nullptr;
	return node;
}*/

/******************* class LabelLinkSite ***************/

/*const state::Node* LabelSite::testEmbedOpt(const state::Node* node,set<InjSet*> port_list,
			const state::State& state,expressions::AuxMap& aux_map) const {
	if(node->getInternalState(id).smallVal != label)
		return nullptr;
	return node;
}*/

/******************* class ValueLinkSite ***************/

/*const state::Node* LabelSite::testEmbedOpt(const state::Node* node,set<InjSet*> port_list,
			const state::State& state,expressions::AuxMap& aux_map) const {
	if(node->getInternalState(id).smallVal != label)
		return nullptr;
	return node;
}*/







} /* namespace pattern */
