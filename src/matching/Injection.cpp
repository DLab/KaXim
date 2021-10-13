/*
 * Injection.cpp
 *
 *  Created on: Apr 11, 2017
 *      Author: naxo
 */

#include <queue>
#include <iostream>
#include "Injection.h"
#include "../state/State.h"
#include "../state/Node.h"
#include "../expressions/Vars.h"
#include "../pattern/mixture/Component.h"
#include "../pattern/mixture/Agent.h"

namespace matching {



/********** MixInjection *****************/

MixInjection::MixInjection(const Injection* inj1,const Injection* inj2,const pattern::Mixture& m) :
	Injection(m),ccInjs{inj1,inj2},distance(m.getRadius()){
	//rev(&m.getComponent(0).getId() < &m.getComponent(1).getId() ? true : false){
	/*if(rev){
		ccInjs[0] = inj2;
		ccInjs[1] = inj1;
	}*/
	emb[0] = *ccInjs[0]->getEmbedding();
	emb[1] = *ccInjs[1]->getEmbedding();
		//throw invalid_argument("MixInjection(): patterns should be sorted.");
}

Injection* MixInjection::clone(const map<Node*,Node*>& mask) const {
	return nullptr;
}
void MixInjection::copy(const MixInjection* inj,const map<Node*,Node*>& mask){
	throw invalid_argument("cannot copy mix injection.");
}


bool MixInjection::operator==(const Injection& inj) const {
	auto& mix_inj = static_cast<const MixInjection&>(inj);
	if(ccInjs[0] == mix_inj.ccInjs[0] && ccInjs[1] == mix_inj.ccInjs[1])
		return true;
	return false;
}



/************ InjSet *****************/

//----------------------------------



} /* namespace simulation */
