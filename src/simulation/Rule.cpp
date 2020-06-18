/*
 * Rule.cpp
 *
 *  Created on: May 9, 2017
 *      Author: naxo
 */

#include <cstring>
#include "../util/Exceptions.h"
#include "../util/Warning.h"
#include "Rule.h"
#include "../pattern/Environment.h"
#include "../grammar/ast/Basics.h"
#include "../expressions/Vars.h"
#include "../pattern/mixture/Agent.h"
#include "../pattern/mixture/Component.h"
#include "../state/State.h"
#include "../matching/InjRandSet.h"
#include <boost/format.hpp>

namespace simulation {

Rule::Rule(int _id,const string& nme, Mixture& mix,const yy::location& _loc) : loc(_loc),id(_id),name(nme),
		lhs(mix),rhs(nullptr),isRhsDeclared(false),rate(nullptr),matchCount(0){}

Rule::~Rule() {
	if(!isRhsDeclared)
		delete rhs;
	if (!dynamic_cast<const state::Variable*>(rate))
		delete rate;
	for(auto node : newNodes)
		delete node;
}


int Rule::getId() const{
	return id;
}

const string& Rule::getName() const {
	return name;
}

const pattern::Mixture& Rule::getLHS() const {
	return lhs;
}
pattern::Mixture& Rule::getLHS() {
	return lhs;
}

const BaseExpression& Rule::getRate() const {
	return *rate;
}
/*const BaseExpression::Reduction& Rule::getReduction() const {
	return basic;
}
const BaseExpression::Reduction& Rule::getUnaryReduction() const {
	return unary;
}*/

void Rule::setRHS(Mixture* mix,bool is_declared){
	rhs = mix;
	isRhsDeclared = is_declared;
}
void Rule::setRate(const BaseExpression* r){
	if(rate)
		delete rate;
	rate = r;
	//if(r->getVarDeps() & BaseExpression::AUX)
	//	basic = r->factorize();
}
void Rule::setUnaryRate(pair<const BaseExpression*,int> u_rate ){
	unaryRate = u_rate;
	//if(u_rate.first->getVarDeps() & BaseExpression::AUX)
	//	unary = u_rate.first->factorize();
}


const list<pair<unsigned,const BaseExpression*> > Rule::getTokenChanges() const{
	return tokenChanges;
}
void Rule::addTokenChange(pair<unsigned,const BaseExpression*> tok) {
	tokenChanges.emplace_back(tok);
}



/** Rule::difference()
 *
 * translate difference between LHS and RHS to Actions
 * Also produce Changes to calculate candidates for influence map
 *
 *
 */
void Rule::difference(const Environment& env, const VarVector& consts){
	small_id ag_pos;
	matchCount = 0;
	//modify nodes
	//string s_i;
	expressions::EvalArguments<true> args(0,&consts);

	for(ag_pos = 0; ag_pos < lhs.size(); ag_pos++){//for each agent in LHS
		auto& lhs_ag = lhs.getAgent(ag_pos);
		if(ag_pos >= rhs->size()) 					//no agent to match in the RHS
			break;
		auto& rhs_ag = rhs->getAgent(ag_pos);
		if(lhs_ag.getId() != rhs_ag.getId())		//the RHS agent type doesn't match
			break;

		auto& ag_sign = env.getSignature(lhs_ag.getId());

		if(lhs_ag.size() < rhs_ag.size())
			throw SemanticError("In rule '"+name+"', agent pattern n°"+to_string(ag_pos)+
					":\n\tRHS agent cannot have more explicit sites than LHS.",loc);
		else if(lhs_ag.size() > rhs_ag.size() && !rhs_ag.isAutoFill())
			throw SemanticError("In rule '"+name+"', agent pattern n°"+to_string(ag_pos)+
					":\n\tRHS agent has to declare every site in LHS or omit with '...'.",loc);
		for(auto& rhs_site : rhs_ag){
			auto st_id = rhs_site.getId();
			auto& lhs_site = [&]() -> const Pattern::Site& { try {return lhs_ag.getSite(st_id);}
					catch(out_of_range& e){throw SemanticError("In rule '"+name+"', agent pattern n°"
							+to_string(ag_pos)+":\n\tLHS agent has to declare every site in RHS.",loc);}}();
			auto& st_sign = ag_sign.getSite(st_id);

			auto diff_list = lhs_site.difference(rhs_site);
			for(auto& diff : diff_list){
				auto ag_coords = lhs.getAgentCoords(ag_pos);
				//		 type  |ag-pos   |site |is-new|side-effect
				Action a{diff.t,ag_coords,st_id,false,diff.side_effect};
				switch(diff.t){
				case ActionType::ERROR:
					throw SemanticError((boost::format(diff.warning)
						% name % ag_sign.getName() % st_sign.getName()).str(),loc);
				case ActionType::CHANGE:
					a.new_label = diff.new_label;
					script.emplace_back(a);
					break;
				case ActionType::ASSIGN:
					a.new_value = diff.new_value;
					script.emplace_back(a);
					break;
				case ActionType::LINK:{
					auto rhs_coords = rhs->getAgentCoords(ag_pos);
					auto rhs_lnk = rhs->follow(rhs_coords,st_id);
					auto trgt_pos = rhs->getAgentPos(ag_st_id(rhs_coords.first,rhs_lnk.first));
					if(ag_pos == trgt_pos)
						throw SemanticError("In rule "+name+
								": self bindings are useless in kappa-models.",loc);
					else if(ag_pos < trgt_pos)
						break;//This avoids to evaluate binds to new agents
					if( diff.new_label ){//new_label is 1 if both sites are BIND
						if(trgt_pos == lhs.getAgentPos(lhs.follow(ag_coords,st_id))
								&& rhs_lnk.second == lhs.follow(ag_coords,st_id).second)
							break;//The bind in RHS is equal to LHS so no action
						else
							diff.warning = "Application of rule '%s' will induce a bind permutation on %s(%s).";
					}
					auto trgt_coords = lhs.getAgentCoords(trgt_pos);
					auto trgt_lnk_type = lhs.getAgent(trgt_pos).getSite(rhs_lnk.second).getLinkType();
					a.chain = new Action{ActionType::LINK,trgt_coords,rhs_lnk.second,
						false,trgt_lnk_type != Pattern::FREE};//TODO replace for unbind -> bind ???
					script.emplace_back(a);
					break;
				}
				case ActionType::UNBIND:
					try{
						auto lhs_lnk = lhs.follow(ag_coords,st_id);
						auto trgt_pos = lhs.getAgentPos(ag_st_id(ag_coords.first,lhs_lnk.first));
						if(trgt_pos < ag_pos)
							break;//This avoids to evaluate unbinds twice
						a.chain = new Action{ActionType::UNBIND,lhs.getAgentCoords(trgt_pos),lhs_lnk.second,false,false};
					}
					catch(out_of_range& e){/*the broken-bind is not fully specified.*/
						a.new_label = diff.new_label;
					}
					script.emplace_back(a);
					break;
				default:
					throw invalid_argument("Rule::difference(): invalid ActionType.");
				} //switch
				if(diff.warning != "")
					ADD_WARN((boost::format(diff.warning) % name %
						ag_sign.getName() % st_sign.getName()).str(),loc);
			}//for diff
		}//for site
	}//for agent
	matchCount = ag_pos;

	//new nodes
	if(matchCount < rhs->size())
		newNodes.reserve(rhs->size()-matchCount);
	for(ag_pos = matchCount; ag_pos < rhs->size(); ag_pos++){
		auto ag_coords = rhs->getAgentCoords(ag_pos);
		auto& rhs_ag = rhs->getAgent(ag_pos);
		auto& ag_sign = env.getSignature(rhs_ag.getId());
		auto new_ag = new state::Node(env.getSignature(rhs_ag.getId()));
		newNodes.emplace_back(new_ag);
		for(auto& st_sign : ag_sign.getSites()){
			auto st_id = st_sign->getId();
			auto def_site = new Pattern::Site(st_id);
			def_site->setValue(st_sign->getDefaultValue());
			auto& site = rhs_ag.getSite(def_site);//get or create the site in rhs new-agent
			auto diff_list = Pattern::Site(0).difference(site);
			for(auto& diff : diff_list){
				switch(diff.t){
				case ActionType::CHANGE:
					new_ag->setInternalValue(st_id,diff.new_label);
					break;
				case ActionType::ASSIGN:
					try{
						auto val = site.getValue(args);
						new_ag->setInternalValue(st_id,val);
					} catch(out_of_range& e){//is not const value
						Action a;
						a.t = ActionType::ASSIGN;
						a.trgt_ag = ag_coords;//TODO
						a.trgt_st = st_id;a.is_new = true;
						a.new_value = diff.new_value;
						script.emplace_back(a);
					}
					catch(invalid_argument& e){//not a valued site
						cout << e.what();
						throw invalid_argument("Rule::difference():trying to"
								" set numeric value on a not valued site.");
					}
					break;
				case ActionType::LINK:{
					auto lnk = rhs->follow(ag_coords,st_id);
					auto trgt_pos = rhs->getAgentPos(ag_st_id(ag_coords.first,lnk.first));
					if(ag_pos == trgt_pos)
						throw SemanticError("In rule "+name+
								": self bindings are useless in kappa-models.",loc);
					if(trgt_pos >= lhs.size()){
						if(trgt_pos > ag_pos){//bind between 2 new nodes
							newNodes[ag_pos - matchCount]->setInternalLink(st_id,newNodes[trgt_pos-matchCount],lnk.second);
							newNodes[trgt_pos - matchCount]->setInternalLink(lnk.second,newNodes[ag_pos-matchCount],st_id);
						}
					} else { //link between new and preserved nodes
						Action a{ActionType::LINK,ag_coords,st_id,true,false};
						a.chain = new Action{ActionType::LINK,rhs->getAgentCoords(trgt_pos),lnk.second,false,false};
						script.emplace_back(a);
					}
					break;
				}
				case ActionType::ERROR:
					throw SemanticError((boost::format(diff.warning)
						% name % ag_sign.getName() % ag_sign.getSite(st_id).getName()).str(),loc);
				default:break;
				}//switch
			}//for diff
		}//for site
	}//for agent

	//deleted nodes
	for(ag_pos = matchCount; ag_pos < lhs.size(); ag_pos++){
		//auto& lhs_ag = lhs.getAgent(ag_pos);
		auto ag_coords = lhs.getAgentCoords(ag_pos);
		Action a{ActionType::DELETE,ag_coords};
		script.emplace_back(a);
	}

}


const list<Action>& Rule::getScript() const{
	return script;
}
const vector<state::Node*>& Rule::getNewNodes() const{
	return newNodes;
}


void Rule::addAgentIncludes(CandidateMap &candidates,
			const Pattern::Agent& ag,small_id rhs_ag_pos,CandidateInfo info){
	for(auto& cc_agid : ag.getIncludes()){//add Agent includes to candidates
		auto& c_info = candidates[CandidateKey{cc_agid.first,{cc_agid.second,rhs_ag_pos}}];
		c_info.emb_coords = info.emb_coords;
		c_info.is_new = info.is_new;
		c_info.infl_by.insert(info.infl_by.begin(),info.infl_by.end());
	}
}


void Rule::checkInfluence(const Environment &env,const VarVector& vars) {
	//we first check all candidates to avoid do complete evaluation of same CC twice
	CandidateMap candidates;
	expressions::EvalArguments<true> args(0,&vars);
	list<Action> changes(script);
	for(auto& act : changes){//for each action (applied to 1 site)
		if(act.is_new)
			continue;
		small_id ag_pos;
		if(act.t == ActionType::DELETE || act.is_new)
			continue;//ag_pos = act.trgt_ag.first;
		else
			ag_pos = lhs.getAgentPos(act.trgt_ag);
		auto& ag = rhs->getAgent(ag_pos);
		auto& rhs_site = ag.getSite(act.trgt_st);
		for(auto ag_ptrn : env.getAgentPatterns(ag.getId())){//for each declared agent-ptrn
			auto& st_ptrn = ag_ptrn->getSiteSafe(act.trgt_st);
			CandidateInfo ci{act.trgt_ag,false};
			switch(act.t){
			case ActionType::CHANGE:case ActionType::ASSIGN:
				ci.infl_by.emplace(act.trgt_st);
				if(st_ptrn.getValueType() != Pattern::EMPTY &&
						st_ptrn.testEmbed(rhs_site,args))
					addAgentIncludes(candidates,*ag_ptrn,ag_pos,ci);
				break;
			case ActionType::UNBIND:case ActionType::LINK:
				ci.infl_by.emplace(-1-int(act.trgt_st));
				if(st_ptrn.getLinkType() != Pattern::WILD
						&& st_ptrn.testEmbed(rhs_site,args))
					addAgentIncludes(candidates,*ag_ptrn,ag_pos,ci);
				//if(act.new_label)//TODO unbinding a BND_PTRN
				break;
			default:
				throw invalid_argument("Rule::checkInfluence(): no a valid Action Type.");
			}
		}
		if(act.chain)
			changes.emplace_back(*act.chain);//TODO check if this will invalidate when last pointer
	}

	//small_id i = 0;
	//new-agents
	for(auto ag_pos = matchCount; ag_pos < rhs->size();ag_pos++){
		auto& new_ag = rhs->getAgent(ag_pos);
		//auto& ag_sign = env.getSignature(new_ag.getId());
		CandidateInfo info{{lhs.compsCount(),ag_pos-matchCount},true,{}};
		for(auto ag_ptrn : env.getAgentPatterns(new_ag.getId())){
			if(ag_ptrn->testEmbed(new_ag,args))
				addAgentIncludes(candidates,*ag_ptrn,ag_pos,info);
		}
	}
	//Checking candidates
	map<int,set<ag_st_id>> already_done;//cc-id -> every match (cc-ag -> rhs-ag)
	//cout << "testing if rule " << this->getName() << " have influence on candidates CC:" << endl;
	for(auto& key_info : candidates){
		auto& key = key_info.first;
		auto rhs_coords = rhs->getAgentCoords(key_info.first.match_root.second);
		ag_st_id root(key.match_root.first,rhs_coords.second);//agent-pos in ptrn -> agent-pos in rhc-cc
		if(already_done[key.cc->getId()].count(root))
			continue;//this match for this cc is already done TODO print this
		//cout << "\t" << cc_info.first->toString(env);
		map<small_id,small_id> emb;
		if(key.cc->testEmbed(rhs->getComponent(rhs_coords.first),root,args,emb)){
			already_done[key.cc->getId()].insert(emb.begin(),emb.end());
			influence.emplace(key_info);
		//else cout << " | NO!";
		}
		//cout << endl;
	}

	/*}//end testing try TODO
	catch(exception &ex){
		cout << "cannot check influences for " << this->getName() << endl;
		throw ex;
	}*/
}

const Rule::CandidateMap& Rule::getInfluences() const{
	return influence;
}



/**** DEBUG ****/
string Rule::toString(const pattern::Environment& env) const {
	static string acts[] = {"CHANGE","ASSIGN","BIND","FREE","DELETE","CREATE","TRANSPORT"};
	string s = name+"'s actions:";
	for(auto nn : newNodes){
		s += "\tINSERT agent "+nn->toString(env);
	}
	for(auto act : script){
		auto& agent = act.is_new?
				rhs->getAgent(act.trgt_ag) : lhs.getAgent(act.trgt_ag);
		string pos = to_string(act.is_new?
				rhs->getAgentPos(act.trgt_ag) : lhs.getAgentPos(act.trgt_ag));
		auto& ag_sign = env.getSignature(agent.getId());
		s += "\n\t";
		switch(act.t){
		case ActionType::DELETE:
			s += acts[act.t] + " agent["+pos+"]"+agent.toString(env);
			break;
		case ActionType::TRANSPORT:
			break;
		case ActionType::CHANGE:{
			auto& lbl_site = dynamic_cast<const pattern::Signature::LabelSite&>(ag_sign.getSite(act.trgt_st));
			s += acts[act.t] + " agent["+pos+"]'s site "+ag_sign.getName()+"."+lbl_site.getName();
			s += " to value " + lbl_site.getLabel(act.new_label);
			break;
		}
		case ActionType::ASSIGN:
			s += acts[act.t]+" ";
			s = s+(act.is_new? "new-":"")+"agent["+pos+"]'s site ";
			s += ag_sign.getName()+"."+ag_sign.getSite(act.trgt_st).getName()+
					" to expression "+act.new_value->toString();
			break;
		case ActionType::LINK:{
			s += acts[act.t] + " agent["+pos+"]'s sites ";
			if(act.is_new)//new node
				s += "(new) " + ag_sign.getName()+"("+ag_sign.getSite(act.trgt_st).getName()+") and ";
			else
				s += ag_sign.getName()+"("+ag_sign.getSite(act.trgt_st).getName()+") and ";
			auto& agent2 = act.chain->is_new?
					rhs->getAgent(act.chain->trgt_ag) : lhs.getAgent(act.chain->trgt_ag);
			auto& ag_sign2 = env.getSignature(agent2.getId());
			if(act.chain->is_new)//new node
				s += "(new) " + ag_sign2.getName()+"("+ag_sign2.getSite(act.chain->trgt_st).getName()+") and ";
			else
				s += ag_sign2.getName()+"("+ag_sign2.getSite(act.chain->trgt_st).getName()+")";
			break;
		}
		case ActionType::UNBIND:
			s += acts[act.t] + " agent["+pos+"]'s site "+ag_sign.getName()+"."+ag_sign.getSite(act.trgt_st).getName();
			if(act.chain){
				auto& ag_sign2 = env.getSignature(lhs.getAgent(act.chain->trgt_ag).getId());
				s += " from "+ag_sign2.getName()+"."+ag_sign2.getSite(act.chain->trgt_st).getName();
			}
			break;
		default: break;
		}
	}
	return s;
}

/*
two<FL_TYPE> Rule::evalActivity(const matching::InjRandContainer* const * injs,
		const VarVector& vars) const{
	//auto& auxs = lhs.getAux();
	FL_TYPE a = 1.0;
	for(unsigned i = 0 ; i < lhs.compsCount() ; i++){
		auto& cc = lhs.getComponent(i);
		injs[cc.getId()]->selectRule(id, i);
		a *= injs[cc.getId()]->partialReactivity();
	}
	if(basic.aux_functions.empty())
		a *= rate->getValue(vars).valueAs<FL_TYPE>();
	else
		for(auto factor : basic.factors)
			a *= factor->getValue(vars).valueAs<FL_TYPE>();
	return make_pair(a,0.0);
}
*/

//Rule::CandidateInfo::CandidateInfo() : (true),is_new(false) {};





/********** Rule::Rate ****************/


Rule::Rate::Rate(const Rule& r,state::State& state) :
		rule(r),baseRate(nullptr),unaryRate(nullptr,-1) {
	auto new_rate = rule.getRate().clone();
	baseRate = new_rate->reduce(state.vars);//rates also have to be reduced for every state
	if(new_rate != baseRate)//reduce can return the same pointer on expression but no with vars.
		delete new_rate;
}

const BaseExpression* Rule::Rate::getExpression(small_id cc_index) const {
	return baseRate;
}


NormalRate::NormalRate(const Rule& r,state::State& state) : Rule::Rate(r,state) {}

SamePtrnRate::SamePtrnRate(const Rule& r,state::State& state,bool normalize) :
		Rule::Rate(r,state),norm(1.0){
	if(normalize)
		for(auto i = r.getLHS().compsCount(); i > 1; i--)
			norm *= i;
}

AuxDepRate::AuxDepRate(const Rule& r,state::State& state) :
		Rule::Rate(r,state){
	map<string,small_id> aux_ccindex;
	for(auto& aux_cc__ : rule.getLHS().getAuxMap())
		aux_ccindex[aux_cc__.first] = aux_cc__.second.cc_pos;
	base = baseRate->reduceAndFactorize(aux_ccindex,state.vars);
}

const BaseExpression* AuxDepRate::getExpression(small_id cc_index) const {
	return base.aux_functions.at(cc_index);
}

SameAuxDepRate::SameAuxDepRate(const Rule& r,state::State& state,bool normalize) :
		Rule::Rate(r,state),SamePtrnRate(r,state,normalize),AuxDepRate(r,state) {
	for(unsigned i = 1; i < base.aux_functions.size();i++)
		if(base.aux_functions[i-1] != base.aux_functions[i]){
			delete base.factor;
			for(auto& elem : base.aux_functions)
				delete elem.second;
			throw invalid_argument("Not the same aux function.");
		}
}


Rule::Rate::~Rate() {
	if(baseRate && !dynamic_cast<const state::Variable*>(baseRate))
		delete baseRate;
	if(unaryRate.first && !dynamic_cast<const state::Variable*>(unaryRate.first))
		delete unaryRate.first;
}

NormalRate::~NormalRate(){}

SamePtrnRate::~SamePtrnRate() {}

AuxDepRate::~AuxDepRate() {
	delete base.factor;
	for(auto& elem : base.aux_functions)
		delete elem.second;

	if(unary.factor){
		delete unary.factor;
		for(auto& elem : unary.aux_functions)
			delete elem.second;
	}
}

SameAuxDepRate::~SameAuxDepRate() {}


two<FL_TYPE> NormalRate::evalActivity(const expressions::EvalArgs& args) const {
	FL_TYPE a = baseRate->getValue(args).valueAs<FL_TYPE>();
	auto& lhs = rule.getLHS();
	for(unsigned i = 0 ; i < lhs.compsCount() ; i++){
		auto& cc = lhs.getComponent(i);
		auto count = args.getState().getInjContainer(cc.getId()).count();
		if(count)
			a *= count;
		else {
			return two<FL_TYPE>(0.0,0.0);//TODO change when implement unary rate
		}
	}
	return two<FL_TYPE>(a,0.0);
}

two<FL_TYPE> SamePtrnRate::evalActivity(const expressions::EvalArgs& args) const {
	FL_TYPE a = baseRate->getValue(args).valueAs<FL_TYPE>()/norm;
	auto& lhs = rule.getLHS();
	auto count = args.getState().getInjContainer(lhs.getComponent(0).getId()).count();
	if(count)
		for(unsigned i = 0 ; i < lhs.compsCount() ; i++){
			//injs.selectRule(rule.getId(), i);
			a *= (count-i);
		}
	else
		return two<FL_TYPE>(0.0,0.0);
	return two<FL_TYPE>(a,0.0);
}

two<FL_TYPE> AuxDepRate::evalActivity(const expressions::EvalArgs& args) const {
	FL_TYPE a = base.factor->getValue(args).valueAs<FL_TYPE>();
	auto& lhs = rule.getLHS();
	for(unsigned i = 0 ; i < lhs.compsCount() ; i++){
		auto& cc = lhs.getComponent(i);
		auto& injs = args.getState().getInjContainer(cc.getId());
		injs.selectRule(rule.getId(), i);
		auto pr = injs.partialReactivity();
		if(pr)
			a *= pr;
		else
			return two<FL_TYPE>(0.0,0.0);
	}
	return two<FL_TYPE>(a,0.0);
}

two<FL_TYPE> SameAuxDepRate::evalActivity(const expressions::EvalArgs& args) const {
	FL_TYPE a = base.factor->getValue(args).valueAs<FL_TYPE>()/norm;
	auto& lhs = rule.getLHS();
	auto& injs = args.getState().getInjContainer(lhs.getComponent(0).getId());
	injs.selectRule(rule.getId(), 0);
	auto ract = injs.partialReactivity();
	if(ract)
		for(unsigned i = 0 ; i < lhs.compsCount() ; i++){
			a *= ract;
		}
	else
		return two<FL_TYPE>(0.0,0.0);
	a -= injs.getM2();
	return two<FL_TYPE>(a,0.0);
}


















} /* namespace simulation */
