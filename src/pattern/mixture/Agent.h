/*
 * Agent.h
 *
 *  Created on: Jan 21, 2019
 *      Author: naxo100
 */

#ifndef SRC_PATTERN_MIXTURE_AGENT_H_
#define SRC_PATTERN_MIXTURE_AGENT_H_

#include "Pattern.h"
#include "../../grammar/location.hh"
#include "../../matching/Injection.h"
#include "../Action.h"


namespace state {
class Node;
class InternalState;
}

namespace pattern {

class Environment;

using InjSet = matching::InjSet;






/** \brief Structure of an agent site of a mixture.
 * It stores the site value and link state of kappa
 * declared site of an agent mixture.
 */
class Pattern::Site {
	friend class Agent;
	friend class Mixture;
protected:
	small_id id;
	ValueType value_type;
	LinkType link_type;
	small_id label;
	expressions::BaseExpression* values[2];	///< [smaller=|value,greater=]
	mutable ag_st_id lnk_ptrn;	///< valid for BIND & BIND_PTRN.
	int lnk_id;

	state::Node* (Site::*testMatchFunc) (const state::InternalState*,map<int,InjSet*>&,
			const expressions::EvalArgs&) const;

	void setMatchFunction();

	template <ValueType vt,LinkType lt>
	state::Node* testMatchOpt(const state::InternalState*,
			map<int,InjSet*>&,const expressions::EvalArgs&) const;
	//template <ValueType vt>
	//expressions::SomeValue getValue(const expressions::EvalArgs& args) const;
	/*template <ValueType vt>
	void removeValueDep(two<InjSet*> &deps,matching::Injection* inj) const;
	template <LinkType lt>
	void removeBindDep(two<InjSet*> &deps,matching::Injection* inj) const;*/
public:
	struct Diff {
		ActionType t;
		small_id new_label;
		expressions::BaseExpression* new_value;
		bool side_effect;
		string warning;
	};
	/** \brief Constructs an empty site.
	 *
	 * Default ValueType is EMPTY and
	 * default link_type is WILD.				*/
	Site(small_id id,ValueType vtype = EMPTY,LinkType ltype = WILD);
	//Site(expressions::SomeValue _val,LinkType ltype = FREE);
	//Site(const Site& site);// = delete;
	~Site();

	small_id getId() const{
		return id;
	}

	void setValue(expressions::SomeValue val);
	void setValuePattern(expressions::BaseExpression** vals);
	void setBindPattern(LinkType lt,small_id ag_id = -1,small_id site_id = 0);

	Site& operator=(const Site& s) = delete;
	bool operator==(const Site &s) const;

	/** \brief Tests if this site-pattern may embed the rhs_site. */
	bool testEmbed(const Site &rhs_site,const expressions::EvalArguments<true>& args) const;
	//int compareLinkPtrn(ag_st_id ptrn) const;
	//bool isEmptySite() const;
	//bool isExpression() const;
	//bool isBindToAny() const;
	/*inline bool isBindToAnyOpt() const {
		return lnk_ptrn.first == small_id(-1);
	}*/
	/** \brief tests if val can be matched in this site pattern. (no-optimized)*/
	bool testValue(const expressions::SomeValue& val,
			const expressions::EvalArguments<true>& args) const;
	//bool testValueOpt(const expressions::SomeValue& val,
	//		const state::State& state,
	//		const expressions::AuxMap& aux_map) const;
	//bool testLink(const pair<const void*,small_id>& lnk) const;

	inline auto getValueType() const {
		return value_type;
	}
	inline auto getLinkType() const {
		return link_type;
	}

	template <bool safe>
	inline expressions::SomeValue getValue(const expressions::EvalArguments<safe>& args) const {
		return value_type == LABEL? expressions::SomeValue(label) :
				value_type == EQUAL? values[0]->getValue(args) :
						throw invalid_argument("Pattern::Site::getValue(): not a valued site.");
	}

	/** \brief Compare this site pattern to a RHS site and returns
	 * the list of differences/actions.				 */
	list<Diff> difference(const Site& rhs) const;

	/** \brief Tests whether a node's site (st) matches with this site.
	 *
	 * Tests if st matches with this mixture site.
	 * Stores in port_list every Dependency set that will
	 * need to store the new injection.
	 * @returns nullptr if no match, a node to follow if there is a Bind,
	 * and an invalid node (signId = -1) elsewhere. */
	inline state::Node* testMatch(const state::InternalState* st,
			map<int,InjSet*> &port_list,const expressions::EvalArgs& args) const {
		return (this->*testMatchFunc)(st,port_list,args);
	}

	/** \brief Optimized function to remove injection deps based on site pattern */
	/*inline void removeDep(two<InjSet*> &deps,matching::Injection* inj) const {
		removeValueDep<value_type>(deps,inj);
		removeBindDep<link_type>(deps,inj);
	}*/
	bool operator<(const Site& site) const {
		return id < site.id;
	}

	string toString(small_id,small_id ag_pos,map<ag_st_id,short> bindLabels,
			const Environment& env) const;
};

inline Pattern::Site::Site(small_id _id,ValueType vtype,LinkType lt) : id(_id),value_type(vtype),
		link_type(lt),label(0),values{nullptr,nullptr},lnk_ptrn(-1,-1),lnk_id(-int(id)-1){
	setMatchFunction();
}














/** \brief Class of an agent declared in a mixture.
 * Stores all site information of a kappa declared
 * agent in a mixture.
 */
class Pattern::Agent {
	short_id signId; 						///< signature ID
	std::set<Site> interface;		///< site-id -> site
	mutable list<pair<const Component*,small_id> > includedIn;	///< CCs including this agent-pattern
	bool autoFill;							///< when true, non declared sites will match LHS sites.

public:
	Agent(short_id name_id);
	~Agent();

