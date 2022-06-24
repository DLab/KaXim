/*
 * Dependencies.cpp
 *
 *  Created on: Apr 21, 2016
 *      Author: naxo
 */

#include "Dependencies.h"

namespace pattern {

Dependencies::Dependencies() {
	// TODO Auto-generated constructor stub

}

Dependencies::~Dependencies() {
	// TODO Auto-generated destructor stub
}

const string Dependency::NAMES[] = {"TIME","EVENT","KAPPA","TOKEN","VAR","RULE","PERT","AUX","NONE"};

Dependency::Dependency(Dep d,unsigned i) : type(d),id(i),aux("") {}
//Dependency::Dependency(FL_TYPE f) : type(TIME),id(0),aux(""),dt(f) {}
Dependency::Dependency(string _aux) : type(AUX),id(0),aux(_aux) {}

bool Dependency::operator <(const Dependency& d) const {
	return type == d.type ?
				(id == d.id ? aux < d.aux : id < d.id) : type < d.type;
}

void Dependencies::addDependency(Dependency key,Dependency::Dep d, unsigned int id){
	deps[key].deps.emplace(d,id);
}

void Dependencies::addTimePertDependency(unsigned id,float time){
	deps[Dependency(Dependency::TIME)].ordered_deps.emplace(time,Dependency(Dependency::PERT,id));
}

const Dependencies::DepSet2& Dependencies::getDependencies(Dependency d) {
	return deps[d];
}

void Dependencies::erase(Dependency key, Dependency trigger){
	deps.erase(key);//erase dep
	deps[trigger].deps.erase(key);//erase the triggerin of this dep by trigger dep
	//deps[trigger].ordered_deps.erase(key); TODO
}

void Dependencies::eraseTimePerts(multimap<float,Dependency>::const_iterator first,
			multimap<float,Dependency>::const_iterator last){
	deps[Dependency::TIME].ordered_deps.erase(first,last);
}
void Dependencies::eraseTimePert(float time,Dependency key){
	auto range = deps[Dependency::TIME].ordered_deps.equal_range(time);
	while(range.first != range.second){
		if(range.first->second.id == key.id){
			deps[Dependency::TIME].ordered_deps.erase(range.first);
			return;
		}
	}
	throw invalid_argument("Dependencies::eraseTimePert(): Dep not found.");
}


} /* namespace pattern */
