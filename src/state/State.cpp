/*
 * State.cpp
 *
 *  Created on: Mar 23, 2016
 *      Author: naxo
 */

#include "State.h"
#include "../pattern/Environment.h"
#include "../matching/Injection.h"
#include "../matching/InjRandSet.h"
#include "../data_structs/MyMaskedBinaryRandomTree.h"
#include "../data_structs/SimpleTree.h"
#include "../simulation/Plot.h"

#include "../pattern/mixture/Component.h"
#include "../pattern/mixture/Agent.h"
#include "../simulation/Simulation.h"
#include "../data_structs/DataTable.h"
#include <cmath>

namespace state {
using Deps = pattern::Dependency::Dep;
using namespace matching;

State::State(int id,simulation::Simulation& _sim,
	const BaseExpression& vol,bool by_events) :
		simulation::SimContext(id,&_sim),
		volume(vol),rates(env.size<simulation::Rule>()),
		tokens (new FL_TYPE[env.size<state::TokenVar>()]()),activityTree(nullptr),
		injections(nullptr),ev(_sim.ev),activeDeps(env.getDependencies()),
		pertIds(),timePerts(){
	if(by_events)
		plot = new simulation::EventPlot(*this);
	else
		plot = new simulation::TimePlot(*this);
	for(unsigned i = 0; i < vars.size(); i++){
		vars[i] = dynamic_cast<Variable*>(vars[i]->clone());
		vars[i]->reduce(*this);//vars are simplified for every state
	}
	for(auto rule_ptr : env.getRules()){
		auto& rule = *rule_ptr;
		int i = rule.getId();
		auto& lhs = rule.getLHS();
		auto lhs_comps = lhs.compsCount();
		bool same_ptrn = false;
		for(int cc_id = 0; cc_id < int(lhs_comps)-1; cc_id++)
			if(&(lhs.getComponent(cc_id)) != &(lhs.getComponent(cc_id+1)) )
				{same_ptrn = false; break;}
			else
				same_ptrn = true;

		if(rule.getTargetCell(id) == -1){
			rates[i] = new simulation::Rule::Rate(rule);
			continue;
		}
		if(rule.getRate().getVarDeps() & BaseExpression::AUX){
			if(same_ptrn){
				try{
					rates[i] = new simulation::SameAuxDepRate(rule,*this,false);
				}
				catch(invalid_argument& e){
					ADD_WARN_NOLOC("Same Pattern but not same pattern for rates?");
					rates[i] = new simulation::AuxDepRate(rule,*this);
				}
			}
			else
				rates[i] = new simulation::AuxDepRate(rule,*this);
		}
		else
			if(same_ptrn && lhs.size() > 1)
				rates[i] = new simulation::SamePtrnRate(rule,*this,false);
			else
				rates[i] = new simulation::NormalRate(rule,*this);
	}
	for(auto pert : env.getPerts()){
		perts.emplace(pert->getId(),*pert);
	}
}

//const State empty = State(0,std::vector<Variable*>(),new Constant<float>(1.0),
//		simulation::Plot(),pattern::Environment::empty)


State::~State() {
	delete[] tokens;
	if(activityTree)
		delete activityTree;
	if(injections){
		for(unsigned i = 0; i < env.size<pattern::Mixture::Component>(); i++)
			delete injections[i];
		delete[] injections;
	}
	//test
	activityTree = nullptr;
	injections = nullptr;
	for(auto rate : rates)
		delete rate;
	for(auto it = vars.rbegin(); it != vars.rend(); it++)
		delete *it;//vars are deleted in main!
}


void State::initNodes(unsigned n,const pattern::Mixture& mix){
	for(auto comp_p : mix){
		graph.addComponents(n,*comp_p,*this);
	}
}

void State::addNodes(unsigned n,const pattern::Mixture& mix){
	for(auto comp_p : mix)//give ev.emb to save new nodes for positive update
		graph.addComponents(n,*comp_p,*this,ev.emb[0]);
}

UINT_TYPE State::mixInstances(const pattern::Mixture& mix) const{
	//auto& mix = env.getMixtures().front();
	UINT_TYPE  countr = 1;
	for(const auto cc : mix){
		countr *= count(cc->getId());
	}
	return countr;
}


void State::apply(const simulation::Rule& r){
	//ev.aux_map.setEmb(ev.emb);

	int i = 0;
	for(auto n : r.getNewNodes()){//initializing new nodes created by applied rule
		auto node = new Node(*n,ev.new_cc);
		ev.new_cc[n] = node;
		graph.allocate(node);
		ev.emb[ev.cc_count][i] = node;
		i++;
		IF_DEBUG_LVL(3,log_msg += "\t[create] " + n->toString(env) + "\n");
	}
	ev.new_cc.clear();
	for(auto& act : r.getScript()){
		ev.act = act;
		auto node = ev.emb[act.trgt_ag.first][act.trgt_ag.second];
		switch(act.t){//TODO remove switch
		case pattern::ActionType::CHANGE:
			node->changeIntState(*this);
			break;
		case pattern::ActionType::ASSIGN:
			node->assign(*this);
			break;
		case pattern::ActionType::LINK:
			node->bind(*this);
			break;
		case pattern::ActionType::UNBIND:
			node->unbind(*this);
			break;
		case pattern::ActionType::DELETE:
			node->removeFrom(*this);
			break;
		case pattern::ActionType::TRANSPORT:{
			map<int,list<Node*>> nbh;
			nbh[0].push_back(node);
			set<Node*> cc;
			graph.neighborhood(nbh,cc);//move-join
			InjSet injs;
			graph.moveOut(cc,injs);
			ev.act.trgt_st = r.getTargetCell(id);
			auto& dest = static_cast<simulation::Simulation*>(parent)->getCell(ev.act.trgt_st);
			for(auto inj : injs){
				auto cc_inj = static_cast<matching::CcInjection*>(inj);
				injections[inj->pattern().getId()]->erase(inj,*this);//TODO this will stop working when injs are reused again
				dest.injections[inj->pattern().getId()]->insert(cc_inj,dest);
				ev.to_update.emplace(&inj->pattern());
			}
			dest.graph.moveIn(cc);
			IF_DEBUG_LVL(3,log_msg += "\t[move-join] " + node->toString(env) + " -> "+ env.cellIdToString(ev.act.trgt_st) +"\n");
			break;
		}
		}
		//ev.emb[act.trgt_ag.first][act.trgt_ag.second]->applyAction(*this);
	}
	for(auto inj : ev.inj_mask)//only injs that will not be copied
		ev.to_update.emplace(&inj.first->pattern());
	//TODO create left nodes from ev.new_cc
	for(auto& node_pair : ev.new_cc){
		if(!node_pair.second){
			node_pair.second = new Node(*node_pair.first,ev.new_cc);
		}
	}
	//copy deps and alloc
	for(auto& node_pair : ev.new_cc){
		if(node_pair.first != node_pair.second){
			node_pair.second->copyDeps(*node_pair.first,ev,injections,*this);
			graph.allocate(node_pair.second);
		}
	}
	//apply token changes
	for(auto& change : r.getTokenChanges()){
		tokens[change.first] += change.second->getValue(*this).valueAs<FL_TYPE>();
		updateDeps(pattern::Dependency(Deps::TOK,change.first));
	}
}

void State::positiveUpdate(const Rule::CandidateMap& wake_up){
	//TODO vars_to_wake_up
	//auto& wake_up = rule.getInfluences();
	for(auto& key_info : wake_up){
		auto key = key_info.first;
		auto info = key_info.second;
		Node* node = ev.emb[info.emb_coords.first][info.emb_coords.second];
		auto nulls = ev.null_actions.equal_range(node);//TODO can be optimized to only call once (candidate key doble mapping)
		//if(info.infl_by.size() && distance(nulls.first,nulls.second) == info.infl_by.size())
		//	continue;	//every action applied to node is null
		map<int,matching::InjSet*> port_list;
		auto inj_p = injections[key.cc->getId()]->
				emplace(*this,node,port_list,key.match_root.first);

		if(inj_p){
			IF_DEBUG_LVL(4,
				if(ev.new_injs.size() == 0)
					log_msg += "New injections found:\n";
				log_msg += "\tPattern " + inj_p->pattern().toString(env) + " matches with agents IDs ";
				for(auto& node : *inj_p->getEmbedding())
					log_msg += to_string(node->getAddress()) + ",";
				log_msg += "\n";
			)
			ev.new_injs.push_back(inj_p);
			for(;nulls.first != nulls.second;++nulls.first)
				port_list.erase(nulls.first->second);
			//TODO? check cc.size()
			node->addDep(inj_p);
			for(auto port : port_list)
				port.second->emplace(inj_p);
			//cout << "matching Node " << node_p->toString(env) << " with CC " << comp.toString(env) << endl;
			ev.to_update.emplace(key.cc);
			updateDeps(pattern::Dependency(Deps::KAPPA,key.cc->getId()));
		}
	}
	for(auto side_eff : ev.side_effects){//trying to create injection for side-effects
		for(auto& cc_ag : env.getFreeSiteCC(side_eff.first->getId(),side_eff.second)){
			map<int,matching::InjSet*> port_list;
			auto inj_p = injections[cc_ag.first->getId()]->emplace(*this,side_eff.first,port_list,cc_ag.second);
			if(inj_p){
				side_eff.first->addDep(inj_p);
				for(auto port : port_list)
					port.second->emplace(inj_p);
				ev.to_update.emplace(cc_ag.first);
			}
		}
	}
}

void State::nonLocalUpdate(const simulation::Rule& rule,
		const list<matching::Injection*>& new_injs) {
	auto& unary_cc = env.getUnaryCCs();
	auto& unary_mixts = env.getUnaryMixtures();
	auto script_it = rule.getScript().rbegin();
	int max_dist = env.getMaxRadius() - 1;
	map<int,list<Node*>> nbh1,nbh2;
	set<Node*> nbh_set1,nbh_set2;
	map<Injection*,int> injs1,injs2,*injs_ptr(nullptr);
	function<bool(Node*,int)> filter = [&] (Node* node,int d){
		for(auto inj : node->getLifts())
			if(unary_cc.find(inj->pattern().getId()) != unary_cc.end())
				injs_ptr->emplace(inj,d);
		return false;
	};
	//possible new unary injections caused by binds
	for(int i = 0; i < rule.getBindCount(); i++){
		auto cc_ag = script_it->trgt_ag;
		auto node1 = ev.emb[cc_ag.first][cc_ag.second];
		cc_ag = script_it->chain->trgt_ag;
		auto node2 = ev.emb[cc_ag.first][cc_ag.second];
		nbh1[0].emplace_back(node1);
		nbh_set1.emplace(node2);
		injs_ptr = &injs1;
		graph.neighborhood(nbh1,nbh_set1,max_dist,&filter);
		nbh2[0].emplace_back(node2);
		nbh_set2.emplace(node1);
		injs_ptr = &injs2;
		graph.neighborhood(nbh2,nbh_set2,max_dist,&filter);//TODO this filter can be more specific

		script_it++;
		for(auto inj1_r : injs1){
			auto inj1 = inj1_r.first;
			for(auto inj2_r : injs2){//TODO iterate in the other way?
				auto inj2 = inj2_r.first;
				if(inj1 == inj2)
					continue;
				pair<int,int> key(inj1->pattern().getId(),inj2->pattern().getId());
				auto umix_it = unary_mixts.find(key);
				if(umix_it != unary_mixts.end()){
					auto& umix = umix_it->second;
					int dist = inj1_r.second + inj2_r.second+1;
					IF_DEBUG_LVL(0,if(dist < 0) throw invalid_argument("State::nonLocalUpdate(): r < 0!");)
					/*for(auto r_mix : umix){
						if(r_mix.first < dist)
							break;*/
					if(umix.back().first < dist)
						continue;
					auto mix_id = umix.front().second;
					auto mix = env.getMixtures()[mix_id];
					IF_DEBUG_LVL(2,log_msg += "New unary injection of "
							+ mix->toString(env) + " found: "
							+ to_string(inj1->getEmbedding()[0][0]->getAddress()) + ","
							+ to_string(inj2->getEmbedding()[0][0]->getAddress()) + "\n"
					);
					if(inj1->pattern().getId() < inj2->pattern().getId())
						nlInjections.at(mix_id)->emplace(*this,inj1,inj2,dist);
					else
						nlInjections.at(mix_id)->emplace(*this,inj2,inj1,dist);
					ev.to_update.emplace(mix);
				}
			}
		}
	}
	//TODO new unary inj from new injs

	nbh1.clear();
	nbh2.clear();
	nbh_set1.clear();
	nbh_set2.clear();
	map<Injection*,two<int>> injs_map2;
	int cc_id;
	function<bool(Node*,int)> filter2 = [&] (Node* node,int d){
		for(auto inj : node->getLifts()){
			auto unmix_it = unary_mixts.find(make_pair(cc_id,inj->pattern().getId()));
			if(unmix_it != unary_mixts.end() && unmix_it->second.back().first >= d)
				injs_map2.emplace(inj,make_pair(d,unmix_it->second.front().second));
		}
		return false;
	};
	for(auto new_inj : new_injs){
		cc_id = new_inj->pattern().getId();
		auto uncc_it = unary_cc.find(cc_id);
		if(uncc_it != unary_cc.end()){
			auto& emb = *new_inj->getEmbedding();
			auto& target_ccs = uncc_it->second;
			nbh1.emplace(piecewise_construct,forward_as_tuple(0),
					forward_as_tuple(emb.begin(),emb.end()));
			graph.neighborhood(nbh1,nbh_set1,target_ccs.back().first,&filter2);
			for(auto inj_dist_rid : injs_map2){
				auto inj2 = inj_dist_rid.first;
				if(inj2 == new_inj)
					continue;
				auto mix = env.getMixtures()[inj_dist_rid.second.second];
				IF_DEBUG_LVL(2,log_msg += "New unary injection of "
						+ mix->toString(env) + " found: "
						+ to_string(new_inj->getEmbedding()[0][0]->getAddress()) + ","
						+ to_string(inj2->getEmbedding()[0][0]->getAddress()) + "\n";
				)
				if(new_inj->pattern().getId() < inj2->pattern().getId())
					nlInjections.at(inj_dist_rid.second.second)->emplace(*this,new_inj,inj2,inj_dist_rid.second.first);
				else
					nlInjections.at(inj_dist_rid.second.second)->emplace(*this,inj2,new_inj,inj_dist_rid.second.first);
				ev.to_update.emplace(mix);

			}
		}
	}
}

void State::activityUpdate(){
	//prepare positive_update
	//set<small_id> rule_ids;
	for(auto ptrn : ev.to_update){
		for(auto r_id : ptrn->includedIn())
			ev.rule_ids.emplace(r_id);
		updateDeps(pattern::Dependency(pattern::Dependency::KAPPA,ptrn->getId()));
	}
	//total update
	//cout << "rules to update: ";
	if(ev.act.t == ActionType::TRANSPORT){
		auto& dest = static_cast<simulation::Simulation*>(parent)->getCell(ev.act.trgt_st);
		for(auto r_id : ev.rule_ids){
			//cout << env.getRules()[r_id].getName() << ", ";
			updateActivity(r_id);
			dest.updateActivity(r_id);
		}
	}
	else
		for(auto r_id : ev.rule_ids)
			updateActivity(r_id);
	//cout << endl;
	counter.incNullActions(ev.null_actions.size());

	ev.clear();
}

void State::advanceUntil(FL_TYPE sync_t,UINT_TYPE max_e) {
	counter.next_sync_at = sync_t;
	plot->fill();
	while(counter.getTime() < sync_t && counter.getEvent() < max_e){
		try{
			const NullEvent ex(event());//next-event
			if(ex.error){
				counter.incNullEvents(ex.error);
				IF_DEBUG_LVL(2,	log_msg = "Null-"+log_msg + "\n---"+ex.what()+"---\n\n");
			}
			else
				counter.incEvents();
		}
		catch(const NullEvent &ex){
			IF_DEBUG_LVL(2,log_msg = "Null-"+log_msg+"\n---"+ ex.what()+ "---\n");
			counter.incNullEvents(ex.error);
		}
		catch(const exception &e){
			std::cout << "[" << log_msg << e.what() << std::endl;
			exit(1);
		}
		activityUpdate();
#ifdef DEBUG
		if((counter.getEvent()+counter.getNullEvent()) % 100 == 0 && params->verbose > 4){
			log_msg += "---------------------------------------------\n";
			log_msg += this->toString();
			log_msg += "\n---------------------------------------------\n";
		}
		size_t pos = 0;
		auto sim_str = "[Sim " + to_string(parent->getId()) + "]: ";
		if(log_msg != "" && params->runs > 1){
			log_msg = sim_str + log_msg;
			while((pos = log_msg.find("\n",pos+1)) != string::npos)
				if(log_msg[pos+1] != '\n' && log_msg[pos+1] != '\0')
					log_msg.insert(pos+1,sim_str);
		}
		std::cout << "[" << log_msg << std::endl;
		log_msg = "";
#endif
		WarningStack::getStack().show(false);
		plot->fill();
	}
}

void State::selectBinaryInj(const simulation::Rule& r,bool clsh_if_un) const {
	set<Node*> total_cod;
	//map<int,SiteGraph::Node*> total_inj;
	//SiteGraph::Node*** total_inj = new SiteGraph::Node**[mix.compsCount()];
	//ev->emb = new Node**[mix.compsCount()];
	auto& mix = r.getLHS();
	ev.cc_count = mix.compsCount();
	IF_DEBUG_LVL(2,log_msg += " (binary) | Root-nodes: ");

	for(unsigned i = 0 ; i < mix.compsCount() ; i++){
		auto& cc = mix.getComponent(i);
		injections[cc.getId()]->selectRule(r.getId(), i);
		auto& inj = injections[cc.getId()]->chooseRandom(rng);
		IF_DEBUG_LVL(2,log_msg += to_string(inj.getEmbedding()[0][0]->getAddress()) + ",";)
		if(inj.codomain(&ev.emb[i],total_cod))
			throw NullEvent(3);//overlapped codomains
	}
	IF_DEBUG_LVL(2,log_msg += "\n");
	if(clsh_if_un && graph.areConnected(ev.emb[0],ev.emb[1],r.getUnaryRate().second)){
		throw NullEvent(2);//selected instance is not binary
	}
	//static MixEmbMap<expressions::Auxiliar,FL_TYPE,Node> mix_map;
	ev.mix_map.clear();
	for(auto& aux_rf : mix.getAuxCoords())
		ev.mix_map[aux_rf.first] = ev.emb[aux_rf.second.cc_pos][aux_rf.second.ag_pos]
				/*Node*/->getInternalValue(aux_rf.second.st_id).valueAs<FL_TYPE>();
	this->setAuxMap(&ev.mix_map);
	return;
}


void State::selectUnaryInj(const simulation::Rule& rule) const {
	auto& mix = rule.getLHS();
	auto& inj = nlInjections.at(mix.getId())->chooseRandom(rng);
	set<Node*> total_cod;

	IF_DEBUG_LVL(2,log_msg +=
		" (unary)  | Root-nodes: " + to_string(inj.getEmbedding()[0][0]->getAddress())
		+ "," + to_string(inj.getEmbedding()[1][0]->getAddress()) + "\n";)


	if(inj.codomain(ev.emb,total_cod)){
		nlInjections.at(mix.getId())->erase(&inj,*this);
		ev.to_update.emplace(&mix);
		throw NullEvent(3);
	}
	//TODO check-connex
	//TODO clear nlInjections
	ev.mix_map.clear();
	for(auto& aux_rf : mix.getAuxCoords())
		ev.mix_map[aux_rf.first] = ev.emb[aux_rf.second.cc_pos][aux_rf.second.ag_pos]
				/*Node*/->getInternalValue(aux_rf.second.st_id).valueAs<FL_TYPE>();
	this->setAuxMap(&ev.mix_map);
	ev.is_unary = true;
	//TODO delete if another way to erase unary-injection is implemented
	nlInjections.at(mix.getId())->erase(&inj,*this);
	ev.to_update.emplace(&mix);
	return;
}

void State::selectInjection(const simulation::Rule& r,two<FL_TYPE> bin_act,
		two<FL_TYPE> un_act) {
	//if mix.is empty TODO
	//if mix.unary -> select_binary
	if(!r.getUnaryRate().first)
		selectBinaryInj(r,false);
	else if(std::isinf(bin_act.first)){
		if(std::isinf(un_act.first)){
			auto rd = uniform_int_distribution<int>(1)(rng);
			if(rd)
				return selectBinaryInj(r,true);
			else
				return selectUnaryInj(r);
		}
		else
			return selectBinaryInj(r,true);
	}
	else{
		if(std::isinf(un_act.first))
			return selectUnaryInj(r);
		else{
			auto rd = uniform_real_distribution<FL_TYPE>(bin_act.first+un_act.first)(rng);
			if(rd > un_act.first)
				return selectBinaryInj(r,true);
			else
				return selectUnaryInj(r);
		}
	}
	return;
}

const simulation::Rule& State::drawRule(){
	auto rid_alpha = activityTree->chooseRandom();
	auto& rule = env.getRule(rid_alpha.first);

	IF_DEBUG_LVL(2,char buf[200];
		sprintf(buf,"|| Rule: %-11.11s ||",rule.getName().c_str());
		log_msg += buf;
	)

	auto a1a2 = rates[rid_alpha.first]->evalActivity(*this);
	auto alpha = a1a2.first + a1a2.second;

	if(alpha == 0.)
		activityTree->add(rid_alpha.first,0.);

	if(alpha < numeric_limits<FL_TYPE>::max()){
		if(alpha > rid_alpha.second){
			//TODO if IntSet.mem rule_id state.silenced then (if !Parameter.debugModeOn then Debug.tag "Real activity is below approximation... but I knew it!") else invalid_arg "State.draw_rule: activity invariant violation"
		}
		auto rd = uniform_real_distribution<FL_TYPE>(0.0,1.0)(rng);
		//auto rd = 0.95; //deterministic
		if(rd > (alpha / rid_alpha.second) ){
			activityTree->add(rid_alpha.first,alpha);
			throw NullEvent(4);//TODO (Null_event 4)) (*null event because of over approximation of activity*)
		}
	}
	int radius = 0;//TODO
	//EventInfo* ev_p;
	selectInjection(rule,make_pair(a1a2.first,radius),
			make_pair(a1a2.second,radius));
	return rule;
}

int State::event() {
	//if(counter.getEvent() == 27511)
	//	cout << "aca!!!"
	//			<< endl;
	FL_TYPE dt,act;
	act = activityTree->total();
	IF_DEBUG_LVL(2,
		char buf[300];
		sprintf(buf,"Event %3lu | Time %.4f | Reactivity = %.4f]\n",
				counter.getEvent(),counter.getTime(),act);
		log_msg += buf;
	)
	IF_DEBUG_LVL(4,
		log_msg += activeRulesStr() + "\n";
	)
	if(act < 0.)
		throw invalid_argument("Activity falls below zero.");
	auto exp_distr = exponential_distribution<FL_TYPE>(act);
	dt = exp_distr(rng);

	//cout << "advance time" << dt << endl;
	counter.advanceTime(dt);
	if(act == 0 || std::isinf(dt))
		return 0;
	//timePerts = pertIds;
	//pertIds.clear();
	FL_TYPE stop_t = 0.0;
#ifdef DEBUG
	if(params->verbose > 1){
		char buf[35];
		sprintf(buf,"|| Time Step: %.4f ||\n",dt);
		log_msg += buf;
	}
#endif
	updateDeps(pattern::Dependency(pattern::Dependency::TIME));
	for(auto& time_dPert : timePerts){//fixed-time-perts
		//cout << "checking time-perts" << endl;
		if(time_dPert.first > counter.getTime())
			break;
		auto p_id = time_dPert.second.id;
		bool abort = false;
		auto& pert = perts.at(p_id);
		stop_t = pert.timeTest(*this);
		IF_DEBUG_LVL(2,dt = stop_t - counter.getTime() + dt;)
		//if(stop_t){
			counter.setTime( (stop_t > 0) ? stop_t : counter.getTime());
			plot->fillBefore();
			//TODO HERE   pert.apply(*this);
			counter.setTime( std::nextafter(counter.getTime(),counter.getTime() + 1.0));//TODO inf?
			abort = pert.testAbort(*this,true);
		//}
		//if(!abort)
		//	abort = pert.testAbort(*this,false);
		if(!abort)
		//	activeDeps.eraseTimePert(time_dPert.first,time_dPert.second);
		//else
			activeDeps.addTimePertDependency(p_id, pert.nextStopTime());
#ifdef DEBUG
		if(params->verbose > 1){
			char buf[35];
			sprintf(buf,"|| Corrected Time Step: %.4f ||\n",dt);
			log_msg += buf;
		}
#endif
	}
	timePerts.clear();
	//cout << "trying to perturbate" << endl;
	tryPerturbate();//trying no-fixed-time perturbations

	//EventInfo* ev = nullptr;
	if(stop_t){
		return 6;//NullEvent caused by perturbation
	}
	//cout << "draw-rule" << endl;
	auto& rule = drawRule();
	plot->fillBefore();

	//cout << "apply rule" << endl;
	apply(rule);
	positiveUpdate(rule.getInfluences());
	//nonLocalUpdate(rule,ev.new_injs);
	return 0;
}

void State::tryPerturbate() {
	while(pertIds.size()){
		auto curr_perts = pertIds;
		pertIds.clear();
		for(auto p_id : curr_perts){
			bool abort = false;
			auto& pert = perts.at(p_id);
			auto trigger = pert.test(*this);
			if(trigger){
				//TODO here pert.apply(*this);
				abort = pert.testAbort(*this,true);
			}
			if(!abort)
				abort = pert.testAbort(*this,false);
			//if(abort)
			//	perts.erase(p_id);

		}
	}
}

void State::updateDeps(const pattern::Dependency& d){
	list<pattern::Dependency> deps;//TODO try with references
	deps.emplace_front(d);
	pattern::Dependency last_dep;
	auto dep_it = deps.begin();
	while(!deps.empty()){
		//auto& dep = *deps.begin();
		IF_DEBUG_LVL(5,log_msg += "\t#Triggering updates from " + dep_it->toString() + " Dependency.\n";)
		switch(dep_it->type){
		case Deps::RULE:
			IF_DEBUG_LVL(4,log_msg += "Updating '"+env.getRule(dep_it->id).getName()+"' rule's rate.\n";)
			ev.rule_ids.emplace(dep_it->id);
			break;
		case Deps::KAPPA:
			break;
		case Deps::VAR:
			break;
		case Deps::PERT:
			if(perts.count(dep_it->id))
				pertIds.push_back(dep_it->id);
			else //pert was aborted
				activeDeps.erase(*dep_it,last_dep);
			break;
		case Deps::TIME:{
			auto& deps = activeDeps.getDependencies(*dep_it).ordered_deps;
			if(deps.size() && deps.begin()->first <= counter.getTime() ){
				auto nextDeps = deps.equal_range(deps.begin()->first);
				timePerts.insert(timePerts.begin(),nextDeps.first,nextDeps.second);
				activeDeps.eraseTimePerts(nextDeps.first,nextDeps.second);
			}
			}break;
		case Deps::NONE://simulation ends
			break;
		case Deps::AUX:
			break;
		}
		auto& more_deps = activeDeps.getDependencies(*dep_it).deps;
		deps.insert(deps.end(),more_deps.begin(),more_deps.end());
		last_dep = *dep_it;
		dep_it = deps.erase(dep_it);
	}
}

void State::initInjections() {
	if(injections){
		for(unsigned i = 0; i < env.size<pattern::Mixture::Component>(); i++)
			delete injections[i];
		delete[] injections;
	}
	injections = new CcInjRandContainer*[env.size<pattern::Mixture::Component>()];
	for(auto cc : env.getComponents())
		if(cc->getRateDeps().size()){
			injections[cc->getId()] = new InjRandTree<CcInjection>(*cc,rates);
			//try{
			for(auto node_p : graph){
				map<int,matching::InjSet*> port_list;
				if(cc->getAgent(0).getId() != node_p->getId())//very little speed-up
					continue;
				//cout << comp.toString(env) << endl;
				auto inj_p = injections[cc->getId()]->emplace(*this,node_p,port_list);

				if(inj_p){
					//if(port_list.empty())
					node_p->addDep(inj_p);
					for(auto port : port_list)
						port.second->emplace(inj_p);
				}
			}
			//}catch(exception &e){
			//	throw e;
			//}
		}
		else	//inj-containers are initialized when need to be count (useful for perturbation mixes)
			injections[cc->getId()] = new InjLazyContainer<CcInjection>(*cc,*this,graph,injections[cc->getId()]);
}

void State::initUnaryInjections(){
	map<two<matching::Injection*>,int> unary_instances;
	auto& unary_mixes = env.getUnaryMixtures();
	auto& mixtures = env.getMixtures();
	matching::InjRandSet<MixInjection>* inj_cont;
	for(auto& un_mix : unary_mixes){
		auto ccs = un_mix.first;
		if(ccs.first > ccs.second )
			continue;
		//initializing lazy containers
		injections[ccs.first]->count();
		injections[ccs.second]->count();
		int r = 0;
		inj_cont = nullptr;
		for(auto r_mix : un_mix.second){
			if(r != r_mix.first){
				inj_cont = new InjRandSet<MixInjection>(*mixtures[r_mix.second],inj_cont);
				r = r_mix.first;
			}
			nlInjections[r_mix.second] = inj_cont;
		}
	}
	for(auto& cc_cc : env.getUnaryCCs()){
		bool rev = false;
		for(auto ccc : cc_cc.second)
			if(ccc.second < cc_cc.first)
				rev = true; /// TODO break for speed-up
		if(rev)
			continue;
		auto cc = env.getComponents()[cc_cc.first];
		auto& cc_candidates = cc_cc.second;
		//tests if theres an injection of a cc_candidate in inj neighborhood
		auto test = [&] (const Injection* inj) -> void {
			auto emb = inj->getEmbedding()[0];
			set<Node*> visited;
			map<int,list<Node*>> nb;
			auto& start = nb[0];
			start.insert(start.begin(),emb.begin(),emb.end());
			graph.neighborhood(nb,visited,cc_candidates.back().first);
			set<pair<Injection*,int>> inj_candidates;
			auto candidate = cc_candidates.begin();
			for(auto& r_nodes : nb){
				if(r_nodes.first == 0)
					continue;
				for(auto node : r_nodes.second){//fill inj_candidates with neighborhood injs
					for(auto inj : node->getLifts())
						inj_candidates.emplace(inj,r_nodes.first);
				}
				while(candidate != cc_candidates.end() && candidate->first <= r_nodes.first){
					for(auto inj2_r : inj_candidates)
						if(inj2_r.first->pattern().getId() == candidate->second){
							//injection pair has a non local injection
							//TODO at this point, cc-injs should be deleted from cc-inj containers...
							two<int> key(cc_cc.first,candidate->second);
							nlInjections[unary_mixes.at(key).front().second]->emplace(*this,inj,inj2_r.first,inj2_r.second);
						}
					candidate++;
				}
			}
			while(candidate != cc_candidates.end()){
				for(auto inj2_r : inj_candidates)//last time
					if(inj2_r.first->pattern().getId() == candidate->second){
						two<int> key(cc_cc.first,candidate->second);
						nlInjections[unary_mixes.at(key).front().second]->emplace(*this,inj,inj2_r.first,inj2_r.second);
					}
				candidate++;
			}
		};
		//testing on every inj of this cc
		injections[cc->getId()]->fold(test);
	}
}

void State::initActTree() {
	if(activityTree)
		delete activityTree;
	int rule_count = env.getRules().size();
	//if(rule_count > 4)
		activityTree = new data_structs::MyMaskedBinaryRandomTree<stack>(rule_count,rng);
	//else
	//	activityTree = new data_structs::SimpleTree(rule_count);
	list<const simulation::Rule*> rules;
	for(auto rule : env.getRules())
		rules.emplace_back(rule);
	//Sorting because ...
	rules.sort([](const simulation::Rule * a, const simulation::Rule* b) { return a->getName() < b->getName(); });
	for(auto rule_p : rules){
		auto& rule = *rule_p;
		auto act_pr = rates[rule.getId()]->evalActivity(*this);
		activityTree->add(rule.getId(),act_pr.first+act_pr.second);
		//cout << "rule " << rule_p->getName() << "->" << (act_pr.first+act_pr.second) << endl;
	}
/*#ifdef DEBUG
	if(simulation::Parameters::get().verbose <= 0)
		return;
	cout << "[Initial activity tree]" << endl;
	for(auto rule_p : rules){
		auto& rule = *rule_p;
		auto act_pr = rates[rule.getId()]->evalActivity(args);
		if(simulation::Parameters::get().verbose > 0)
			printf("\t%s\t%.6f\n", rule.getName().c_str(),(act_pr.first+act_pr.second));
	}
	cout << endl;
#endif*/
}

void State::plotOut() const {
	plot->fill();
}

data_structs::DataTable* State::getTrajectory() const {
	auto tab = new data_structs::DataTable(plot->getData(),true);
	//tab->col_names.emplace_back("Time");
	for(auto var : env.getObservables())
		tab->col_names.emplace_back(var->toString());
	return tab;
}

void State::collectRawData(map<string,list<const vector<FL_TYPE>*>> &raw_list) const {
	for(auto& pert : perts)
		pert.second.collectRawData(raw_list);
}
void State::collectTabs(map<string,list<const data_structs::DataTable*>> &tab_list) const {
	for(auto& pert : perts)
		pert.second.collectTabs(tab_list);
}

void State::print() const {
	graph.print(env);
	cout << "Active Injections -> {\n";
	int i = 0;
	for(auto& cc : env.getComponents()){
		i = cc->getId();
		if(injections[i]->count())
			cout << "\t("<< i <<")\t" << injections[i]->count() <<
			" injs of " << cc->toString(env) << endl;
	}
	for(auto& mix : env.getMixtures()){
		i = mix->getId();
		matching::InjRandContainer<matching::MixInjection>* injs = nullptr;
		try {
			injs = nlInjections.at(i);
		}
		catch(std::out_of_range& e){ }
		if(injs && injs->count())
			cout << "\t("<< i <<")\t" << injs->count() <<
			" mix-injs of " << mix->toString(env) << endl;
	}
	cout << "}\nActive Rules -> {\n";
	for(auto& r : env.getRules()){
		auto act = rates[r->getId()]->evalActivity(*this);
		if(act.first || act.second)
			cout << "\t(" << r->getId() << ")\t" << r->getName() << "\t("
				<< act.first << " , " << act.second << ")" << endl;
	}

	cout << "}\n";
	cout << counter.toString() << endl;
}

string State::activeRulesStr() const {
	string ret;// = "Active Rules -> {\n";
	for(auto r : env.getRules()){
		auto act = rates[r->getId()]->evalActivity(*this);
		auto sim = static_cast<const simulation::Simulation*>(parent);
		auto tree_act = sim->activityTree->find(sim->maskSpatialRule(id,r->getId()));
		if(act.first || act.second || tree_act)
			ret += "\t(" + to_string(r->getId()) + ")\t" + r->getName() + "\t("
				+ to_string(act.first) + " , " + to_string(act.second) +
				") |" + to_string(tree_act)+"\n"; //checking tree_act problems
	}
	return ret;// + "}";
}

string State::toString() const {
	string ret = graph.toString(env);
	ret += "Active Injections -> {\n";
	int i = 0;
	for(auto& cc : env.getComponents()){
		i = cc->getId();
		if(injections[i]->count())
			ret += "\t("+ to_string(i) +")\t" + to_string(injections[i]->count()) +
			" injs of " + cc->toString(env) + "\n";
	}
	for(auto& mix : env.getMixtures()){
		i = mix->getId();
		matching::InjRandContainer<matching::MixInjection>* injs = nullptr;
		try {
			injs = nlInjections.at(i);
		}
		catch(std::out_of_range& e){ }
		if(injs && injs->count())
			ret += "\t(" + to_string(i) +")\t" + to_string(injs->count()) +
			" mix-injs of " + mix->toString(env) + "\n";
	}
	ret += "}\nActive Rules -> {\n";
	for(auto& r : env.getRules()){
		auto act = rates[r->getId()]->evalActivity(*this);
		if(act.first || act.second)
			ret += "\t(" + to_string(r->getId()) + ")\t" + r->getName() + "\t("
				+ to_string(act.first) + " , " + to_string(act.second) + ")\n" ;
	}
	ret += "}\n" + counter.toString();
	return ret;
}

} /* namespace ast */
