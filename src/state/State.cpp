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
#include "../simulation/Plot.h"

#include "../pattern/mixture/Component.h"
#include "../pattern/mixture/Agent.h"
#include <cmath>

namespace state {
using Deps = pattern::Dependency::Dep;

State::State(const simulation::Simulation& _sim,
		const std::vector<Variable*>& _vars,
		const BaseExpression& vol,simulation::Plot& _plot,
		const pattern::Environment& _env,int seed) :
	sim(_sim),env(_env),volume(vol),vars(_vars.size()),rates(env.size<simulation::Rule>()),
	tokens (new float[env.size<state::TokenVar>()]()),activityTree(nullptr),
	injections(nullptr),rng(seed),ev(),
	plot(_plot),activeDeps(env.getDependencies()),
	args(this,&vars,&ev.aux_map){
	for(unsigned i = 0; i < _vars.size(); i++){
		vars[i] = dynamic_cast<Variable*>(_vars[i]->clone());
		vars[i]->reduce(vars);//vars are simplified for every state
	}
	for(auto& rule : env.getRules()){
		int i = rule.getId();
		auto& lhs = rule.getLHS();
		auto lhs_comps = lhs.compsCount();
		bool same_ptrn = false;
		for(int cc_id = 0; cc_id < int(lhs_comps)-1; cc_id++)
			if(&(lhs.getComponent(cc_id)) != &(lhs.getComponent(cc_id+1)) )
				{same_ptrn = false; break;}
			else
				same_ptrn = true;

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
		graph.addComponents(n,*comp_p,args);
	}
}

void State::addNodes(unsigned n,const pattern::Mixture& mix){
	for(auto comp_p : mix)//give ev.emb to save new nodes for positive update
		graph.addComponents(n,*comp_p,args,ev.emb[0]);
}

UINT_TYPE State::mixInstances(const pattern::Mixture& mix) const{
	//auto& mix = env.getMixtures().front();
	UINT_TYPE  count = 1;
	for(const auto cc : mix){
		count *= injections[cc->getId()]->count();
	}
	return count;
}


void State::apply(const simulation::Rule& r){
	ev.aux_map.setEmb(ev.emb);

	int i = 0;
	for(auto n : r.getNewNodes()){
		auto node = new Node(*n,ev.new_cc);
		ev.new_cc[n] = node;
		graph.allocate(node);
		ev.emb[ev.cc_count][i] = node;
		i++;
	}
	ev.new_cc.clear();
	for(auto& act : r.getScript()){
		ev.act = act;
		auto node = ev.emb[act.trgt_ag.first][act.trgt_ag.second];
		switch(act.t){
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
			node_pair.second->copyDeps(*node_pair.first,ev,injections,args);
			graph.allocate(node_pair.second);
		}
	}
	//apply token changes
	for(auto& change : r.getTokenChanges()){
		tokens[change.first] += change.second->getValue(args).valueAs<FL_TYPE>();
		updateDeps(pattern::Dependency(Deps::TOK,change.first));
	}
}

void State::positiveUpdate(const simulation::Rule::CandidateMap& wake_up){
	//TODO vars_to_wake_up
	//auto& wake_up = r.getInfluences();
	for(auto& key_info : wake_up){
		auto key = key_info.first;
		auto info = key_info.second;
		Node* node = ev.emb[info.emb_coords.first][info.emb_coords.second];
		auto nulls = ev.null_actions.equal_range(node);//TODO can be optimized to only call once (candidate key doble mapping)
		//if(info.infl_by.size() && distance(nulls.first,nulls.second) == info.infl_by.size())
		//	continue;	//every action applied to node is null
		map<int,matching::InjSet*> port_list;
		auto inj_p = injections[key.cc->getId()]->emplace(*node,port_list,args,key.match_root.first);

		if(inj_p){
			for(;nulls.first != nulls.second;++nulls.first)
				port_list.erase(nulls.first->second);
			if(port_list.empty())//TODO check cc.size()
				node->addDep(inj_p);
			else
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
			auto inj_p = injections[cc_ag.first->getId()]->emplace(*side_eff.first,port_list,args,cc_ag.second);
			if(inj_p){
				if(port_list.empty())
					side_eff.first->addDep(inj_p);
				else{
					for(auto port : port_list)
						port.second->emplace(inj_p);
				}
				ev.to_update.emplace(cc_ag.first);
			}
		}
	}

	//prepare negative_update
	//set<small_id> rule_ids;
	for(auto cc : ev.to_update){
		for(auto r_id : cc->includedIn())
			ev.rule_ids.emplace(r_id);
	}
	//total update
	//cout << "rules to update: ";
	for(auto r_id : ev.rule_ids){
		//cout << env.getRules()[r_id].getName() << ", ";
		auto act = rates[r_id]->evalActivity(args);
		activityTree->add(r_id,act.first+act.second);
	}
	//cout << endl;
	counter.incNullActions(ev.null_actions.size());

	ev.clear();
}

void State::advanceUntil(FL_TYPE sync_t) {
	counter.next_sync_at = sync_t;
	plot.fill(*this,env);
	while(counter.getTime() < sync_t){
		try{
			const NullEvent ex(event());
			if(ex.error){
				counter.incNullEvents(ex.error);
				#ifdef DEBUG
					cout << "\tnull-event (" << ex.what() << ")" << endl;
				#endif
			}
			else
				counter.incEvents();
		}
		catch(const NullEvent &ex){
			#ifdef DEBUG
				cout << "\tnull-event (" << ex.what() << ")" << endl;
			#endif
			counter.incNullEvents(ex.error);
		}
#ifdef DEBUG
		if(counter.getEvent() % 10 == 0)
			this->print();
#endif
		WarningStack::getStack().show(false);
		plot.fill(*this,env);
	}
}

void State::selectBinaryInj(const simulation::Rule& r,bool clsh_if_un) const {
	set<Node*> total_cod;
	//map<int,SiteGraph::Node*> total_inj;
	//SiteGraph::Node*** total_inj = new SiteGraph::Node**[mix.compsCount()];
	//ev->emb = new Node**[mix.compsCount()];
	auto& mix = r.getLHS();
	ev.cc_count = mix.compsCount();
	for(unsigned i = 0 ; i < mix.compsCount() ; i++){
		auto& cc = mix.getComponent(i);
		injections[cc.getId()]->selectRule(r.getId(), i);
		auto& inj = injections[cc.getId()]->chooseRandom(rng);
		try{
			inj.codomain(ev.emb[i],total_cod);
		}
		catch(False& ex){
			throw NullEvent(2);//overlapped codomains
		}
	}
	if(clsh_if_un)
		return;//TODO
	return;
}

void State::selectUnaryInj(const simulation::Rule& mix) const {
	return;
}

void State::selectInjection(const simulation::Rule& r,two<FL_TYPE> bin_act,
		two<FL_TYPE> un_act) {
	//if mix.is empty TODO
	//if mix.unary -> select_binary
	if(std::isinf(bin_act.first)){
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
	auto& rule = env.getRules()[rid_alpha.first];

	auto a1a2 = rates[rid_alpha.first]->evalActivity(args);
	auto alpha = a1a2.first + a1a2.second;

	//*?
	if(alpha == 0.)
		activityTree->add(rid_alpha.first,0.);

	if(!std::isinf(alpha)){
		if(alpha > rid_alpha.second){
			//TODO if IntSet.mem rule_id state.silenced then (if !Parameter.debugModeOn then Debug.tag "Real activity is below approximation... but I knew it!") else invalid_arg "State.draw_rule: activity invariant violation"
		}
		auto rd = uniform_real_distribution<FL_TYPE>(0.0,1.0)(rng);
		if(rd > (alpha / rid_alpha.second) ){
			activityTree->add(rid_alpha.first,alpha);
			throw NullEvent(3);//TODO (Null_event 3)) (*null event because of over approximation of activity*)
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
	if(act < 0.)
		throw invalid_argument("Activity falls below zero.");
	auto exp_distr = exponential_distribution<FL_TYPE>(act);
	dt = exp_distr(rng);

	counter.advanceTime(dt);
	if(act == 0 || std::isinf(dt))
		return 0;
	updateDeps(pattern::Dependency(pattern::Dependency::TIME));
	//timePerts = pertIds;
	//pertIds.clear();
	FL_TYPE stop_t = 0.0;
	for(auto& time_dPert : timePerts){//fixed-time-perts
		if(time_dPert.first > counter.getTime())
			break;
		auto p_id = time_dPert.second.id;
		bool abort = false;
		auto& pert = perts.at(p_id);
		stop_t = pert.timeTest(args);
		//if(stop_t){
			counter.time = (stop_t > 0) ? stop_t : counter.time;
			plot.fillBefore(*this,env);
			pert.apply(*this);
			counter.time = std::nextafter(counter.time,counter.time + 1.0);//TODO inf?
			abort = pert.testAbort(args,true);
		//}
		//if(!abort)
		//	abort = pert.testAbort(*this,false);
		if(abort)
			perts.erase(p_id);
		else
			activeDeps.addTimePertDependency(p_id, pert.nextStopTime());
	}
	timePerts.clear();
	tryPerturbate();//trying no-fixed-time perturbations

	#ifdef DEBUG
		printf("Event %3lu | Time %.4f \t  act = %.4f",
				counter.getEvent(),counter.getTime(),act);
	#endif
	//EventInfo* ev = nullptr;
	if(stop_t)
		return 6;//NullEvent caused by perturbation
	auto& rule = drawRule();
	plot.fillBefore(*this,env);

	#ifdef DEBUG
		printf( "  | Rule: %-11.11s",rule.getName().c_str());
		cout << "  Root-node: ";
		for(int i = 0; i < ev.cc_count; i++)
			cout << ev.emb[i][0]->getAddress() << ",";
		cout << endl;
		//printf("  Root-node: %03lu\n",(ev->cc_count ?
		//		ev->emb[0][0]->getAddress() : -1L));
	#endif
	apply(rule);
	positiveUpdate(rule.getInfluences());

	return 0;
}

void State::tryPerturbate() {
	while(pertIds.size()){
		auto curr_perts = pertIds;
		pertIds.clear();
		for(auto p_id : curr_perts){
			bool abort = false;
			auto& pert = perts.at(p_id);
			auto trigger = pert.test(args);
			if(trigger){
				pert.apply(*this);
				abort = pert.testAbort(args,true);
			}
			if(!abort)
				abort = pert.testAbort(args,false);
			if(abort)
				perts.erase(p_id);

		}
	}
}

void State::updateVar(const Variable& var,bool by_value){
	//delete vars[var.getId()];
	if(by_value)
		vars[var.getId()]->update(var.getValue(args));
	else
		vars[var.getId()]->update(var);
	updateDeps(pattern::Dependency(Deps::VAR,var.getId()));
}

void State::updateDeps(const pattern::Dependency& d){
	list<pattern::Dependency> deps;//TODO try with references
	deps.emplace_front(d);
	pattern::Dependency last_dep;
	auto dep_it = deps.begin();
	while(!deps.empty()){
		//auto& dep = *deps.begin();
		switch(dep_it->type){
		case Deps::RULE:
			ev.rule_ids.emplace(dep_it->id);
			break;
		case Deps::KAPPA:
			break;
			//auto act = evalActivity(env.getRules()[dep.id]);
			//activityTree->add(dep.id,act.first+act.second);
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
			if(deps.size() && deps.begin()->first <= counter.time ){
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
	injections = (matching::InjRandContainer**)(new matching::InjRandContainer*[env.size<pattern::Mixture::Component>()]);
	for(auto cc : env.getComponents())
		if(cc->getRateDeps().size()){
			injections[cc->getId()] = new matching::InjRandTree(*cc,rates);
			//try{
			for(auto node_p : graph){
				map<int,matching::InjSet*> port_list;
				if(cc->getAgent(0).getId() != node_p->getId())//very little speed-up
					continue;
				//cout << comp.toString(env) << endl;
				auto inj_p = injections[cc->getId()]->emplace(*node_p,port_list,args);

				if(inj_p){
					if(port_list.empty())
						node_p->addDep(inj_p);
					else{
						for(auto port : port_list)
							port.second->emplace(inj_p);
					}
				}
			}
			//}catch(exception &e){
			//	throw e;
			//}
		}
		else
			injections[cc->getId()] = new matching::InjLazyContainer(*cc,*this,graph,injections[cc->getId()]);
}

void State::initActTree() {
	if(activityTree)
		delete activityTree;
	activityTree = new data_structs::MyMaskedBinaryRandomTree<stack>(env.size<simulation::Rule>(),rng);
#ifdef DEBUG
	cout << "[Initial activity tree]" << endl;
#endif
	list<const simulation::Rule*> rules;
	for(auto& rule : env.getRules())
		rules.emplace_back(&rule);
	rules.sort([](const simulation::Rule * a, const simulation::Rule* b) { return a->getName() < b->getName(); });
	for(auto rule_p : rules){
		auto& rule = *rule_p;
		auto act_pr = rates[rule.getId()]->evalActivity(args);
		activityTree->add(rule.getId(),act_pr.first+act_pr.second);
#ifdef DEBUG
		printf("\t%s\t%.6f\n", rule.getName().c_str(),(act_pr.first+act_pr.second));
#endif
	}
}


void State::print() const {
	cout << "state with {SiteGraph.size() = " << graph.getPopulation();
	//cout << "\n\tvolume = " << volume.getValue().valueAs<FL_TYPE>();
	cout << "\n\tInjections {\n";
	int i = 0;
	for(auto& cc : env.getComponents()){
		if(injections[i]->count())
			cout << "\t("<< i <<")\t" << injections[i]->count() <<
			" injs of " << cc->toString(env) << endl;
		i++;
	}
	cout << "\t}\n\tRules {\n";
	for(auto& r : env.getRules()){
		auto act = rates[r.getId()]->evalActivity(args);
		if(act.first || act.second)
			cout << "\t(" << r.getId() << ")\t" << r.getName() << "\t("
				<< act.first << " , " << act.second << ")" << endl;
	}
	cout << "\t}\n}\n";
	cout << counter.toString() << endl;
	cout << graph.toString(env) << endl;
}

} /* namespace ast */
