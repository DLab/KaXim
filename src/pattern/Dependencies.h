/*
 * Dependencies.h
 *
 *  Created on: Apr 21, 2016
 *      Author: naxo
 */

#ifndef PATTERN_DEPENDENCIES_H_
#define PATTERN_DEPENDENCIES_H_

#include <map>
#include <set>
#include <vector>
#include <list>
#include "../util/params.h"

namespace pattern {

using namespace std;

struct Dependency {
	enum Dep {TIME,EVENT,KAPPA,TOK,VAR,RULE,PERT,AUX,NONE};
	static const string NAMES[];
	Dep type;
	//union{
		big_id id;
		string aux;
		//FL_TYPE dt;
	//};
	Dependency(Dep d = Dep::NONE,unsigned i = 0);
	//Dependency(FL_TYPE f);
	Dependency(string _aux);
	bool operator<(const Dependency& d) const;

	string toString(){
		switch(type){
		case TIME:case EVENT:case NONE:
			return NAMES[type];
		case KAPPA:case TOK:case VAR:case RULE:case PERT:
			return NAMES[type] + "(" +to_string(id) + ")";
		case AUX:
			return NAMES[type] + "(" + aux + ")";
		}
		throw invalid_argument("invalid dependency");
	}
};

class Dependencies {
public:
	struct DepSet2 {
		set<Dependency> deps;
		multimap<float,Dependency> ordered_deps;//todo idea to improve time execution in time perturbations?
	};
protected:
	map< Dependency , DepSet2 > deps;
public:
	Dependencies();
	~Dependencies();

	void addDependency(Dependency key,Dependency::Dep d, unsigned int id);
	void addTimePertDependency(unsigned p_id,float time);
	const DepSet2& getDependencies(Dependency d);

	void erase(Dependency key,Dependency trigger);
	void eraseTimePerts(multimap<float,Dependency>::const_iterator first,
			multimap<float,Dependency>::const_iterator last);
	void eraseTimePert(float time,Dependency key);
};

typedef set<Dependency> DepSet;


} /* namespace pattern */



#endif /* PATTERN_DEPENDENCIES_H_ */
