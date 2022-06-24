/*
 * Environment.h
 *
 *  Created on: Mar 23, 2016
 *      Author: naxo
 */

#ifndef PATTERN_ENVIRONMENT_H_
#define PATTERN_ENVIRONMENT_H_

#include <map>
#include <unordered_map>
#include <vector>
#include <string>
#include "Signature.h"
#include "mixture/Mixture.h"
#include "Dependencies.h"
#include "../simulation/Rule.h"
#include "../spatial/Channel.h"
#include "../spatial/Compartment.h"
#include "../spatial/Transport.h"
//#include "../state/AlgExpression.h"
//#include "../grammar/ast/Basics.h"
#include "../util/Exceptions.h"

namespace grammar {
namespace ast {
class Id;
class KappaAst;
}
}

namespace expressions {
template <typename T> class Auxiliar;
}

namespace simulation {
class Perturbation;
}

namespace pattern {

using namespace std;
using namespace spatial;

using AstId = grammar::ast::Id;

typedef unsigned short short_id;

/** \brief All the mappings for names and ids needed by the simulation.
 *
 * Manipulate and store all the mappings between names and ids of
 * elements declared with kappa. Global elements are stored first
 * to broadcast the global environment among MPI processes.
 */
class Environment {
	friend class grammar::ast::KappaAst;
protected:
	typedef unordered_map<string,short> IdMap;
	//static Environment env;
	//IdMap algMap, kappaMap;
	IdMap varMap, compartmentMap, channelMap, signatureMap;
	unordered_map<string,unsigned> tokenMap;
	map<string,const expressions::BaseExpression*> paramVars;
	//vector<string> algNames, kappaNames;
	vector<string> varNames, tokenNames, compartmentNames;
	vector<Signature> signatures;
	vector<Compartment> compartments;
	vector<list<Channel> > channels;
	//vector<Transport> transports;
	vector<UseExpression> useExpressions;
	vector<Mixture*> mixtures;
	vector<Mixture::Component*> components;
	vector<list<Mixture::Agent*> > agentPatterns;
	vector<simulation::Rule*> rules;
	map<int,list<two<int>>> unaryCC;			///> cc_id[0] -> [radius,cc_id[1]]
	map<two<int>,list<two<int>>> unaryMix;		///> (cc1,cc2) -> [radius,mix_id]
	int max_radius;
	vector<simulation::Perturbation*> perts;
	list<state::Variable*> observables;
	multimap<two<int>,pair<const Pattern::Component&,small_id>> auxSiteDeps;
	map<string,tuple<Mixture*,small_id,small_id>> auxTemp;
	map<const Mixture*,list<expressions::Auxiliar<FL_TYPE>*>> auxExpressions;
	mutable Dependencies deps;//mutable because [] accessing
	unsigned cellCount;

	struct Init {
		int use_id;
		expressions::BaseExpression* n;
		Mixture* mix;
		int tok_id;

		Init(int id, expressions::BaseExpression* expr,
				Mixture* m, int tok) : use_id(id),n(expr),mix(m),tok_id(tok) {}
		~Init(){
			delete n;delete mix;
		}
	};
	list<Init> inits;

	//(ag_id,site) -> list { (cc,ag_id) } map to every freeSite for side-effect events?
	unordered_map<int,list<pair<const Pattern::Component*,small_id> > > freeSiteCC;

	bool exists(const string &name,const IdMap &map);
public:
	//inline static Environment& getInstance() {return env;}
	Environment();
	~Environment();

	//environment is unique, cannot be copied
	Environment(const Environment& env) = delete;
	Environment& operator=(const Environment& env) = delete;

	unsigned declareToken(const AstId &name);
	short declareParam(const AstId& name,const expressions::BaseExpression* value);

	/** \brief Declares a new variable name and returns its id.
	 *
	 * @returns the id associated to the new name or -1 if a
	 * model param was declared with the same name. Throws if the
	 * name was already used. */
	short declareVariable(const AstId &name,bool isKappa);
	Signature& declareSignature(const AstId& sign);
	Compartment& declareCompartment(const AstId& comp);
	UseExpression& declareUseExpression(unsigned short id,size_t n);
	Channel& declareChannel(const AstId &channel);
	void declareMixture(Mixture* &m);
	map<small_id,small_id> declareComponent(Pattern::Component* &c);

