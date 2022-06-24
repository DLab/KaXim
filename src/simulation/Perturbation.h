/*
 * Perturbation.h
 *
 *  Created on: May 2, 2019
 *      Author: naxo100
 */

#ifndef SRC_SIMULATION_PERTURBATION_H_
#define SRC_SIMULATION_PERTURBATION_H_

#include "../pattern/mixture/Pattern.h"
#include "Rule.h"
#include "../state/Variable.h"

#include <fstream>


namespace state { class State; }
namespace data_structs { class DataTable; }

namespace simulation {

class Simulation;

using namespace expressions;
using namespace pattern;

class Perturbation {
public:
	class Effect {
	public:
		virtual ~Effect();
		virtual void apply(Simulation &state) const = 0;
		virtual int addInfluences(int current,Rule::CandidateMap& map,
				const SimContext &context,const Environment &env) const;
		virtual string toString(const SimContext& context) const {
			return "Unk. Effect";
		};
	};
	friend class pattern::Environment;
protected:
	int id;
	BaseExpression* condition;
	BaseExpression* until;
	list<Effect*> effects;
	Rule::CandidateMap influence;
	int introCount;
	float nextStop;
	float incStep;

	mutable int applies;
	bool isCopy;

	void setId(int _id);
public:
	Perturbation(BaseExpression* cond,BaseExpression* unt,const yy::location& loc,const simulation::SimContext &context);
	Perturbation(const Perturbation& pert);
	~Perturbation();

	int getId() const;

	bool test(const SimContext& context) const;
	FL_TYPE timeTest(const SimContext& context,FL_TYPE dt = 0.0) const;
	bool testAbort(const SimContext& context,bool just_applied);
	void apply(Simulation &state) const;

	void addEffect(Effect* eff,const simulation::SimContext &context,const Environment& env);

	void collectRawData(map<string,list<const vector<FL_TYPE>*>> &raw_list) const;
	void collectTabs(map<string,list<const data_structs::DataTable*>> &tab_list) const;

	float nextStopTime() const;

	string toString(const SimContext& state) const;
};



class Intro : public Perturbation::Effect {
	const BaseExpression* n;
	const pattern::Mixture* mix;

public:
	Intro(const BaseExpression* n,const pattern::Mixture* mix);
	~Intro();

	void apply(Simulation &state) const override;
	int addInfluences(int current,Rule::CandidateMap& map,const simulation::SimContext &context,
			const Environment &env) const override;
	string toString(const SimContext& context) const override;
};

class Delete : public Perturbation::Effect {
	const BaseExpression* n;
	const pattern::Mixture& mix;

public:
	Delete(const BaseExpression* n,const pattern::Mixture& mix);
	~Delete();

	void apply(Simulation &state) const override;
	string toString(const SimContext& context) const override;
};

class Update : public Perturbation::Effect {
	state::Variable* var;
	bool byValue;

public:
	Update(const state::Variable& _var,expressions::BaseExpression* expr);
	//Update(unsigned var_id,pattern::Mixture* _var);
	~Update();

	void apply(Simulation &state) const override;
	void setValueUpdate();
	string toString(const SimContext& context) const override;
};

class UpdateToken : public Perturbation::Effect {
	unsigned tokId;
	const expressions::BaseExpression* value;

public:
	UpdateToken(unsigned var_id,expressions::BaseExpression* val);
	~UpdateToken();

	void apply(Simulation &state) const override;
	string toString(const SimContext& context) const override;
};

class ApplyRule : public Perturbation::Effect {
	Rule rule;
	expressions::BaseExpression* n;

public:
	ApplyRule(const Rule& r,expressions::BaseExpression* expr);
	~ApplyRule();

	void apply(Simulation &state) const override;
	string toString(const SimContext& context) const override;
};

class Histogram : public Perturbation::Effect {
	friend class Perturbation;
	mutable vector<unsigned> bins;
	mutable vector<FL_TYPE> points;
	mutable float min, max;
	string prefix;//,filetype;
	BaseExpression* func;
	list<const state::KappaVar*> kappaList;
	mutable bool newLim;
	bool fixedLim;
	mutable map<string,list<data_structs::DataTable>> tabs;
	mutable map<string,list<vector<FL_TYPE>>> rawData;

public:
	Histogram(const SimContext& sim,int _bins,string file_name,
			list<const state::KappaVar*> k_vars,BaseExpression* f = nullptr);
	~Histogram();

	void apply(Simulation& state) const override;

	void setPoints() const;
	void tag(FL_TYPE val) const;

	void printHeader(ofstream &file,const string& kappa) const;

	string toString(const SimContext& context) const override;

};


//class DELETE,UPDATE,UPDATE_TOK,STOP,SNAPSHOT,PRINT,PRINTF,CFLOW,CFLOW_OFF,FLUX,FLUX_OFF



} /* namespace simulation */

#endif /* SRC_SIMULATION_PERTURBATION_H_ */
