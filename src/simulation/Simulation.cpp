/*
 * Simulation.cpp
 *
 *  Created on: Apr 19, 2016
 *      Author: naxo
 */

#include "Simulation.h"
#include "../state/Variable.h"
#include "../data_structs/DataTable.h"
#include "../data_structs/MyMaskedBinaryRandomTree.h"
#include "../state/Node.h"
#include <set>
#include <boost/random.hpp>

namespace simulation {

/*Simulation::Simulation(pattern::Environment& _env,VarVector& _vars,int _id) :
		SimContext(_id,Parameters::get().seed+_id),env(_env),plot(env,_id),
		ccInjections(nullptr),mixInjections(nullptr),done(false){}*/

Simulation::Simulation(SimContext& context,int _id) :
		SimContext(_id,&context,&counter),activityTree(nullptr),
		ccInjections(nullptr),mixInjections(nullptr),
		activeDeps(env.getDependencies()),done(false){
	if(params->maxTime >= std::numeric_limits<FL_TYPE>::infinity())
		if(params->maxEvent == std::numeric_limits<UINT_TYPE>::infinity())
			throw invalid_argument("No limit to stop or control simulation.");
		else
			plot = new EventPlot(*this);
	else
		plot = new TimePlot(*this);

	for(unsigned i = 0; i < vars.size(); i++){
		vars[i] = dynamic_cast<state::Variable*>(vars[i]->clone());
		vars[i]->reduce(*this);//vars are simplified for every state
	}

	for(auto pert : env.getPerts()){
		perts.emplace(pert->getId(),*pert);
	}
}

/*Simulation::Simulation() : SimContext(0,Parameters::get().seed),
		plot(*env,0),ccInjections(nullptr),mixInjections(nullptr),
		done(false) {}
*/
Simulation::~Simulation() {
//	cout << "deleting Simulation[" << id << "]." << endl;
	delete[] ccInjections;
	delete[] mixInjections;
	for(auto cell : cells)
		delete cell;
}

//void Simulation::setCells(const list<unsigned int>& _cells,const VarVector& vars){}

void Simulation::initialize(){
	int pid = 0;
	/*for(auto& param : env.getParams()){
		auto var = getVars()[env.getVarId(param.first)];
		//if(var == nullptr)
			var = new state::ConstantVar<FL_TYPE>(pid,param.first,
					param.second->getValue(*this).valueAs<FL_TYPE>());
		pid++;
	}
	for(auto var : parent->getVars()){
		//todo set sim-vars
	}*/
	//auto distr = uniform_int_distribution<int>();
	for(int i = 0; i < env.getCellCount(); i++){//TODO all cells
		/*cells.emplace(piecewise_construct,forward_as_tuple(cell_id),
				forward_as_tuple((int)cell_id,*this,
				env.getCompartmentByCellId(cell_id).getVolume(),*plot));*/
		cells.push_back(new state::State(i,*this,env.getCompartmentByCellId(i).getVolume(),
				/**new expressions::Constant<int>(1)*/plot->getType() == "Event"));
		subContext.push_back(cells.back());
	}
	IF_DEBUG_LVL(4,cout << "[Sim "+to_string(id)+ "]: Agent Initialization" << endl);
	for(auto& init : env.getInits()){
		auto &cells = env.getUseExpression(init.use_id).getCells();
		auto n_value = init.n->getValueSafe(*this);
		//cout << n_value << " init agents or tok\n" << endl;
		if(init.mix){
			int n;
			if(n_value.t == Type::FLOAT){
				n = round(n_value.fVal);
				if(n != n_value.fVal)
					ADD_WARN_NOLOC("Making approximation of a float value in agent initialization to "+
							to_string(n)+" ("+to_string(n_value.fVal)+")");
			}
			else
				n = n_value.valueAs<int>();
			if(n < 0)
				throw invalid_argument("Initializing "+to_string(n)+" Agent(s) " + init.mix->toString(env));
			addAgents(cells,n,*init.mix);
		}
		else {
			vars[init.tok_id]->update(n_value);
		}
	}
	IF_DEBUG_LVL(4,cout << "[Sim "+to_string(id)+ "]: Activity Tree Initialization" << endl);
	if(activityTree)
		delete activityTree;
	int rule_count = env.getRules().size();
	//spatial activity tree
	activityTree = new data_structs::MyMaskedBinaryRandomTree<stack>(rule_count*cells.size(),rng);
	list<const simulation::Rule*> rules;

	env.buildInfluenceMap(*this);
	for(auto cell : cells){
		//env.buildInfluenceMap(*cell);
		cell->initInjections();
		cell->initUnaryInjections();
		cell->initActTree();
		for(int i = 0; i < rule_count; i++)
			activityTree->add(maskSpatialRule(cell->getId(),i),cell->activityTree->find(i));
		cell->plot->fill();
	}
	plot->fill();
}


void Simulation::run(){
	FL_TYPE pert_t,stop_t = params->maxTime;
	counter.next_sync_at = stop_t;
	auto stop_e = params->maxEvent;
	while(counter.getTime() < stop_t && counter.getEvent() < stop_e){
		try{
			pert_t = advanceTime();
			tryPerturbate();
			if(!pert_t){
				auto cid_rid = drawRule();
				if(cid_rid.first >= 0){
					plot->fillBefore();
					cells[cid_rid.first]->plot->fillBefore();
					auto& rule = env.getRule(cid_rid.second);
					cells[cid_rid.first]->apply(rule);
					cells[cid_rid.first]->positiveUpdate(rule.getInfluences());
					log_msg += cells[cid_rid.first]->log_msg;
					cells[cid_rid.first]->log_msg = "";
					counter.incEvents();
				}
				else {
					NullEvent ex(cid_rid.second);
					counter.incNullEvents(cid_rid.second);
					IF_DEBUG_LVL(2,	log_msg = "Null-"+log_msg + "\n---"+ex.what()+"---\n\n");
				}
			}
			else {
				NullEvent ex(6);
				counter.incNullEvents(6);
				IF_DEBUG_LVL(2,	log_msg = "Null-"+log_msg + "\n---"+ex.what()+"---\n\n");
			}
		}
		catch(const NullEvent &ex){
			IF_DEBUG_LVL(2,log_msg = "Null-"+log_msg+"\n---"+ ex.what()+ "---\n");
			counter.incNullEvents(ex.error);
		}
		/*catch(const exception &e){
			std::cout << "[" << log_msg << e.what() << std::endl;
			exit(1);
		}*/
		activityUpdate();
#ifdef DEBUG
		if((counter.getEvent()+counter.getNullEvent()) % 100 == 0 && params->verbose > 4){
			log_msg += "---------------------------------------------\n";
			//log_msg += this->toString();
			log_msg += "\n---------------------------------------------\n";
		}
		size_t pos = 0;
		auto sim_str = "[Sim " + to_string(parent->getId()) + "]: ";
		if(log_msg != ""){
			if(params->runs > 1){
				log_msg = sim_str + log_msg;
				while((pos = log_msg.find("\n",pos+1)) != string::npos)
					if(log_msg[pos+1] != '\n' && log_msg[pos+1] != '\0')
						log_msg.insert(pos+1,sim_str);
			}
			std::cout << "[" << log_msg << std::endl;
		}
		log_msg = "";
#endif
		WarningStack::getStack().show(false);
		plot->fill();
		if(ev.comp_id != -1)
			cells[ev.comp_id]->plot->fill();
	}
	done = true;
	for(auto cell : cells){
		cell->tryPerturbate();
		cell->plotOut();
	}
	plot->fill();
	#pragma omp critical
	if(params->verbose > 0 && id == 0){
		if(params->runs > 1)
			cout << "[Sim 0]: ";
		cout << "=== Final-State === \n";
		print();
		cout << "============================ \n";
	}
}

FL_TYPE Simulation::advanceTime() {
	//if(counter.getEvent() == xx) cout << "bad event!!!" << endl;
	FL_TYPE dt,act;
	act = activityTree->total();
	IF_DEBUG_LVL(2,
		char buf[300];
		sprintf(buf,"Event %3lu | Time %.4f | Reactivity = %.4f]\n",
				counter.getEvent(),counter.getTime(),act);
		log_msg += buf;
	)
	IF_DEBUG_LVL(4,
		for(auto cell : cells)
			log_msg += env.cellIdToString(cell->getId())+" | " + cell->activeRulesStr() + "\n";
	)
	if(act < 0.)
		throw invalid_argument("Activity falls below zero.");
	auto exp_distr = exponential_distribution<FL_TYPE>(act);
	dt = exp_distr(rng);

	//cout << "advance time" << dt << endl;
	counter.advanceTime(dt);
	if(act == 0 || std::isinf(dt))
		throw NullEvent(0);
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
		counter.setTime( (stop_t > 0) ? stop_t : counter.getTime());
		plot->fillBefore();
		pert.apply(*this);
		counter.setTime( std::nextafter(counter.getTime(),counter.getTime() + 1.0));//TODO inf?
		abort = pert.testAbort(*this,true);
		if(!abort)
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
	return stop_t;
}

pair<int,int> Simulation::drawRule() {
	auto id_a = activityTree->chooseRandom();
	auto comp_rid = unmaskSpatialRule(id_a.first);

	ev.comp_id = comp_rid.first;
	IF_DEBUG_LVL(2,char buf[200];
		sprintf(buf,"|| Rule: %-20.20s |",env.getRule(comp_rid.second).getName().c_str());
		if(cells.size() > 1)
			log_msg += "|| " + env.cellIdToString(ev.comp_id) +" ";
		log_msg = log_msg + buf;
	)

	auto a1a2 = cells[comp_rid.first]->getRuleActivity(comp_rid.second);
	auto alpha = a1a2.first + a1a2.second;

	if(alpha == 0.)
		activityTree->add(id_a.first,0.);

	if(alpha < numeric_limits<FL_TYPE>::max()){
		if(alpha > id_a.second){
			//TODO if IntSet.mem rule_id state.silenced then (if !Parameter.debugModeOn then Debug.tag "Real activity is below approximation... but I knew it!") else invalid_arg "State.draw_rule: activity invariant violation"
		}
		auto rd = uniform_real_distribution<FL_TYPE>(0.0,1.0)(rng);
		//auto rd = 0.95; //deterministic
		if(rd > (alpha / id_a.second) ){
			activityTree->add(id_a.first,alpha);
			throw NullEvent(4);//TODO (Null_event 4)) (*null event because of over approximation of activity*)
		}
	}
	int radius = 0;//TODO
	//EventInfo* ev_p;
	cells[comp_rid.first]->selectInjection(comp_rid.second,make_pair(a1a2.first,radius),
			make_pair(a1a2.second,radius));
	return comp_rid;
}


void Simulation::tryPerturbate() {
	while(pertIds.size()){
		auto curr_perts = pertIds;
		pertIds.clear();
		for(auto p_id : curr_perts){
			bool abort = false;
			auto& pert = perts.at(p_id);
			auto trigger = pert.test(*this);
			if(trigger){
				pert.apply(*this);
				abort = pert.testAbort(*this,true);
			}
			if(!abort)
				abort = pert.testAbort(*this,false);
			//if(abort)
			//	perts.erase(p_id);

		}
	}
}


using Deps = pattern::Dependency::Dep;

void Simulation::updateDeps(const pattern::Dependency& d){
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


void Simulation::activityUpdate(){
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
		//auto& dest = static_cast<simulation::Simulation*>(parent)->getCell(ev.act.trgt_st);
		for(auto r_id : ev.rule_ids){
			//cout << env.getRules()[r_id].getName() << ", ";
			updateActivity(ev.comp_id,r_id);
			updateActivity(ev.act.trgt_st,r_id);
		}
	}
	else
		for(auto r_id : ev.rule_ids)
			updateActivity(ev.comp_id,r_id);
	//cout << endl;
	counter.incNullActions(ev.null_actions.size());

	ev.clear();
}

/*
void simulation::parallelRun(){
	//updates
	//TODO while(counter.getTime() < params.limitTime()){
		//calculate Tau-Leaping
			//calculate map [species -> diffusion-to-cells array]
			//map-diffusion-in = scatter map-diffusion-to?
		auto tau = params->maxTime;// = calculate-tau( map-diffusion-in )
		//parallel
		for(auto& id_state : cells){
			id_state.second->advanceUntil(counter.getTime()+tau,params->maxEvent);
			id_state.second->updateDeps(pattern::Dependency());
		}
		done = true;
		for(auto& id_state : cells){
			id_state.second->tryPerturbate();
			id_state.second->plotOut();
		}
		#pragma omp critical
		if(params->verbose > 0 && id == 0){
			if(params->runs > 1)
				cout << "[Sim 0]: ";
			cout << "=== Final-State === \n";
			print();
			cout << "============================ \n";
		}
		counter.advanceTime(tau);
	//}
}
*/
int Simulation::getId() const {
	return id;
}

//TODO
template <typename T>
list<T> Simulation::allocParticles(unsigned cells,T count,const list<T>* vol_ratios){
	//TODO ....
	return list<T>();
}
template list<float> Simulation::allocParticles<float>(unsigned cells,float count,const list<float>* vol_ratios);


vector<unsigned> Simulation::allocAgents1(unsigned cells,unsigned ag_count,const list<float>* vol_ratios){
	vector<unsigned> allocs(cells,0);
	unsigned div = ag_count/cells;
	unsigned r = ag_count%cells;
	for(unsigned i = 0; i < cells; i++)
		allocs[i] = div;
	for(unsigned i = 0; i < r; i++)
		allocs[i] += 1;
	return allocs;
}

// Allocate agents sampling a multinomial distribution.
vector<unsigned> Simulation::allocAgents2(unsigned cells, unsigned ag_counts, const list<float>* vol_ratios) {

	vector<unsigned> allocs;
	list<float> p_values;

	// Create p_values from *vol_ratios.
	if ( vol_ratios == nullptr )
		p_values.assign(cells, 1.0/cells);
	else {
		float sum_vol_ratios = 0.0;
		for (auto v : *vol_ratios)
			sum_vol_ratios += v;
		for (auto v : *vol_ratios)
			p_values.push_back(v/sum_vol_ratios);
	}

	// Sampling multinomial distribution.
	auto distr = uniform_int_distribution<int>();
	boost::mt19937 b_rng(distr(rng));
	unsigned sum_ag = 0;
	float sum_p  = 1.0;
	for (auto p : p_values) {
		boost::binomial_distribution<> distr(ag_counts - sum_ag , p/sum_p);
		boost::variate_generator<boost::mt19937&,
		                         boost::binomial_distribution<> > sampler(b_rng, distr);
		unsigned s = sampler();
		sum_ag += s;
		sum_p  -= p;

		allocs.push_back(s);
	}

	return allocs;
}

/*template <template<typename,typename...> class Range,typename... Args>
void Simulation::addTokens(const Range<int,Args...> &cell_ids,float count,short token_id){
	//list<float> per_cell = allocAgents2(cell_ids.size(),count);
	//auto ids_it = cell_ids.begin();
	for(auto id : cell_ids){
		try{
			cells.at(id)->addTokens(count/cell_ids.size(),token_id);
		}
		catch(out_of_range &e){
			//other mpi_process will add this tokens.
		}
	}
}
template void Simulation::addTokens(const set<int> &cell_ids,float count,short token_id);
*/

template <template<typename,typename...> class Range,typename... Args>
void Simulation::addAgents(const Range<int,Args...> &cell_ids,unsigned count,const pattern::Mixture &mix){
	auto per_cell = allocAgents2(cell_ids.size(),count);
	auto ids_it = cell_ids.begin();
	for(auto n : per_cell){
		if(n == 0)
			continue;
		try{
			cells.at(*ids_it)->initNodes(n,mix);
		}
		catch(out_of_range &e){
			//other mpi_process will add this tokens.
		}
		ids_it++;
	}
}
template void Simulation::addAgents(const set<int> &cell_ids,unsigned count,const pattern::Mixture &mix);


/** \brief Return a way to allocate cells among cpu's.
 *
 * @param n_cpus Number of machines or comm_world.size()
 * @param w_vertex Weights for every cell (total reactivity).
 * @param w_edges Weights for every channel (sum of transport reactivity).
 * @param tol Tolerance in the number of processors to be assigned
 * @return a vector with the compartments indexed by ID processor
 */
vector<list<unsigned int>> Simulation::allocCells( int n_cpus,
		const vector<double> &w_vertex,
		const map<pair<int,int>,double> &w_edges,
		int tol) {

	vector<list<unsigned int>> P (n_cpus); //array of indexed compartments by cpu; P[ core ] = { compartments }
	// initialize P
	for( int i = 0 ; i < n_cpus ; i++ )
		P[i] = list<unsigned int>();

	vector<pair<pair<int,int>,double>> ordered_edges = sortEdgesByWeidht(w_edges);
	vector<double> assigned (n_cpus, 0); // assigned and saved edges
	double totalWeight = 0;

	// initialize totalWeight
	for( auto weight : w_vertex )
		totalWeight += weight;

	// assign cores to compartments
	for( auto edge : ordered_edges ) {
		//cout << "viewing " << edge.first.first << " -> " << edge.first.second << " -> " << edge.second << endl;

		int core1 = searchCompartment(P, edge.first.first);
		int core2 = searchCompartment(P, edge.first.second);
		//cout << "edge " << edge.first.first << "," << edge.first.first << endl ;

		if( core1 == -1 && core2 == -1 ) { // if the compartments edge.first.first & edge.first.second don't has assigned core
			unsigned core = minP(P);
			//cout << "core = " << core << endl;

			if( edge.first.first == edge.first.second ){
				P[core].push_back( edge.first.first );
				//assigned[core] += w_vertex[ edge.first.first ] + w_vertex[ edge.first.second ];
				assigned[core] += w_vertex[ edge.first.first ];
			} else {
				P[core].push_back( edge.first.first );
				P[core].push_back( edge.first.second );
				assigned[core] += w_vertex[ edge.first.first ] + w_vertex[ edge.first.second ];
			}

		} else if( core1 == -1 ) { // if edge.first.first has been assigned
			//cout << "k = " << kSecond << endl;

			if( assigned[core2] < (double)totalWeight / n_cpus + tol ) {
				P[core2].push_back( edge.first.first );
				assigned[core2] += w_vertex[ edge.first.first ];
			}

		} else if( core2 == -1 ) {
			//cout << "k = " << kFirst << endl;

			if( assigned[core1] < (double)totalWeight / n_cpus + tol ) {
				P[core1].push_back( edge.first.second );
				assigned[core1] += w_vertex[ edge.first.second ];
			}

		}
	}
	// post processing
	//find no assigned compartments
	for(size_t i = 0;  i < w_vertex.size() ; i++ ) {
		int core = searchCompartment(P, i);
		if( core == -1 ) {
			core = minP(P);
			P[core].push_back( i );
			assigned[core] += w_vertex[ i ];
		}
	}

	/*
#ifdef DEBUG
	if(Parameters::get().verbose > 0){
		cout << "Vertex Assignment:" << endl;
		for( unsigned i = 0 ; i < P.size() ; i++ ) {
			cout << "P[" << i << "] = { " ;
			for ( auto p : P[i] ) {
				cout << p << ",";
			}
			cout << " }" << endl;
		}
	}
#endif
*/
	return P;
}



/** \brief Sort edges by weight from lowest to highest
 *  @param w_edges edges with weight
 */
vector< pair< pair<int,int>, double > > Simulation::sortEdgesByWeidht( const map<pair<int,int>,double> &w_edges ) {
	// function to sort edges
	struct LocalSort {
		bool operator()( pair<pair<int,int>,float> a, pair<pair<int,int>,float> b ) {
			return a.second < b.second;
		}
	} f;

	vector< pair< pair<int,int>, double> > ordered_edges;

	// save edge
	if( ordered_edges.empty() ) {
		for( auto &edge : w_edges ) {
			ordered_edges.push_back( edge );
		}
	}

	sort( ordered_edges.begin(), ordered_edges.end(), f );
	return ordered_edges;
}


unsigned Simulation::minP( vector<list<unsigned int>> P ) {
	unsigned k = 0;
	for ( unsigned i = 0; i < P.size() ; i++ ) {
		for ( unsigned j = 0 ; j < P.size() ; j++ ) {
			if( P[i].size() < P[j].size() )
				k = i; // save the minus size
		}
	}
	return k;
}

/** \brief Search if a compartment was assigned to a core
 * @param assigned assigned compartment list
 * @param c compartment to search
 */
int Simulation::searchCompartment(vector<list<unsigned int>> assigned, unsigned c ) {
	for( int i = 0 ; i < (int)assigned.size() ; i++ ) {
		for ( auto p : assigned[i] ) {
			if( p == c )
				return i;
		}
	}

	return -1;
}

/*bool Simulation::isDone() const {
	return done;
}*/

data_structs::DataTable* Simulation::getTrajectory() const {
	auto tab = new data_structs::DataTable(plot->getData(),true);
	//tab->col_names.emplace_back("Time");
	for(auto var : env.getObservables())
		tab->col_names.emplace_back(var->toString());
	return tab;
}

data_structs::DataTable* Simulation::getTrajectory(int i) const {
	return cells.at(i)->getTrajectory();
}


void Simulation::collectRawData() const {
	rawList.clear();
	for(auto& pert : perts)
		pert.second.collectRawData(rawList);
}
void Simulation::collectTabs(map<string,list<const data_structs::DataTable*>> &tab_list) const {
	for(auto& pert : perts)
		pert.second.collectTabs(tab_list);
	for(auto& cell : cells){
		//cell.second->collectTabs(tab_list);
		//throw invalid_argument("Simulation::collectTabs(): spatial not implemented yet.");
	}
}


void Simulation::print() const {
	//cout << "========= Simulation[" << id << "] =========" << endl;
	if(cells.size() > 1)
		cout << cells.size() << " disjoint volumes in this simulation object." << endl;
	/*for(auto& cell : cells){
		if(cells.size() > 1)
			cout << "---" << env.cellIdToString(cell->getId()) << "---\n";
		cell->print();
	}*/
	cout << "Active Injections -> {\n";
	int i = 0;
	for(auto& cc : env.getComponents()){
		i = cc->getId();
		if(count(i))
			cout << "\t("<< i <<")\t" << count(i) <<
			" injs of " << cc->toString(env) << endl;
	}
	for(auto& mix : env.getMixtures()){
		i = mix->getId();
		matching::InjRandContainer<matching::MixInjection>* injs = nullptr;
		/*try {
			injs = nlInjections.at(i);
		}
		catch(std::out_of_range& e){ }
		if(injs && injs->count())
			cout << "\t("<< i <<")\t" << injs->count() <<
			" mix-injs of " << mix->toString(env) << endl;*/
	}
	cout << "}\nActive Rules -> {\n";
	for(auto cell : cells){
		cout << env.cellIdToString(cell->getId()) << "\n";
		cout << cell->activeRulesStr();
	}

	cout << "}\n";
	cout << counter.toString() << endl;

}

} /* namespace state */