	Agent(const Agent& a) = delete;
	Agent& operator=(const Agent& a) = delete;

	short_id getId() const;

	/** \brief Includes a copy of the site in this agent,
	 * returns a const ref to the copy, and destroys site.  */
	const Site& addSite(Site* site);

	/** \brief returns the number of declared sites. */
	size_t size() const {return interface.size();};

	/** \brief returns the site reference to the same site-id.
	 * Declares and return an empty site if it doesn't exists.
	 * site is destroyed. */
	const Site& getSite(Site* site);
	/** \brief returns a const ref to the site. unexpected if doesnt exists. (not safe)*/
	inline const Site& getSite(small_id id) const {
		return *interface.find(Site(id));
	}/** \brief returns a const ref to the site. unexpected if doesnt exists. (not safe)*/
	inline const Site& getSite(const Site& site) const {
		return *interface.find(site);
	}
	/** \brief Returns the site of this id.
	 * If it doesnt exists, returns an empty site or an auto-fill site.	 */
	const Site& getSiteSafe(small_id id) const;
	bool operator==(const Agent& a) const;

	/** \brief Test if this agent-pattern embeds the rhs_site.
	 * @param sites_to_test stores site-ids of every bind that will need to be tested. */
	bool testEmbed(const Agent& rhs_ag,const expressions::EvalArguments<true>& args,list<small_id>& sites_to_test) const;
	inline bool testEmbed(const Agent& rhs_ag,const expressions::EvalArguments<true>& args) const{
		static list<small_id> empty_list;
		return testEmbed(rhs_ag,args,empty_list);
	}

	/** \brief Set this RHS-pattern to auto fill missing sites. */
	void setAutoFill();
	bool isAutoFill() const{return autoFill;}

	/** \brief Add a cc that declares this agent pattern. */
	void addCc(const Component* cc,small_id id) const;

	/** \brief CCs that include this pattern.
	 * @returns a list of CCs where this agent-pattern is included (and its own position in CC). */
	const list<pair<const Component*,small_id> >& getIncludes() const;

	const set<Site>::const_iterator begin() const;
	const set<Site>::const_iterator end() const;

	string toString( const Environment& env, short mixAgId=-1,
			map<ag_st_id,short> bindLabels = map<ag_st_id,short>() ) const;
};



/*
template <> inline
expressions::SomeValue Pattern::Site::getValue<Pattern::EQUAL>(const expressions::EvalArgs& args) const {
	return values[0]->getValue(args);
}
template <> inline
expressions::SomeValue Pattern::Site::getValue<Pattern::LABEL>(const expressions::EvalArgs& args) const {
	return expressions::SomeValue(label);
}

template <Pattern::ValueType vt> inline
void Pattern::Site::removeValueDep(two<InjSet*> &deps,matching::Injection* inj) const {
	deps.first->erase(inj);
}
template <> inline
void Pattern::Site::removeValueDep<Pattern::ValueType::EMPTY>(two<InjSet*> &deps,matching::Injection* inj) const {}

template <Pattern::LinkType lt> inline
void Pattern::Site::removeBindDep(two<InjSet*> &deps,matching::Injection* inj) const {
	deps.second->erase(inj);
}
template <> inline
void Pattern::Site::removeBindDep<Pattern::LinkType::WILD>(two<InjSet*> &deps,matching::Injection* inj) const {}
*/

/*
struct LabelSite : public virtual Pattern::Site {
	small_id label;//small_id(-2) -> aux
public:
	LabelSite(small_id val = EMPTY);

	virtual const state::Node* testEmbedOpt(const state::Node* node,set<InjSet*> port_list,
			const state::State& state,expressions::AuxMap& aux_map) const;
};

struct ValueSite : public virtual Pattern::Site {
	expressions::BaseExpression* values[3];//[smaller=,value,greater=]
public:
	ValueSite();

	const state::Node* testEmbedOpt(const state::Node* node,set<InjSet*> port_list,
			const state::State& state,expressions::AuxMap& aux_map) const;
};

struct LinkSite : public virtual Pattern::Site {
	Pattern::LinkType link_type;
	ag_st_id lnk_ptrn;	//< valid for BIND|BIND_TO. agent_id,site (-1,-1)
public:
	LinkSite(Pattern::LinkType ltype = Pattern::FREE);

	virtual const state::Node* testEmbedOpt(const state::Node* node,set<InjSet*> port_list,
			const state::State& state,expressions::AuxMap& aux_map) const;
};

struct LabelLinkSite : public LabelSite,public LinkSite {
public:
	LabelLinkSite(small_id val = EMPTY,Pattern::LinkType ltype = Pattern::FREE);

	virtual const state::Node* testEmbedOpt(const state::Node* node,set<InjSet*> port_list,
				const state::State& state,expressions::AuxMap& aux_map) const;
};

class ValueLinkSite : public ValueSite,public LinkSite {
public:
	ValueLinkSite(Pattern::LinkType ltype = Pattern::FREE);

	virtual const state::Node* testEmbedOpt(const state::Node* node,set<InjSet*> port_list,
				const state::State& state,expressions::AuxMap& aux_map) const;
};


template <typename T>
class RangedSite : public Pattern::Site {
	T max,min;

public:
	bool operator==(const Site &s) const;
	int compare(const Site &s) const;
	bool isEmptySite() const;//TODO inline???
	bool isBindToAny() const;
};
*/

} /* namespace pattern */

#endif /* SRC_PATTERN_MIXTURE_AGENT_H_ */
