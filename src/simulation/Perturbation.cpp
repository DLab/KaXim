/*
 * Perturbation.cpp
 *
 *  Created on: May 2, 2019
 *      Author: naxo100
 */

#include "Perturbation.h"
#include "Parameters.h"
#include "Simulation.h"
#include "../data_structs/DataTable.h"
#include "../state/State.h"
#include "../state/Node.h"
#include "../pattern/mixture/Agent.h"
#include "../pattern/mixture/Component.h"
#include "../pattern/Environment.h"
#include "../expressions/BinaryOperation.h"
#include "../expressions/NullaryOperation.h"
#include "../expressions/Vars.h"
#include "../util/Warning.h"
#include "../pattern/Dependencies.h"

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
		//cout << " a time dependant pertubation" << endl;
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
		if(expr1 && (op1 == BaseExpression::EQUAL || op1 == BaseExpression::GREATER)){
			float val1 = expr2->getValueSafe(context).valueAs<FL_TYPE>();
			auto time_expr = dynamic_cast<const NullaryOperation<FL_TYPE>*>(expr1);
			if(time_expr && (time_expr->getVarDeps() & BaseExpression::TIME) ){
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
					nextStop = std::signbit(val1) ? val1 : val2+val1;
					incStep = val2;
					return;
				}
			}
		}
		ADD_WARN("Optimized time-perturbations should be expressed like [T] = x or ([T] % x) = n",loc);
	}
	else {
		//cout << "not a time dependant pertubation" << endl;
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

FL_TYPE Perturbation::timeTest(const SimContext& context,FL_TYPE f) const {
	static const FL_TYPE min_inc = numeric_limits<FL_TYPE>::epsilon();
	if(nextStop >= 0)
		return nextStop > context.getCounter().getTime()+f ? 0.0 : nextStop+min_inc;
	else
		return condition->getValue(context).valueAs<bool>() ? -1.0 : 0.0;
}

void Perturbation::apply(Simulation& state) const {
	auto params = state.params;
	IF_DEBUG_LVL(2,state.log_msg += "|| Applying perturbation ["
			+ to_string(id) + "] at time " + to_string(state.getCounter().getTime()) + " ||\n");
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


void Perturbation::collectRawData(map<string,list<const vector<FL_TYPE>*>> &raw_list) const {
	for(auto effect : effects){
		auto hist = dynamic_cast<Histogram*>(effect);
		if(hist)
			for(auto& raws : hist->rawData){
				auto& l = raw_list[raws.first];
				for(auto& raw : raws.second)
					l.emplace_back(&raw);
			}
	}
}
void Perturbation::collectTabs(map<string,list<const data_structs::DataTable*>> &tab_list) const {
	for(auto effect : effects){
		auto hist = dynamic_cast<Histogram*>(effect);
		if(hist)
			for(auto& tabs : hist->tabs){
				auto& l = tab_list[tabs.first];
				for(auto& tab : tabs.second)
					l.emplace_back(&tab);
			}
	}
}


string Perturbation::toString(const SimContext& state) const {
	string ret;
	if(until)
		ret += "As long as {"+ until->toString()+" remains False} And ";
	ret += "If {"+condition->toString()+"} condition triggers, apply effects:\n";
	for(auto effect : effects)
		ret += "\t"+effect->toString(state) +"\n";
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

void Intro::apply(Simulation& state) const {
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

string Intro::toString(const SimContext& context) const {
	string ret("Create "+n->toString());
	return ret + " new instances of "+mix->toString(context.getEnv());
}

/****** Delete *************************/
Delete::Delete(const BaseExpression* n, const Mixture& mix ) : n(n),mix(mix) {
	if(mix.compsCount() != 1)
		throw invalid_argument("Perturbations can only delete unary agent patterns.");
}

Delete::~Delete(){
	delete n;
}

void Delete::apply(Simulation& state) const {
	//auto& inj_cont = state.getInjContainer(mix.getId());
	auto total = state.count(mix.getId());
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
		state.clearInstances(mix);
		if(total < del && del != numeric_limits<size_t>::max())
			ADD_WARN_NOLOC("Trying to delete "+to_string(del)+" instances of "+
				mix.getComponent(0).toString(state.getEnv())+" but there are "+
				to_string(total)+" available. Deleting all.");
		return;
	}

	state.removeInstances(del,mix);


}

string Delete::toString(const SimContext& context) const {
	string ret("Delete "+n->toString());
	return ret + " instances of "+mix.toString(context.getEnv());
}

/***** Update **************************/

Update::Update(const state::Variable& _var,expressions::BaseExpression* expr) : var(nullptr),byValue(false){
	var = state::Variable::makeAlgVar(_var.getId(), _var.toString(), expr);
}
Update::~Update(){
	delete var;
}

void Update::apply(Simulation &state) const {
	//cout << "updating var " << var->toString() << " to value " << var->getValue(state) << endl;
	state.updateVar(*var,byValue);
	state.updateDeps(pattern::Dependency(pattern::Dependency::VAR,var->getId()));
}

void Update::setValueUpdate() {
	byValue= true;
}
string Update::toString(const SimContext& context) const {
	string ret("Update variable "+var->toString());
	return ret + " to value "+var->getValue(context).toString();
}


/***** Update Token ***********************/

UpdateToken::UpdateToken(unsigned tok_id,expressions::BaseExpression* val) :
		tokId(tok_id),value(val)  {}
UpdateToken::~UpdateToken(){
	delete value;
}
void UpdateToken::apply(Simulation &state) const {
	state.setTokenValue(value->getValue(state).valueAs<FL_TYPE>(),tokId);
	state.updateDeps(pattern::Dependency(pattern::Dependency::Dep::TOK,tokId));
}

string UpdateToken::toString(const SimContext& context) const {
	string ret("Update token "+context.getEnv().getTokenName(tokId));
	return ret + " to value "+value->toString();
}


/***** Histogram **************************/

Histogram::Histogram(const SimContext& sim,int _bins,string _prefix,
			list<const state::KappaVar*> k_vars,BaseExpression* f) :
		bins(_bins+2),points(_bins+1),min(-numeric_limits<float>::infinity()),max(-min),
		prefix(_prefix),func(f),kappaList(k_vars),newLim(false),fixedLim(false){
	const map<string,pattern::Mixture::AuxCoord>* p_auxs(nullptr);
	auto& env = sim.getEnv();
	for(auto kappa : kappaList){
		auto& mix = kappa->getMix();
		tabs.emplace(kappa->toString(),0);
		rawData.emplace(kappa->toString(),0);
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

	if(_bins && min != max && !isinf(max) && !isinf(min)){
		fixedLim = true;
		newLim = true;
		setPoints();
	}
	auto& params = *sim.params;
	if(params.outputFile == "" && params.outputFileType.find("data") != string::npos){
		if(prefix != "")
			ADD_WARN_NOLOC("Histograms will not be printed to files. Use returned results.");
		prefix = "";
		return;
	}
	auto& folder = params.outputDirectory;
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
}

Histogram::~Histogram(){
	//TODO
	//file.close();
}


void Histogram::apply(Simulation& context) const {
	char file_name[2000];
	FL_TYPE sum(0.0);
	int comp_id = 0;
	auto& state = context.getCell(comp_id);//TODO for every cell
	auto& params = *state.params;

	ofstream file;
	if(prefix != ""){
		if(params.runs > 1)
			sprintf(file_name,("%s/%s(%s-%0"+to_string(int(log10(params.runs-1)+1))+"d).%s").c_str(),
					params.outputDirectory.c_str(),prefix.c_str(),params.outputFile.c_str(),
					state.getParentContext().getId(),params.outputFileType.c_str());
		else
			sprintf(file_name,"%s/%s(%s).%s",params.outputDirectory.c_str(),prefix.c_str(),
					params.outputFile.c_str(),params.outputFileType.c_str());
		if(strlen(file_name) > 1999)
			throw invalid_argument("Histogram file name is too long!");
		file.open(file_name,ios_base::app);
		if(file.fail()){
			ADD_WARN_NOLOC("Cannot open the file "+string(file_name)+" to print the histogram.");
			return;
		}
	}

	if(newLim){
		if(kappaList.size() > 1)
			throw invalid_argument("FIX HERE!!! perturbation, apply!");
		printHeader(file,kappaList.front()->toString());
		setPoints();
		newLim = false;
	}

	CcEmbMap<expressions::Auxiliar,FL_TYPE,state::Node> cc_map;
	state.setAuxMap(&cc_map);
	for(auto kappa : kappaList){
		auto& mix = kappa->getMix();
		auto& cc = mix.getComponent(0);
		auto count = state.getInjContainer(cc.getId()).count();
		auto& injs = state.getInjContainer(cc.getId());
		state.setAuxMap(&cc_map);//Fixing bug with lazy init
		rawData.at(kappa->toString()).emplace_back(count+1);
		auto& raw = rawData.at(kappa->toString()).back();
		auto time = state.getCounter().getTime();
		raw[0] = time;
		if(count == 0){
			/*auto& dt = tabs.at(kappa->toString()).back();
			file << kappa->toString() << "\t" << time << "\tNo values\n";
			dt.data.conservativeResize(dt.data.rows()+1,Eigen::NoChange);
			dt.row_names.emplace_back(to_string(time));
			//cout << "1) dt.data(" << dt.data.rows()-1 << "," << 0 << "= nan" << endl;
			dt.data(dt.data.rows()-1,0) = nan("");
			*/continue;
		}

		sum = 0;
		int i = 0;
		int col = 0;
		if(bins.size() == 2){//no-bins | print only once at end-sim
			map<FL_TYPE,int> values;
			list<string> col_names;
			function<void (const matching::Injection*)> f = [&](const matching::Injection* inj) -> void {
				cc_map.setEmb(*inj->getEmbedding());
				auto val = func->getValue(state).valueAs<FL_TYPE>();
				sum += val;
				values[val]++;
				raw[++i] = val;
			};
			injs.fold(f);
			file << "\ttime";
			for(auto val : values){
				int vi(val.first);
				string vs(val.first == vi ? to_string(vi) : to_string(val.first));
				file << "\t" << vs;
				col_names.emplace_back(vs);
			}
			col_names.emplace_back("Average");
			tabs.at(kappa->toString()).emplace_back(list<string>({to_string(time)}),col_names);
			auto& dt = tabs.at(kappa->toString()).back();
			file << "\tAverage\n" << kappa->toString() << "\t" << time;
			for(auto val : values){
				file << "\t" << val.second;
				//cout << "2) dt.data(" << dt.data.rows()-1 << "," << col << ") = " << val.second << endl;
				dt.data(dt.data.rows()-1,col++) = val.second;
			}
			auto avg = sum / injs.count();
			file << "\t" << avg << endl;
			//cout << "3) dt.data(" << dt.data.rows()-1 << "," << col << ") = " << avg << endl;
			dt.data(dt.data.rows()-1,col) = avg;
			continue;
		}

		if(min == max || isinf(min) || isinf(max)){
			list<FL_TYPE> values;
			max = -numeric_limits<float>::infinity();
			min = -max;
			function<void (const matching::Injection*)> f = [&](const matching::Injection* inj) -> void {
				cc_map.setEmb(*inj->getEmbedding());
				auto val = func->getValue(state).valueAs<FL_TYPE>();
				if(val < min)
					min = val;
				if(val > max)
					max = val;
				values.push_back(val);
				sum += val;
				raw[++i] = val;
			};
			injs.fold(f);
			bins.assign(bins.size(),0);
			setPoints();
			printHeader(file,kappa->toString());
			newLim = false;
			if(min != max)
				for(auto v : values){
					tag(v);
				}
		}
		else{
			function<void (const matching::Injection*)> f = [&](const matching::Injection* inj) -> void {
				cc_map.setEmb(*inj->getEmbedding());
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
				raw[++i] = val;
			};
			bins.assign(bins.size(),0);
			injs.fold(f);
		}
		file << kappa->toString() << "\t";
		file << state.getCounter().getTime() << "\t";
		auto& dt = tabs.at(kappa->toString()).back();
		dt.data.conservativeResize(dt.data.rows()+1,Eigen::NoChange);
		dt.row_names.emplace_back(to_string(time));
		if(min == max){
			file << injs.count() << "\t";
			//cout << "4) dt.data(" << dt.data.rows()-1 << "," << 0 << ") = " << injs.count() << endl;
			dt.data(dt.data.rows()-1,0) = injs.count();
		}
		else
			for(auto bin : bins) {
				file << bin << "\t";
				//cout << "5) dt.data(" << dt.data.rows()-1 << "," << col << ") = " << bin << endl;
				dt.data(dt.data.rows()-1,col++) = bin;
			}
		file << sum / injs.count() << "\n";
		//cout << "6) dt.data(" << dt.data.rows()-1 << "," << col << ") = " << sum/injs.count() << endl;
		dt.data(dt.data.rows()-1,col) = sum / injs.count();
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

void Histogram::printHeader(ofstream& file,const string& kappa) const {
	char s1[100];
	list<string> col_names;
	file << "\ttime\t";
	if(isinf(min) || isinf(max)){
		sprintf(s1,"[%.5g,%.5g]",min,max);
		file << s1;
		col_names.emplace_back(s1);
	}
	else if(min == max){
		sprintf(s1,"%.5g",min);
		file << s1;
		col_names.emplace_back(s1);
	}
	else{
		auto dif = (max - min) / (bins.size()-2);
		sprintf(s1,"x < %.5g",min);
		file << s1;
		col_names.emplace_back(s1);
		for(unsigned i = 0; i < bins.size()-2; i++){
			sprintf(s1,"[%.5g,%.5g%s",min+dif*i,min+dif*(i+1),i == bins.size()-3 ? "]" : ")");
			file << "\t" << s1;
			col_names.emplace_back(s1);
		}
		sprintf(s1,"%.5g < x",max);
		file << "\t" << s1 ;
		col_names.emplace_back(s1);
	}
	file << "\tAverage\n";
	col_names.emplace_back("Average");
	tabs.at(kappa).emplace_back(list<string>(),col_names);
	/*cout << "COL_NAMES:" << endl;
	for(auto n : col_names)
		cout << n << ", ";
	cout << endl;*/
	newLim = false;
}

string Histogram::toString(const SimContext& context) const {
	string ret("Histogram of { ");
	for(auto kappa : kappaList)
		ret += kappa->toString()+", ";
	return ret + "}";
}



} /* namespace simulation */