	/** \brief Declare an Agent as a kappa-pattern
	 *
	 * The agent-patterns in the environment are unique. If an equal
	 * pattern is found, new_agent will be deleted and the reference
	 * will be changed to point to the existing agent.
	 * @param new_agent a reference to the new Agent pointer.		 */
	void declareAgentPattern(Mixture::Agent* &new_agent);
	//simulation::Rule& declareRule(const grammar::ast::Id &name,Mixture& mix,
	//		const yy::location& loc);
	void declareRule(simulation::Rule* rule,bool force = false);
	void declareUnaryMix(Mixture& mix,int radius);
	void declarePert(simulation::Perturbation* pert);

	void declareMixInit(int use_id,expressions::BaseExpression* n,Mixture* mix);
	void declareTokInit(int use_id,expressions::BaseExpression* n,int tok_id);

	void declareObservable(state::Variable* obs);

	void addAuxSiteInfl(int ag_id, int st_id,small_id ag_pos,const Pattern::Component& cc){
		auxSiteDeps.emplace(make_pair(ag_id,st_id),pair<const Pattern::Component&,small_id>(cc,ag_pos));
	}
	auto getAuxSiteInfl(two<int> ag_st) const {
		return auxSiteDeps.equal_range(ag_st);
	}
	void buildInfluenceMap(const simulation::SimContext &context);
	void buildFreeSiteCC();
	const list<pair<const Mixture::Component*,small_id> >& getFreeSiteCC(small_id ag,small_id site) const;

	//template <typename T>
	//const T& getSafe(grammar::ast::Id& name) const;

	const Signature& getSignature(short id) const;
	const vector<Signature>& getSignatures() const;
	const Channel& getChannel(short id) const;
	const Compartment& getCompartment(short id) const;
	const UseExpression& getUseExpression(short id) const;
	//const Mixture& getMixture(small_id id) const;
	const vector<Mixture*>& getMixtures() const;
	const vector<Mixture::Component*>& getComponents() const;
	const list<Mixture::Agent*>& getAgentPatterns(small_id id) const;
	const vector<simulation::Rule*>& getRules() const {
		return rules;
	}
	const simulation::Rule& getRule(int id) const {
		return *rules[id];
	}
	/** \brief The list of unary CCs, the radius and the CC pair.
	 *
	 * @returns a map of [unary-cc1-id] -> { (radius,cc2-id) }  */
	auto& getUnaryCCs() const{
		return unaryCC;
	}
	auto& getUnaryMixtures() const {
		return unaryMix;
	}
	int getMaxRadius() const {
		return max_radius;
	}
	string getTokenName(int id) const {
		return tokenNames.at(id);
	}
	unsigned getCellCount() const {
		return cellCount;
	}
	const vector<simulation::Perturbation*>& getPerts() const;
	const list<Init>& getInits() const;
	const list<state::Variable*>& getObservables() const;
	const map<string,const expressions::BaseExpression*>& getParams() const;


	const Compartment& getCompartmentByCellId(unsigned id) const;

	short idOfAlg(const string& name) const;
	short getVarId(const string &name) const;
	short getVarId(const grammar::ast::Id &name) const;
	short getChannelId(const string &name) const;
	short getCompartmentId(const string &name) const;
	short getSignatureId(const string &name) const;
	unsigned getTokenId(const string &name) const;

	state::BaseExpression* getVarExpression(const string &name) const;

	const DepSet& getDependencies(const Dependency& dep) const;
	const Dependencies& getDependencies() const;
	void addDependency(Dependency key,Dependency::Dep dep,unsigned id);

	template <typename T>
	size_t size() const;
	template <typename T>
	void reserve(size_t count);


	//DEBUG methods
	std::string cellIdToString(unsigned int cell_id) const;
	void show(const simulation::SimContext& context) const;

};

//using env = Environment::getInstance();

} /* namespace pattern */

#endif /* PATTERN_ENVIRONMENT_H_ */
