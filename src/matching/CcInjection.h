/*
 * CcInjection.h
 *
 *  Created on: Oct 7, 2021
 *      Author: naxo
 */

#ifndef SRC_MATCHING_CCINJECTION_H_
#define SRC_MATCHING_CCINJECTION_H_

#include "Injection.h"
#include "../state/Node.h"


namespace matching {


class CcInjection : public Injection {
	//friend class InjRandSet;
	//map<small_id,big_id> ccAgToNode;//unordered
	vector<Node*> ccAgToNode;
	//const pattern::Mixture::Component& cc;//cc_id

public:
	//CcInjection(const pattern::Pattern& _cc);
	CcInjection(const pattern::Pattern& _cc);
	Injection* clone(const map<Node*,Node*>& mask) const override;
	void copy(const CcInjection* inj,const map<Node*,Node*>& mask);
	~CcInjection();

	bool reuse(const simulation::SimContext& context,Node* node,
			map<int,InjSet*>& port_list,small_id root = 0);

	inline const vector<Node*>* getEmbedding() const override {
		return &ccAgToNode;
	}

	bool codomain(vector<Node*>* injs,set<Node*>& cod) const override;

	inline size_t count() const override {
		return ccAgToNode[0]->getCount();
	}

	bool operator==(const Injection& inj) const override;
};





class CcValueInj : public CcInjection {
	map<distribution_tree::DistributionTree<CcValueInj>*,int> containers;//TODO try with unordered_map
	//FL_TYPE value;
public:
	CcValueInj(const pattern::Pattern::Component& _cc);
	CcValueInj(const CcInjection& inj);

	void addContainer(distribution_tree::DistributionTree<CcValueInj>& cont,int addr);
	void selfRemove();
	//returns true if the containers list is empty
	bool removeContainer(distribution_tree::DistributionTree<CcValueInj>& cont);

};




/************* CcValueInj Inlines ****************************/

inline CcValueInj::CcValueInj(const pattern::Pattern::Component& _cc) : CcInjection(_cc) {}

inline CcValueInj::CcValueInj(const CcInjection& inj) : CcInjection(inj) {};

inline void CcValueInj::addContainer(distribution_tree::DistributionTree<CcValueInj>& cont,int address){
	containers[&cont] = address;this->address++;
}

inline bool CcValueInj::removeContainer(distribution_tree::DistributionTree<CcValueInj>& cont){
	containers.erase(&cont);address--;
	return containers.empty();
}

inline void CcValueInj::selfRemove(){
	for(auto cont_addr : containers){
		auto inj = cont_addr.first->erase(cont_addr.second);//TODO this will delete cont, check iterator invalidation
#ifdef DEBUG
		if(this != inj)
			throw invalid_argument("erasing another inj (CcValueInj::selfRemove())");
#endif
	}
	containers.clear();//is mandatory!
}





/*namespace distribution_tree{
	template <typename T>
	class DistributionTree<T>;
}*/


} /* namespace matching */

#endif /* SRC_MATCHING_CCINJECTION_H_ */
