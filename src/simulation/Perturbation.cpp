/*
 * Perturbation.cpp
 *
 *  Created on: May 2, 2019
 *      Author: naxo100
 */

#include "Perturbation.h"
#include "Parameters.h"
#include "Simulation.h"
#include "../state/State.h"
#include "../state/Node.h"
#include "../pattern/mixture/Agent.h"
#include "../pattern/mixture/Component.h"
#include "../pattern/Environment.h"
#include "../expressions/BinaryOperation.h"
#include "../expressions/NullaryOperation.h"
#include "../expressions/Vars.h"
#include "../util/Warning.h"

#include <limits>
#include <float.h>
#include <boost/filesystem.hpp>


namespace simulation {

Perturbation::Perturbation(BaseExpression* cond,BaseExpression* unt,
		const yy::location& loc,const simulation::SimContext &context) : id(-1),
		condition(cond),until(unt),introCount(0),nextStop(-1.0),incStep(0.0),applies(0),isCopy(false){
#ifdef DEBUG
	if(condition == nullptr)
		throw invalid_argument("Perturbation: condition cannot be null.");
#endif
	if(cond->getVarDeps() & BaseExpression::TIME){
		const BaseExpression *expr1 = nullptr,*expr2 = nullptr,*expr3 = nullptr;
		char op1,op2;
		auto ff_expr = dynamic_cast<BinaryOperation<bool,FL_TYPE,FL_TYPE>*>(cond);//opt1 [T] = x ?
		if(ff_expr){
			expr1 = ff_expr->exp1;
			expr2 = ff_expr->exp2;
			op1 = ff_expr->op;
		}
		else{
			auto fi_expr = dynamic_cast<BinaryOperation<bool,FL_TYPE,int>*>(cond);
			if(fi_expr){
				expr1 = fi_expr->exp1;
				expr2 = fi_expr->exp2;
				op1 = fi_expr->op;
			}
		}
		if(expr1 && op1 == BaseExpression::EQUAL){
			float val1 = expr2->getValueSafe(context).valueAs<FL_TYPE>();
			auto time_expr = dynamic_cast<const NullaryOperation<FL_TYPE>*>(expr1);
			if(time_expr && (time_expr->getVarDeps() & BaseExpression::TIME) && op1 != BaseExpression::EQUAL ){
				nextStop = val1;
				return;
			}
			else{
				auto mod_expr = dynamic_cast<const BinaryOperation<FL_TYPE,FL_TYPE,FL_TYPE>*>(expr1);
				if(mod_expr && dynamic_cast<const NullaryOperation<FL_TYPE>*>(mod_expr->exp1)){
					expr3 = mod_expr->exp2;
					op2 = mod_expr->op;
				}
				else {
					auto mod_expr_int = dynamic_cast<const BinaryOperation<FL_TYPE,FL_TYPE,int>*>(expr1);
					if(mod_expr_int && dynamic_cast<const NullaryOperation<FL_TYPE>*>(mod_expr_int->exp1)){
						expr3 = mod_expr_int->exp2;
						op2 = mod_expr_int->op;
					}
				}
				if(expr3 && op2 == BaseExpression::MODULO){//option ([t] % expr3) = expr2
					auto val2 = expr3->getValueSafe(context).valueAs<FL_TYPE>();
					if(val1 < 0)
						throw SemanticError("Comparing time with negative values will generate unexpected behavior.",loc);
					nextStop = std::signbit(val1) ? val1 : val2;
					incStep = val2;
					return;
				}
			}
		}
		ADD_WARN("Optimized time-perturbations should be expressed like [T] = x or ([T] % x) = n",loc);
	}
}

Perturbation::Perturbation(const Perturbation& p) : id(p.id),condition(p.condition),
		until(p.until),influence(p.influence),introCount(0),
		nextStop(p.nextStop),incStep(p.incStep),applies(0),isCopy(true) {
	//cloning state-dependent effects
	for(auto eff : p.effects){
		auto hist = dynamic_cast<Histogram*>(eff);
		if(hist)
			effects.push_back(new Histogram(*hist));
		else
			effects.push_back(eff);
	}
}

Perturbation::~Perturbation() {
	if(!isCopy){
		delete condition;
		delete until;
		for(auto eff : effects)
			delete eff;
	}
	else
		for(auto eff : effects)
			if(dynamic_cast<Histogram*>(eff))
				delete eff;
}

void Perturbation::setId(int _id){
	id = _id;
}

int Perturbation::getId() const {
	return id;
}

bool Perturbation::test(const SimContext& context) const {
	return condition->getValue(context).valueAs<bool>();
}

FL_TYPE Perturbation::timeTest(const SimContext& context) const {
	static const FL_TYPE min_inc = numeric_limits<FL_TYPE>::epsilon();
	if(nextStop >= 0)
		return nextStop > context.getCounter().getTime() ? 0.0 : nextStop+min_inc;
	else
		return condition->getValue(context).valueAs<bool>() ? -1.0 : 0.0;
}

void Perturbation::apply(state::State& state) const {
#ifdef DEBUG
	if(Parameters::get().verbose > 0)
		cout << "Applying perturbation " << id << " at time " << state.getCounter().getTime() << endl;
#endif
	for(auto eff : effects)
		eff->apply(state);
	state.positiveUpdate(influence);
	applies++;
}

bool Perturbation::testAbort(const SimContext& context,bool just_applied){
	bool abort = false;
	if(until){
		abort = until->getValue(context).valueAs<bool>();
	}
	else{
		abort = just_applied;
	}
	while(incStep && nextStop <= context.getCounter().getTime())
		nextStop += incStep;
#ifdef DEBUG
	if(abort && !applies)
		ADD_WARN_NOLOC("Perturbation "+to_string(id)+" was aborted at time "+
				to_string(context.getCounter().getTime())+" and never applied.");
#endif
	return abort;
}

void Perturbation::addEffect(Effect* eff,
		const simulation::SimContext &context,const Environment &env) {
	effects.push_back(eff);
	introCount += eff->addInfluences(introCount,influence,context,env);
}

float Perturbation::nextStopTime() const {
	return nextStop;
}

string Perturbation::toString(const state::State& state) const {
	string ret;
	if(until)
		ret += "While "+ until->toString()+" ";
	ret += "Apply effects: -";
	ret += " whenever "+condition->toString();
	return ret;
}

Perturbation::Effect::~Effect(){}

int Perturbation::Effect::addInfluences(int current,Rule::CandidateMap& map,
		const SimContext &context,const Environment& env) const {
	return 0;
}

/**************** Effects *******************/
/****** Intro****************************/
Intro::Intro(const BaseExpression* n, const Mixture* mix ) : n(n),mix(mix) {}

Intro::~Intro(){
	delete n;
	delete mix;
}

void Intro::apply(state::State& state) const {
	auto nn = n->getValue(state).valueAs<int>();
	if(nn < 0)
		throw invalid_argument("A perturbation is trying to add a negative number of Agents.");
	state.addNodes(nn, *mix);//no neg update
}


int Intro::addInfluences(int current,Rule::CandidateMap& map,
		const SimContext &context,const Environment &env) const {
	//expressions::AuxMap aux_map;
	for(unsigned ag_pos = 0; ag_pos < mix->size(); ag_pos++){
		auto& new_ag = mix->getAgent(ag_pos);
		auto ag_coords = mix->getAgentCoords(ag_pos);
		for(auto ag_ptrn : env.getAgentPatterns(new_ag.getId()))
			for(auto cc_root : ag_ptrn->getIncludes()){
				Rule::CandidateKey key{cc_root.first,{cc_root.second,ag_pos}};
				Rule::CandidateInfo info{{0,ag_pos},true};
				if(cc_root.first->testEmbed(mix->getComponent(ag_coords.first),
						key.match_root,context))
					map.emplace(key,info);
			}
	}
	return mix->size();
}

/****** Delete *************************/
Delete::Delete(const BaseExpression* n, const Mixture& mix ) : n(n),mix(mix) {
	if(mix.compsCount() != 1)
		throw invalid_argument("Perturbations can only delete unary agent patterns.");
}

Delete::~Delete(){
	delete n;
}

void Delete::apply(state::State& state) const {
	auto& inj_cont = state.getInjContainer(mix.getId());
	auto total = inj_cont.count();
	auto some_del = n->getValue(state);
	size_t del;
	if(some_del.t == Type::FLOAT){
		if(some_del.fVal == numeric_limits<FL_TYPE>::infinity())
			del = numeric_limits<size_t>::max();
		else{
			del = round(some_del.fVal);
			ADD_WARN_NOLOC("Making approximation of a float value in agent initialization to "+to_string(del));
		}
	}
	else
		del = some_del.valueAs<int>();

	if(some_del.valueAs<FL_TYPE>() < 0)
		throw invalid_argument("A perturbation is trying to delete a negative number ("+
			to_string(del)+") of instances of "+mix.getComponent(0).toString(state.getEnv()));
	if(total <= del){
		inj_cont.clear();
		if(total < del && del != numeric_limits<size_t>::max())
			ADD_WARN_NOLOC("Trying to delete "+to_string(del)+" instances of "+
				mix.getComponent(0).toString(state.getEnv())+" but there are "+
				to_string(total)+" available. Deleting all.");
		return;
	}

	auto distr = uniform_int_distribution<unsigned>();
	for(size_t i = 0; i < del; i++){
		auto j = distr(state.getRandomGenerator());
		auto& inj = inj_cont.choose(j);
		for(auto node : inj.getEmbedding()){
			node->removeFrom(state);
		}
	}
}

/***** Update **************************/


Update::Update(const state::Variable& _var,expressions::BaseExpression* expr) : var(nullptr),byValue(false){
	var = state::Variable::makeAlgVar(_var.getId(), _var.toString(), expr);
}
Update::~Update(){
	delete var;
}

void Update::apply(state::State &state) const {
	//cout << "updating var " << var->toString() << " to value " << var->getValue(state) << endl;
	state.updateVar(*var,byValue);
}

void Update::setValueUpdate() {
	byValue= true;
}


/***** Histogram **************************/

Histogram::Histogram(const Environment& env,int _bins,string _prefix,
			list<const state::KappaVar*> k_vars,BaseExpression* f) :
		bins(_bins+2),points(_bins+1),min(-numeric_limits<float>::infinity()),max(-min),
		prefix(_prefix),func(f),kappaList(k_vars),newLim(false),fixedLim(false){
	const map<string,pattern::Mixture::AuxCoord>* p_auxs(nullptr);
	for(auto kappa : kappaList){
		auto& mix = kappa->getMix();
		if(mix.compsCount() != 1)
			throw invalid_argument("Mixture must contain exactly one Connected Component.");
		auto& cc = mix.getComponent(0);
		auto& auxs = mix.getAuxMap();
		if(auxs.size() < 1)
			throw invalid_argument("Mixture must have at least one Aux. variable.");
		if(!func){
			if(auxs.size() > 1)
				throw invalid_argument("Mixture must have exactly one Aux. variable "
						"if you don't declare a value function for histogram.");
			auto& aux_coord = *(auxs.begin());
			func = new expressions::Auxiliar<FL_TYPE>(aux_coord.first,aux_coord.second);
		}
		if(p_auxs){
			for(auto& aux : auxs)
				if(p_auxs->count(aux.first) && !(p_auxs->at(aux.first) == aux.second))
					ADD_WARN_NOLOC("Every declared auxiliary in Histogram's variables should match the same Agent's sites.");
		}
		else{
			p_auxs = &auxs;
			if(dynamic_cast<expressions::Auxiliar<FL_TYPE>*>(func)){
				pattern::Mixture::AuxCoord coord;
				try{
					coord = p_auxs->at(func->toString());
				}
				catch(std::out_of_range &e){
					throw invalid_argument
						("Auxiliaries in Histogram function have to match with the auxiliaries in every KappaVar.");
				}
				auto lims = env.getSignature(cc.getAgent(coord.ag_pos).getId()).getSite(coord.st_id).getLimits();
				min = lims.first.valueAs<FL_TYPE>();
				max = lims.second.valueAs<FL_TYPE>();
			}
		}
	}


	auto& folder = simulation::Parameters::get().outputDirectory;
	auto pos = _prefix.rfind('/');
	if(pos != string::npos){
		boost::filesystem::path p(folder + "/" + _prefix.substr(0,pos));
		if(!boost::filesystem::exists(p))
			create_directories(p);
		else
			if(!is_directory(p))
				throw invalid_argument("Cannot create folder " + p.string() +
						": another file with the same name exists.");
	}
/*
	pos = file_name.rfind('.');
	if( pos == string::npos )
		filename = file_name;
	else{
		filename = folder + "/" + file_name.substr(0,pos);
		filetype = file_name.substr(pos+1);
	}
*/

	if(min != max && !isinf(max) && !isinf(min)){
		fixedLim = true;
		newLim = true;
		setPoints();
	}
}

Histogram::~Histogram(){
	//TODO
	//file.close();
}


void Histogram::apply(state::State& state) const {
	char file_name[180];
	FL_TYPE sum(0.0);
	auto& params = simulation::Parameters::get();

	if(Parameters::get().runs > 1)
		sprintf(file_name,("%s/%s(%s-%0"+to_string(int(log10(params.runs-1)+1))+"d).%s").c_str(),
				params.outputDirectory.c_str(),prefix.c_str(),params.outputFile.c_str(),
				state.getParentContext().getId(),params.outputFileType.c_str());
	else
		sprintf(file_name,"%s/%s(%s).%s",params.outputDirectory.c_str(),prefix.c_str(),
				params.outputFile.c_str(),params.outputFileType.c_str());
	ofstream file(file_name,ios_base::app);
	if(file.fail()){
		ADD_WARN_NOLOC("Cannot open the file "+string(file_name)+" to print the histogram.");
		return;
	}

	if(newLim){
		printHeader(file);
		setPoints();
		newLim = false;
	}

	CcEmbMap<expressions::Auxiliar,FL_TYPE,state::Node> cc_map;
	state.setAuxMap(&cc_map);
	for(auto kappa : kappaList){
		auto& mix = kappa->getMix();
		auto& cc = mix.getComponent(0);

		if(state.getInjContainer(cc.getId()).count() == 0){
			file << kappa->toString() << "\t" << state.getCounter().getTime() << "\tNo values\n";
			continue;
		}

		sum = 0;
		if(min == max || isinf(min) || isinf(max)){
			list<FL_TYPE> values;
			max = -numeric_limits<float>::infinity();
			min = -max;
			function<void (const matching::Injection*)> f = [&](const matching::Injection* inj) -> void {
				cc_map.setEmb(inj->getEmbedding());
				auto val = func->getValue(state).valueAs<FL_TYPE>();
				if(val < min)
					min = val;
				if(val > max)
					max = val;
				values.push_back(val);
				sum += val;
			};
			state.getInjContainer(cc.getId()).fold(f);
			bins.assign(bins.size(),0);
			setPoints();
			printHeader(file);
			newLim = false;
			if(min != max)
				for(auto v : values)
					tag(v);
		}
		else{
			function<void (const matching::Injection*)> f = [&](const matching::Injection* inj) -> void {
				cc_map.setEmb(inj->getEmbedding());
				auto val = func->getValue(state).valueAs<FL_TYPE>();
				if(!fixedLim && !isinf(val)){
					if(val < min){
						min = val;newLim = true;
					}
					if(val > max){
						max = val;newLim = true;
					}
				}
				tag(val);
				sum += val;
			};
			bins.assign(bins.size(),0);
			state.getInjContainer(cc.getId()).fold(f);
		}
		file << kappa->toString() << "\t";
		file << state.getCounter().getTime() << "\t";
		if(min == max)
			file << state.getInjContainer(cc.getId()).count() << "\t";
		else
			for(auto bin : bins)
				file << bin << "\t";
		file << sum / state.getInjContainer(cc.getId()).count() << "\n";
	}
	if(kappaList.size() > 1)
		file << endl;
	file.close();
}

void Histogram::setPoints() const {
	auto dif = (max - min) / (bins.size()-2);
	for(unsigned i = 0; i < points.size(); i++){
		points[i] = min + i*dif;
	}
}

void Histogram::tag(FL_TYPE val) const {
	if(val < points[0]){
		if(fixedLim)
			ADD_WARN_NOLOC("The value for a site in an agent instance is bellow the minimum allowed.");
		bins[0]++;
		return;
	}
	for(unsigned i = 1; i < points.size()-1; i++)
		if(val < points[i]){
			bins[i]++;return;
		}
	if(val <= points.back())
		bins[points.size()-1]++;
	else{
		if(fixedLim)
			ADD_WARN_NOLOC("The value for a site in an agent instance is over the maximum allowed");
		bins[points.size()]++;
	}
}

void Histogram::printHeader(ofstream& file) const {
	char s1[100];

	if(isinf(min) || isinf(max)){
		sprintf(s1,"\ttime\t[%.5g,%.5g]\tAverage",min,max);
		file << s1 << "\n";
	}
	else if(min == max){
		sprintf(s1,"\ttime\t%.5g\tAverage",min);
		file << s1 << "\n";
	}
	else{
		auto dif = (max - min) / (bins.size()-2);
		sprintf(s1,"\ttime\tx < %.5g",min);
		file << s1;
		for(unsigned i = 0; i < bins.size()-2; i++){
			if(i)
				file << ")";
			sprintf(s1,"\t[%.5g,%.5g",min+dif*i,min+dif*(i+1));
			file << s1;
		}
		sprintf(s1,"]\t%.5g < x\tAverage\n",max);
		file << s1 ;
	}
	newLim = false;
}



} /* namespace simulation */
