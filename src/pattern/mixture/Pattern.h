/*
 * Pattern.h
 *
 *  Created on: May 29, 2020
 *      Author: naxo100
 */

#ifndef SRC_PATTERN_MIXTURE_PATTERN_H_
#define SRC_PATTERN_MIXTURE_PATTERN_H_

#include "../../util/params.h"
#include <set>


//forward declaration of Rule
namespace simulation {
	class Rule;
}



namespace pattern {
//Forward declaration needed for setComponents() and toString()
class Environment;
using namespace std;

/* Abstract class Pattern *
 *
 * CCs and Mixtures are patterns.
 * They are included in rule's LHSs.
 */
class Pattern {
protected:
	std::set<small_id> includes;
public:
	class Agent;
	class Site;
	class Component;

	/** \brief the type of this Link pattern/state. */
	enum LinkType {
		FREE,		///< The site is disconnected (s).
		WILD,		///< The site's link-state is unknown (s!?).
		BND_ANY,	///< The site is bound to a not explicit site (s!_).
		BND_PTRN,	///< The site is bound to a not explicit site but with a pattern type (s!t.A).
		BND,		///< The site is bound to a explicit site in the mixture (s!1).
		PATH		///< Multi-Bindings (not implemented).(LAST)
	};
	enum ValueType {
		EMPTY,		///< The site value is unknown.
		AUTOFILL,	///< The RHS-site value have no difference with the LHS (...).
		LABEL,		///< The site have a label value (s~lbl).
		EQUAL,		///< The site have a numeric value (s~{[aux=]val}).
		DIFF,		///< The site have a (different) numeric value (s~{[aux<>]val}).
		GREATER,	///< The site num. value is no less than X (s~{X <= aux}).
		SMALLER,	///< The site num. value is no more than Y (s~{aux} <= Y).
		BETWEEN		///< The site num. value is between X and Y (s~{X <= aux <= Y}).(LAST)
	};
	virtual ~Pattern(){};
	virtual const short_id& getId() const = 0;
	virtual const Agent& getAgent(small_id ag_id ) const = 0;
	virtual size_t size() const = 0;
	const set<small_id>& includedIn() const{
		return includes;
	}
	virtual void addInclude(small_id id){
		includes.emplace(id);
	}
	virtual ag_st_id follow(small_id ag_id,small_id site) const = 0;
	virtual string toString(const Environment& env,int mark = -1) const = 0;
};

class Mixture;


/*
//pair of shorts used for (agent,agent) or (agent,site) ids.
ostream& operator<<(ostream& os,const ag_st_id &ag_st);

//struct to compare id_pair needed for maps
template <typename T1,typename T2>
struct ComparePair{
	bool operator()(pair<T1,T2> p1,pair<T1,T2> p2);
};
*/

}



#endif /* SRC_PATTERN_MIXTURE_PATTERN_H_ */
