/*
 * Environment.cpp
 *
 *  Created on: Mar 23, 2016
 *      Author: naxo
 */

#include "Environment.h"
#include "../grammar/ast/Basics.h"
#include "mixture/Agent.h"
#include "mixture/Component.h"
#include "../util/Warning.h"
#include "../expressions/Vars.h"
#include "../simulation/Perturbation.h"


namespace pattern {

using namespace std;
using namespace grammar::ast;

Environment::Environment() : max_radius(0){
	//the empty mixture pattern is only declared once.
	auto empty_mix = new Mixture(0);
	empty_mix->setId(0);
	mixtures.emplace_back(empty_mix);
}

Environment::~Environment() {
	for(auto pert : perts)
		delete pert;
}


bool Environment::exists(const string &name,const Environment::IdMap &map){
	return map.count(name);
}


unsigned Environment::declareToken(const Id &name_loc){
	const string& name = name_loc.getString();
	if(tokenMap.count(name) || signatureMap.count(name) )
		throw SemanticError("Name "+name+" already defined for agent or token.",name_loc.loc);
	auto id = tokenNames.size();
	tokenMap[name] = id;
	tokenNames.push_back(name);
	return id;
}

short Environment::declareParam(const AstId &name_loc,const expressions::BaseExpression* val){
	auto name  = name_loc.getString();
	short& id = varMap[name];
	if(!paramVars.count(name)){//first param declaration
		id = varNames.size();
		varNames.push_back(name);
	}
	else if(paramVars[name])//param was declared with value before
		ADD_WARN("Previous value of model parameter '"+name+"' overwritten.",name_loc.loc);
	paramVars.emplace(name_loc.getString(),val);
	return id;
}

short Environment::declareVariable(const Id &name_loc,bool is_kappa){
	//if(this->exists(name,algMap) || this->exists(name,kappaMap))
	const string& name = name_loc.getString();
	//if var declared and not a param
	if(this->exists(name,varMap)){
		/*if(!paramVars.count(name))
			if(paramVars.at(name)){
				ADD_WARN("Ignoring value for parameter defined in higher level.",name_loc.loc);
				return -1;
			}*/
		throw SemanticError("Label "+name+" already defined.",name_loc.loc);
	}
	short id;
	id = varNames.size();
	varMap[name] = id;
	varNames.push_back(name);
	return id;
}
Compartment& Environment::declareCompartment(const Id &name_loc){
	const string& name = name_loc.getString();
	if(compartmentMap.count(name))
		throw SemanticError("Compartment "+name+" already defined.",name_loc.loc);
	short id = compartments.size();
	compartments.emplace_back(name);
	compartmentMap[name] = id;
	return compartments[id];
}
UseExpression& Environment::declareUseExpression(unsigned short id,size_t n){
	if(useExpressions.size() != id)
		throw std::invalid_argument("UseExpression id doesn't match.");
	useExpressions.emplace_back(n);
	return useExpressions[id];
}
Channel& Environment::declareChannel(const Id &name_loc) {
	const string& name = name_loc.getString();
	short id;
	if(channelMap.count(name))
		id = channelMap[name];
	else{
		id = channels.size();
		channels.push_back(list<Channel>());
		channelMap[name] = id;
	}
	channels[id].emplace_back(name);
	return channels[id].back();
}
Signature& Environment::declareSignature(const Id &name_loc) {
	const string& name = name_loc.getString();
	if(tokenMap.count(name) || signatureMap.count(name) )
		throw SemanticError("Name "+name+" already defined for agent or token.",name_loc.loc);
	short id = signatures.size();
	signatures.emplace_back(name);
	signatureMap[name] = id;
	signatures[id].setId(id);
	return signatures[id];
}

void Environment::declareMixture(Mixture* &new_mix) {
	if(new_mix->size() == 0){
		delete new_mix;
		new_mix = mixtures.front();
		return;
	}
	new_mix->setId(mixtures.size());
	new_mix->declareComponents(*this);
	mixtures.emplace_back(new_mix);
}

map<small_id,small_id> Environment::declareComponent(Mixture::Component* &new_comp){
	auto order_mask = new_comp->declareAgents(*this);
	for(auto& comp : components ){
		if( *comp == *new_comp){
			delete new_comp;
			new_comp = comp;
			return order_mask;
		}
	}
	components.emplace_back(new_comp);
	new_comp->setId(components.size()-1);
	small_id i = 0;
	for(auto ag : *new_comp)
		ag->addCc(new_comp,i++);
	return order_mask;
}

void Environment::declareAgentPattern(Pattern::Agent* &new_ag){
	for(auto &ag : agentPatterns[new_ag->getId()] ){
		if( *new_ag == *ag ){
			//cout << "Same agent pattern was declared before: " << ag.toString(*this) << endl;
			delete new_ag;
			new_ag = ag;
			return;
		}
	}
	agentPatterns[new_ag->getId()].emplace_back(new_ag);
}

simulation::Rule& Environment::declareRule(const Id &name_loc,Mixture& mix,const yy::location& loc){
	//if(this->exists(name,algMap) || this->exists(name,kappaMap))
	const string& name = name_loc.getString();
	if(this->exists(name,varMap))
		throw SemanticError("Label "+name+" already defined.",name_loc.loc);
	small_id id;
	id = varNames.size();
	varMap[name] = id;
	varNames.push_back(name);
	rules.emplace_back(rules.size(),name_loc.getString(),mix,loc);
	return rules.back();
}

void Environment::declareUnaryMix(Mixture& mix,int radius){
	//TODO check if new unary mix is in reverse order
	if(mix.compsCount() != 2)
		throw invalid_argument("Environment::declareUnaryRule().");
	mix.setRadius(radius);
	auto ids = mix.getUnaryKey();
	if(ids.first > ids.second)
		throw SemanticError("The C.Components of unary mixture have to be put in reverse order (see TODO).",yy::location());
	unaryMix[ids].emplace_back(radius,int(mix.getId()));
	unaryMix[ids].sort();
	unaryCC[ids.first].emplace_back(radius,ids.second);
	unaryCC[ids.first].sort();
	if(ids.first != ids.second){
		unaryMix[make_pair(ids.second,ids.first)].emplace_back(radius,int(mix.getId()));
		unaryMix[make_pair(ids.second,ids.first)].sort();
		unaryCC[ids.second].emplace_back(radius,ids.first);
		unaryCC[ids.second].sort();
	}
	if(max_radius < radius)
		max_radius = radius;
}

void Environment::declarePert(simulation::Perturbation* pert){
	pert->setId(perts.size());
	perts.push_back(pert);
}

void Environment::declareMixInit(int use_id,expressions::BaseExpression* n,Mixture* mix){
	inits.emplace_back(use_id,n,mix,-1);
}
void Environment::declareTokInit(int use_id,expressions::BaseExpression* n,int tok_id){
	inits.emplace_back(use_id,n,nullptr,tok_id);
}

/*
void Environment::declareAux(const Id& name,Mixture& mix,small_id site_id){
	auto& aux_name = name.getString();
	if(auxTemp.count(aux_name)){
		auto& mix_coords = auxTemp.at(aux_name);
		if(get<0>(mix_coords) == &mix)
			throw SemanticError("Auxiliary "+aux_name+" is defined twice for the same mixture.",name.loc);
		else if(get<0>(mix_coords))
			throw invalid_argument("Environment::declareAux(): Aux temporary used by other mixture (maybe other thread?).");
		else{		//

		}

			mix_coords = make_tuple(mix,mix.size(),site_id);
	}
	typedef expressions::Auxiliar<FL_TYPE>::CoordInfo Coord;
	for(auto& aux : auxExpressions[&mix]){
		auto& coords = aux.getCoords();
		if(mix.size() == get<Coord::AG_ID>(coords)){//Aux expression declared in the same agent
			if(site_id >= get<Coord::ST_ID>(coords))
				throw SemanticError("Pattern expression depends on signature-declared-later" +
						" site value in the same agent. Try rearrange sites in signature.",name.loc);
		}
		else {
			//expr in other agent, give a hint to mixture to declare this agent before expr agent
		}
	}
		auxTemp.emplace(aux_name,make_tuple(&mix,mix.size(),site_id));//agent id should be equal to mix.size()
}

expressions::BaseExpression* Environment::declareAuxExpr(const Id &name,const Mixture& mix,small_id st_id){
	auto& aux_name = name.getString();
	auto& aux_list = auxExpressions[&mix];
	auto aux = new expressions::Auxiliar<FL_TYPE>(aux_name);
	aux_list.emplace_back(aux_name);
	return &aux_list.back();
}
*/


void Environment::declareObservable(state::Variable* var){
	observables.emplace_back(var);
}

void Environment::buildInfluenceMap(const simulation::SimContext &context){
	for(auto& r : rules)
		r.checkInfluence(*this,context);
}
void Environment::buildFreeSiteCC() {
	for(auto ag_class : agentPatterns){
		for(auto& ag : ag_class){
			for(auto& site : *ag){
				if(site.getLinkType() == Pattern::FREE){
					auto& l = freeSiteCC[(ag->getId() << sizeof(small_id)) + site.getId()];
					l.insert(l.end(),ag->getIncludes().begin(),ag->getIncludes().end());
				}
			}
		}
	}
}

const list<pair<const Mixture::Component*,small_id> >& Environment::getFreeSiteCC(small_id ag,small_id site) const{
	static list<pair<const Mixture::Component*,small_id> > l;
	try{
		return freeSiteCC.at((ag << sizeof(small_id)) + site);
	}
	catch(std::out_of_range& ex){}
	return l;
}



/*template <typename T>
const T& Environment::getSafe(grammar::ast::Id& name) const {
	if()
}*/


short Environment::getVarId(const string &s) const {
	return varMap.at(s);
}
short Environment::getVarId(const Id &s) const {
	try {
		return varMap.at(s.getString());
	}catch(out_of_range &ex){
		throw SemanticError("Variable "+s.getString()+" has not been declared (yet).",s.loc);
	}
	return -1;
}
short Environment::getChannelId(const string &s) const {
	return channelMap.at(s);
}
short Environment::getCompartmentId(const string &s) const {
	return compartmentMap.at(s);
}
short Environment::getSignatureId(const string &s) const {
	return signatureMap.at(s);
}
unsigned Environment::getTokenId(const string &s) const {
	return tokenMap.at(s);
}


const Compartment& Environment::getCompartment(short id) const{
	return compartments[id];
}
const list<Channel>& Environment::getChannels(short id) const{
	return channels[id];
}
const UseExpression& Environment::getUseExpression(short id) const {
	return useExpressions[id];
}
const Signature& Environment::getSignature(short id) const {
	return signatures[id];
}
const vector<pattern::Signature>& Environment::getSignatures() const{
	return signatures;
}
const vector<Mixture*>& Environment::getMixtures() const{
	return mixtures;
}
const vector<Mixture::Component*>& Environment::getComponents() const{
	return components;
}
const list<Mixture::Agent*>& Environment::getAgentPatterns(small_id id) const{
	return agentPatterns[id];
}
const vector<simulation::Rule>& Environment::getRules() const {
	return rules;
}
const vector<simulation::Perturbation*>& Environment::getPerts() const {
	return perts;
}
const list<Environment::Init>& Environment::getInits() const{
	return inits;
}
const list<state::Variable*>& Environment::getObservables() const {
	return observables;
}
const map<string,const expressions::BaseExpression*>& Environment::getParams() const {
	return paramVars;
}


const Compartment& Environment::getCompartmentByCellId(unsigned cell_id) const{
	for(auto &comp : compartments){
		if(cell_id < (unsigned)comp.getLastCellId())
			return comp;
	}
	throw std::out_of_range("Compartment of cell-id "+to_string(cell_id)+" not found.");
}


template <>
size_t Environment::size<Mixture>() const {
	return mixtures.size();
}
template <>
size_t Environment::size<Mixture::Component>() const {
	return components.size();
}
template <>
size_t Environment::size<Channel>() const {
	return channels.size();
}
template <>
size_t Environment::size<state::TokenVar>() const {
	return tokenNames.size();
}
template <>
size_t Environment::size<simulation::Rule>() const {
	return rules.size();
}


template <>
void Environment::reserve<Compartment>(size_t count) {
	compartments.reserve(count);
}
template <>
void Environment::reserve<Signature>(size_t count) {
	signatures.reserve(count);
}
template <>
void Environment::reserve<Mixture::Agent>(size_t count) {
	agentPatterns.resize(count);
}
template <>
void Environment::reserve<Channel>(size_t count) {
	channels.reserve(count);
}
template <>
void Environment::reserve<state::TokenVar>(size_t count) {
	tokenNames.reserve(count);
}
template <>
void Environment::reserve<UseExpression>(size_t count) {
	useExpressions.reserve(count);
}
template <>
void Environment::reserve<simulation::Rule>(size_t count) {
	rules.reserve(count);
}
template <>
void Environment::reserve<simulation::Perturbation>(size_t count) {
	perts.reserve(count);
}

const DepSet& Environment::getDependencies(const Dependency& dep) const{
	return deps.getDependencies(dep).deps;
}
const Dependencies& Environment::getDependencies() const{
	return deps;
}
void Environment::addDependency(Dependency key,Dependency::Dep dep,unsigned id){
	if(key.type == Dependency::TIME && dep == Dependency::PERT &&
			perts[id]->nextStopTime() >= 0)
		deps.addTimePertDependency(id, perts[id]->nextStopTime());
	else
		deps.addDependency(key,dep,id);
}


//DEBUG methods

std::string Environment::cellIdToString(unsigned int cell_id) const {
	return this->getCompartmentByCellId(cell_id).cellIdToString(cell_id);

}

void Environment::show(const simulation::SimContext& context) const {
	//try{
		cout << "========= The Environment =========" << endl;
		/*cout << "\t\tCompartments[" << compartments.size() << "]" << endl;
		for(unsigned int i = 0; i < compartments.size() ; i++)
			cout << (i+1) << ") " << compartments[i].toString() << endl;

		cout << "\n\t\tUseExpressions[" << useExpressions.size() << "]" << endl;
		for(auto& use_expr : useExpressions){
			cout << "list of cells: " ;
			for(auto cell_id : use_expr.getCells())
				cout << cellIdToString(cell_id) << ", ";
			cout << endl;
		}

		cout << "\n\t\tChannels[" << channels.size() << "]" << endl;
		for(unsigned int i = 0; i < channels.size() ; i++){
			cout << (i+1) << ") ";
			for(list<Channel>::const_iterator it = channels[i].cbegin();it != channels[i].cend();it++){
				cout << it->toString() << endl;
				list< list<int> > l = it->getConnections(context);
				it->printConnections(l);
			}
		}
		*/
		/*cout << "\n\t\tAgentPatterns[" ;
		int count = 0;
		for(auto& ag_list : agentPatterns)
			count += ag_list.size();
		cout << count << "]" << endl;
		for(auto& ap_list : agentPatterns)
			for(auto& ap : ap_list){
				for(auto& site_ptrns : ap.getParentPatterns()){
					cout << this->getSignature(ap.getId()).getSite(site_ptrns.first).getName()
							<< "~{";
					for(auto& emb : site_ptrns.second)
						cout << emb->toString(*this) << ",";
					cout << "}, ";
				}
				cout << " >>> [ " << ap.toString(*this) << " ] >>> ";
				for(auto& site_ptrns : ap.getChildPatterns()){
					cout << this->getSignature(ap.getId()).getSite(site_ptrns.first).getName()
							<< "~{";
					for(auto& emb : site_ptrns.second)
						cout << emb->toString(*this) << ",";
					cout << "}, ";
				}
				cout << endl;
			}*/
		cout << "Components[" << components.size() << "]" << endl;
		int i = 0;
		for(auto comp : components){
			cout << "  [" << comp->getId() << "] -> ";
			cout << comp->toString(*this) << endl;
			//i++;
		}
		/*cout << "Mixtures[" << mixtures.size() << "]" << endl;
		for(auto mix : mixtures){
			cout << "  [" << mix->getId() << "] -> ";
			cout << mix->toString(*this) << endl;
			//i++;
		}*/
		cout << "Rules[" << rules.size() << "]" << endl;
		i = 0;
		for(auto& rul : rules){
			cout << "  [" << rul.getId() << "] ";
			cout << rul.toString(*this);
			int rhs_ag_pos(-1);
			int j = 0;
			for(auto& key_info : rul.getInfluences()){
				if(rhs_ag_pos != int(key_info.first.match_root.second)){
					rhs_ag_pos = key_info.first.match_root.second;
					cout << "\n      Changes in RHS[" << rhs_ag_pos << "]"
						" could produce new Injections of:";
				}
				cout <<"\n\t(" << j++ << ")  " <<
					key_info.first.cc->toString(*this,key_info.first.match_root.first);
			}
			cout << "\n" << endl;
			i++;
		}

	/*}
	catch(exception &e){
		cout << "error: " << e.what() << endl;
	}*/
	cout << "-----------------------------" << endl;

}








} /* namespace pattern */
