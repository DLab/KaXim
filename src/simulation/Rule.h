/*
 * Rule.h
 *
 *  Created on: May 9, 2017
 *      Author: naxo
 */

#ifndef SRC_SIMULATION_RULE_H_
#define SRC_SIMULATION_RULE_H_

#include "../util/params.h"
#include "../pattern/mixture/Mixture.h"
#include "../pattern/mixture/Agent.h"
#include "../grammar/location.hh"
#include "../state/SiteGraph.h"
#include "../pattern/Action.h"
#include <list>
#include <unordered_map>

//class pattern::Environment;

/*namespace matching {
template <typename T> class InjRandTree;
}*/


namespace simulation {

using namespace std;
using namespace pattern;
using namespace expressions;


/** \brief Basic unit of the simulation dynamics.
 *
 * Rules are the basic unit of the simulation dynamics. They will be applied
 * to Nodes in a SiteGraph that match with its LHS. Actions are the changes
 * that the rule produce in those nodes. difference() must be called before
 * trying to apply the rule.
 */
class Rule {
	friend class pattern::Environment;
public:
	struct CandidateKey {
		const Mixture::Component* cc;
		ag_st_id match_root;			///< (ag-pos in cc, ag-pos in rhs)
		bool operator<(const CandidateKey& key) const {
			return match_root.second == key.match_root.second ?(
				match_root.first == key.match_root.first ?
					cc < key.cc :
					match_root.first < key.match_root.first) :
				match_root.second < key.match_root.second;
		}
	};
	struct CandidateInfo {
		ag_st_id emb_coords;
		bool is_new;
		set<int> infl_by;
	};
	/// ( CC-ptrn , root-match ) -> cc-coords,emb-coords
	typedef map<CandidateKey,CandidateInfo> CandidateMap;
	//TODO analyze possibility of a changing agent influence 2 different agents in the same CC
	class Rate {
	protected:
		const Rule& rule;
		BaseExpression* baseRate;
		pair<BaseExpression*,int> unaryRate;
	public:
		Rate(const Rule& r) : rule(r),baseRate(nullptr) {};
		Rate(const Rule& r,state::State& state);
		virtual ~Rate();

		virtual two<FL_TYPE> evalActivity(const SimContext &context) const {
			return two<FL_TYPE>(.0,.0);
		}
		virtual const BaseExpression* getExpression(small_id cc_index = 0) const;
		//CandidateMap influence;
	};
protected:
	int id;
	string name;
	Mixture &lhs;
	Mixture *rhs;
	bool isRhsDeclared;
	const BaseExpression *rate;//basic rate
	pair<const BaseExpression*,int> unaryRate;//rate,radius
	//BaseExpression::Reduction basic,unary;
	list<Action> script;
	int bindCount;
	small_id matchCount;		///< counter of lhs agents paired with rhs agents
	vector<state::Node*> newNodes;
	CandidateMap influence;
	//map<ag_st_id,ag_st_id> matches;//rhs-ag -> lhs-ag
	list<pair<unsigned,const BaseExpression*> > tokenChanges;

public:
	/** \brief Initialize a rule with a declared kappa label and its LHS.
	 * @param nme Declared kapa label of rule.
	 * @param mix LHS of the rule.
	 */
	Rule(const string& nme, Mixture& mix,const yy::location& _loc);
	~Rule();

	int getId() const;
	const string& getName() const;
	/*void setName(string nme) {
		name = nme;
	}*/
	const Mixture& getLHS() const;
	Mixture& getLHS();
	const Mixture& getRHS() const;
	const BaseExpression& getRate() const;
//	const BaseExpression::Reduction& getReduction() const;
	const pair<const BaseExpression*,int>& getUnaryRate() const;
//	const BaseExpression::Reduction& getUnaryReduction() const;
	const CandidateMap& getInfluences() const;
	const list<pair<unsigned,const BaseExpression*> > getTokenChanges() const;
	int getBindCount() const {
		return bindCount;
	}

	//two<FL_TYPE> evalActivity(const matching::InjRandContainer* const * injs,const VarVector& vars) const;

	/** \brief Set RHS of the rule.
	 * If this is a reversible rule, mix is declared in env and should not be
	 * freed by rule destructor.				*/
	void setRHS(Mixture* mix,bool is_declared = false);

	/** \brief Set basic rate for the rule.
	 * @param r basic rate of the rule.			*/
	void setRate(const BaseExpression* r);

	/** \brief Set the unary rate for the rule.
	 * @param u_rate pair of (rate,radius) for unary instances of a binary rule.
	 * @param Radius indicate the range of search for connectivity.		*/
	void setUnaryRate(pair<const BaseExpression*,int> u_rate = make_pair(nullptr,0));

	/** \brief Calculate actions to apply using the difference between LHS and RHS.
	 * Actions could create, delete, bind or unbind nodes, and change internal state
	 * of node sites. This method match the first agents of LHS and RHS by its signId,
	 * preserving those nodes, and after first unmatched agents, delete every node from
	 * LHS and create new nodes for every extra agent in RHS.
	 * @param env the environment of the simulation.
	 * @param lhs_order initial order of LHS agents (obtained from Mixture::declare()).
	 * @param rhs_order initial order of RHS agents.
	 * @param consts to evaluate const-expressions.			*/
	void difference(const Environment& env,const SimContext &context);

	void addTokenChange(pair<unsigned,const BaseExpression*> tok);

	const list<Action>& getScript() const;
	const vector<state::Node*>& getNewNodes() const;

	static void addAgentIncludes(CandidateMap &m,
		const Pattern::Agent& ag,small_id rhs_ag_pos,CandidateInfo info);


	void checkInfluence(const Environment &env,const SimContext &context);
	void initialize(const state::State& state,VarVector& vars);

	virtual int getTargetCell(int id) const {
		return 0;
	}


	string toString(const pattern::Environment& env) const;

	const yy::location loc;
};



class NormalRate : public virtual Rule::Rate {
public:
	NormalRate(const Rule& r,state::State& state);
	virtual ~NormalRate();

	two<FL_TYPE> evalActivity(const SimContext &context) const override;
};

class SamePtrnRate : public virtual Rule::Rate {
protected:
	float norm;
public:
	SamePtrnRate(const Rule& r,state::State& state,bool normalize = false);
	virtual ~SamePtrnRate();

	two<FL_TYPE> evalActivity(const SimContext &context) const override;
};

class AuxDepRate : public virtual Rule::Rate {
protected:
	BaseExpression::Reduction base,unary;
public:
	AuxDepRate(const Rule& r,state::State& state);
	virtual ~AuxDepRate();

	virtual const BaseExpression* getExpression(small_id cc_index = 0) const override;
	virtual two<FL_TYPE> evalActivity(const SimContext &context) const override;
};

class SameAuxDepRate : public SamePtrnRate,public AuxDepRate {
public:
	SameAuxDepRate(const Rule& r,state::State& state,bool normalize);
	virtual ~SameAuxDepRate();

	virtual two<FL_TYPE> evalActivity(const SimContext &context) const override;
};




} /* namespace simulation */

#endif /* SRC_SIMULATION_RULE_H_ */
