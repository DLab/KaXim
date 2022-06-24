/*
 * Mixture.cpp
 *
 *  Created on: Apr 26, 2016
 *      Author: naxo
 */

#include <cstring>
#include "Mixture.h"
#include "../Environment.h"
//#include "../util/Warning.h"
#include "Agent.h"
#include "Component.h"


namespace pattern {


/***************** class Mixture ************************/

Mixture::Mixture(short agent_count,yy::location _loc) : loc(_loc),radius(0),id(-1){
	agents.reserve(agent_count);
}

/*Mixture::Mixture(const Mixture& m) : id(-1){
	if(m.comps){
		agents = nullptr;
		comps = new vector<Component*>(m.compCount);
		for(size_t i = 0; i < compCount; i++)
			(*comps)[i] = (*m.comps)[i];
	}
	else {
		comps = nullptr;
		agents = new Agent*[m.compCount];
		for(size_t i = 0; i < agentCount; i++)
			agents[i] = m.agents[i];
	}
}*/

//TODO
Mixture::~Mixture() {
	if(id == short_id(-1)){
		for(auto comp : comps)
			delete comp;
		for(auto ag : agents)
			delete ag;
	}
}

void Mixture::setId(short_id i){
	id = i;
}
/*const short_id& Mixture::getId() const {
	return id;
}*/
void Mixture::addAgent(Mixture::Agent *a){
	if(isSet())
		throw std::invalid_argument("Mixture::addAgent(): "
				"Cannot call addAgent() on an initialized Mixture");
	agents.emplace_back(a);
}


void Mixture::addBindLabel(int lbl, small_id ag_pos,small_id site_id){
	if(isSet())
		throw std::invalid_argument("Mixture::addBindLabel(): forbidden call.");
	auto& sites = labelBinds[lbl];
	sites.emplace_back(ag_pos,site_id);
	if(sites.size() == 2){
		auto& p1 = sites.front();
		auto& p2 = sites.back();
		if(p1.first < p2.first)
			binds.emplace(p1,p2);
		else
			binds.emplace(p2,p1);
	}
	else if(sites.size() > 2)
		throw SemanticError("Edge identifier "+to_string(lbl)+
				" used 3+ times in this mixture.",loc);
}

void Mixture::declareComponents(Environment &env){
	int cc_pos = 0;
	//cout << "before reorder: " << toString(env) << endl;
	for(auto& comp : comps){
		auto order_mask = env.declareComponent(comp);
		for(auto swap : order_mask){//updating possible agent pointer change and positions
			ag_st_id old_ag_coords(cc_pos,swap.first);
			ag_st_id new_ag_coords(cc_pos,swap.second);
			auto ag_pos = mixAgPos.at(old_ag_coords);
			agents.at(ag_pos) = comp->agents.at(swap.second);
			ccAgPos.at(ag_pos) = new_ag_coords;
		}
		for(auto pos_coords : ccAgPos)
			mixAgPos[pos_coords.second] = pos_coords.first;
		cc_pos++;
	}
	for(auto& aux_coord : auxCoords){
		auto& coord = aux_coord.second;
		auto ag_pos = coord.ag_pos;
		coord.cc_pos = ccAgPos.at(ag_pos).first;
		coord.ag_pos = ccAgPos.at(ag_pos).second;
		if(coord.is_ptrn)
			env.addAuxSiteInfl(getAgent(ag_pos).getId(),coord.st_id,coord.ag_pos,*comps[coord.cc_pos]);
	}
	for(auto aux : auxRefs)
		aux->coords = auxCoords.at(aux->toString());
	//cout << "after reorder: " << toString(env) << endl;
}

void Mixture::setComponents(){
	if(isSet())
		throw std::invalid_argument("Cannot call setComponents() on an initialized Mixture");
	for(auto& binds : labelBinds)//checking if error in label binds
		if(binds.second.size() != 2)//1->semantic error. other->invalid call
			throw SemanticError("Bind label "+to_string(binds.first)+" is not paired in mixture.",loc);

	//list<pair<Component*,map<short,short> > > comps;
	//iterate link map ordered by agent_id, one copy per non directed link
	for(auto l_it = binds.cbegin();l_it != binds.cend(); l_it++){
		auto p1 = l_it->first,p2 = l_it->second;
		auto c_it = comps.begin();

		agents[p1.first]->getSite(p1.second).lnk_ptrn =
				ag_st_id(agents[p2.first]->getId(),p2.second);
		agents[p2.first]->getSite(p2.second).lnk_ptrn =
				ag_st_id(agents[p1.first]->getId(),p1.second);
		//iterate on local components, test if ag(p1) or ag(p2) \in comp
		unsigned cc_pos = 0;
		for(;c_it != comps.end(); c_it++,cc_pos++){
			Component &comp = **c_it;
			//if one of the edge nodes are in a CC
			if(ccAgPos.find(p1.first) != ccAgPos.end()){
				if(ccAgPos.find(p2.first) == ccAgPos.end()){//and the other node is not
					comp.addAgent(agents[p2.first]);
					ccAgPos.emplace(p2.first,make_pair(cc_pos,comp.size()-1));
				}
				comp.addBind(ag_st_id(ccAgPos[p1.first].second,p1.second),
						ag_st_id(ccAgPos[p2.first].second,p2.second));
				break;
			}
			else if(ccAgPos.find(p2.first) != ccAgPos.end()){//the other node
				comp.addAgent(agents[p1.first]);
				ccAgPos.emplace(p1.first,make_pair(cc_pos,comp.size()-1));
				comp.addBind(ag_st_id(ccAgPos[p1.first].second,p1.second),
						ag_st_id(ccAgPos[p2.first].second,p2.second));
				break;
			}
		}
		//add a new component if edge agents are not in any defined component
		if(c_it == comps.end()){
			comps.emplace_back(new Component());
			comps.back()->addAgent(agents[p1.first]);
			comps.back()->addAgent(agents[p2.first]);
			comps.back()->addBind(ag_st_id(0,p1.second),ag_st_id(1,p2.second));
			ccAgPos.emplace(p1.first,two<small_id>(cc_pos,0));
			ccAgPos.emplace(p2.first,two<small_id>(cc_pos,1));
		}

	}
	//add not connected agents as not connected components
	for(size_t i = 0; i < agents.size() ; i++){
		if(ccAgPos.find(i) == ccAgPos.end()){
			comps.emplace_back(new Component());
			comps.back()->addAgent(agents[i]);
			ccAgPos.emplace(i,make_pair(comps.size()-1,0));
		}
	}
	for(auto& pos : ccAgPos)
		mixAgPos[pos.second] = pos.first;

	//TODO if non declared mixture can use aux. vars, set auxCoords HERE.
	labelBinds.clear();
}


bool Mixture::isSet() const {
	return comps.size() ? true : (id != short_id(-1));
}

/*bool Mixture::operator==(const Mixture& m) const{
	if(this == &m)
		return true;
	if(agentCount != m.agentCount || compCount != m.compCount ||
			siteCount!= m.siteCount )
		return false;
	if(comps){
		for(size_t i = 0; i < compCount; i++)
			if(!((*comps)[i] == (*m.comps)[i]))
				return false;
	} else
		for(size_t i = 0; i < compCount; i++)
			if(!(agents[i] == m.agents[i]))
				return false;
	return true;
}*/

Mixture::Agent& Mixture::getAgent(small_id ag_pos){
	return *agents.at(ag_pos);
}

const Mixture::Agent& Mixture::getAgent(small_id ag_pos) const {
	return *agents.at(ag_pos);
}
Mixture::Component& Mixture::getComponent(small_id cc_pos) {
	return *comps.at(cc_pos);
}

bool Mixture::addAuxCoords(string aux_name,small_id agent_pos,small_id site_id) {
	auto aux_it = auxCoords.find(aux_name);
	if(aux_it != auxCoords.end()){	//aux_name was declared before
		auto& coord = aux_it->second;
		if(coord.cc_pos != small_id(-1)) { //aux_name was used in other site pattern-value
			coord.cc_pos = 0;coord.ag_pos = agent_pos;
			coord.st_id = site_id;
			return true;
		}
		return false;//aux_name used before in same mixture
	}
	return auxCoords.emplace(aux_name,AuxCoord{0,agent_pos,site_id,false}).second;
}

void Mixture::addAuxRef(expressions::Auxiliar<FL_TYPE>* expre) const {
	if(isSet())
		expre->coords = auxCoords.at(expre->name);
	else
		auxRefs.push_back(expre);
}

const map<string,Mixture::AuxCoord>& Mixture::getAuxMap() const {
	return auxCoords;
}

void Mixture::addInclude(small_id r_id) {
	if(!isSet())
		throw std::invalid_argument("Cannot call addInclude() on a uninitialized Mixture");
	for(auto cc : comps)
		cc->addInclude(r_id);
	Pattern::addInclude(r_id);
}


const vector<Pattern::Component*>::const_iterator Mixture::begin() const {
	return comps.begin();
}
const vector<Pattern::Component*>::const_iterator Mixture::end() const {
	return comps.end();
}

string Mixture::toString(const Environment& env,int mark) const {
	string out;
	short i = 0;
	if(comps.size())
		for( auto c : comps ) {
			out += "{" + c->toString(env,mark) + "},";
			i++;
		}
	else
		out = "{}";

	return out;
}

string Mixture::toString(const Environment& env,const vector<ag_st_id>& mask) const {
	string out("");
	for(size_t i = 0;i < mask.size(); i++)
		out += getAgent(mask[i]).toString(env)+",";
	out.pop_back();
	return out;
}






/*****************************************/
/*
template <typename T1,typename T2>
bool ComparePair<T1,T2>::operator()(pair<T1,T2> p1,pair<T1,T2> p2){
	return p1.first < p2.first ? true : (p1.second < p2.second ? true : false );
}


ostream& operator<<(ostream& os,const ag_st_id &ag_st){
	return os << "(" << ag_st.first << "," << ag_st.second << ")";
}
*/
} /* namespace pattern */
