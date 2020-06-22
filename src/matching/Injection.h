/*
 * Injection.h
 *
 *  Created on: Apr 11, 2017
 *      Author: naxo
 */

#ifndef SRC_MATCHING_INJECTION_H_
#define SRC_MATCHING_INJECTION_H_

#include <map>
#include <set>
#include <unordered_set>
#include <vector>
#include <list>
#include <random>
#include "../util/params.h"
//#include "../state/SiteGraph.h"
#include "../pattern/mixture/Pattern.h"
#include "../data_structs/DistributionTree.h"
#include "../expressions/BaseExpression.h"

namespace state {
	class State;
	class Node;
	class Internal;
}
namespace expressions {class AuxMixEmb;class AuxCcEmb;}

namespace matching {

using namespace std;

using Node = state::Node;


class InjRandContainer;
class InjRandSet;

/*struct try_match {
	const pattern::Mixture::Component* comp;
	Node* node;
	two<list<Internal*> > port_lists;
	small_id root;
};*/


class Injection {
protected:
	friend class InjRandSet;
	const pattern::Pattern& ptrn;
	unsigned address;
#ifdef DEBUG
	int cc_id;
#endif
public:
	/** \brief Constructs a new pattern based injection. */
	Injection(const pattern::Pattern& ptrn);
	virtual ~Injection();

	/** \brief Set the container-address of this Injection. */
	inline void alloc(unsigned addr);
	/** \brief Unset the container address of this injection. */
	inline void dealloc();
	/** \brief Returns true if the Injection is not allocated. */
	inline bool isTrashed() const;
	/** \brief Returns a reference to the pattern of this Injection. */
	inline const pattern::Pattern& pattern() const;
	/** \brief Returns the container-address of the Injection. */
	inline unsigned getAddress() const;
	/** \brief Returns the value of aux_expr using the internal values
	 * of the nodes pointed by this injection. */
	inline FL_TYPE evalAuxExpr(const expressions::EvalArgs& args,
			const expressions::BaseExpression* aux_expr) const;

	/** \brief Returns a new injection based on this one but
	 * with the nodes mapped by mask. */
	virtual Injection* clone(const map<Node*,Node*>& mask) const = 0;
	/** \brief Returns the nodes pointed by this Injection. */
	virtual const vector<Node*>& getEmbedding() const = 0;

	virtual void codomain(vector<Node*>& injs,set<Node*>& cod) const = 0;

	virtual size_t count() const = 0;
	//bool operator< (const Injection& inj) const;
	virtual bool operator==(const Injection& inj) const = 0;

};


#ifdef DEBUG
class InjSet : public set<Injection*> {
public:
	inline auto emplace(Injection* inj) {
		for(auto dep : *this)
			if(*inj == *dep)
				throw invalid_argument("Node::addDep(): cannot add same dependency (cc->emb).");
		return set<Injection*>::emplace(inj);
	}
	using set<Injection*>::erase;
};

#else
class InjSet : public set<Injection*>{};
#endif





class CcInjection : public Injection {
	friend class InjRandSet;
	//map<small_id,big_id> ccAgToNode;//unordered
	vector<Node*> ccAgToNode;
	//const pattern::Mixture::Component& cc;//cc_id

public:
	CcInjection(const pattern::Pattern& _cc);
	CcInjection(const pattern::Pattern::Component& cc);
	Injection* clone(const map<Node*,Node*>& mask) const override;
	void copy(const CcInjection* inj,const map<Node*,Node*>& mask);
	~CcInjection();

	bool reuse(const pattern::Pattern::Component& cc,Node& node,
			map<int,InjSet*>& port_list,const expressions::EvalArgs& args,small_id root = 0);

	const vector<Node*>& getEmbedding() const override;

	void codomain(vector<Node*>& injs,set<Node*>& cod) const override;

	size_t count() const override;

	bool operator==(const Injection& inj) const override;
};


/*namespace distribution_tree{
	template <typename T>
	class DistributionTree<T>;
}*/

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


/***************** Injection inlines ****************/

static auto trshd = unsigned(-1);//TODO a global static?

inline Injection::Injection(const pattern::Pattern& _ptrn) : ptrn(_ptrn),address(unsigned(-1)) {
#ifdef DEBUG
	cc_id = ptrn.getId();
#endif
}

inline Injection::~Injection() {}

inline void Injection::alloc(unsigned addr){
	address = addr;
}
inline void Injection::dealloc(){
	address = trshd;
}

inline bool Injection::isTrashed() const {
	return address == trshd;
}

inline unsigned Injection::getAddress() const {
	return address;
}

inline const pattern::Pattern& Injection::pattern() const{
	return ptrn;
}

inline FL_TYPE Injection::evalAuxExpr(const expressions::EvalArgs& args,
		const expressions::BaseExpression* aux_func) const {
	//expressions::AuxCcEmb aux_map(getEmbedding());
	//expressions::EvalArgs args(&_args.getState(),&args.getVars(),&aux_map);
	return aux_func->getValue(args).valueAs<FL_TYPE>();
}



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







/*class MixInjection : Injection {
	big_id** agCcIdToNode;
	mid_id address;
	small_id coordinate;//mix_id

public:
	MixInjection(small_id size);
	~MixInjection();

	const vector<Node*>& getEmbedding() const override;

	bool isTrashed() const override;

	void codomain(Node* injs[],set<Node*> cod) const override;
};*/

//TODO choose the best implementation of InjSet
//typedef set<Injection*> InjSet;
/*class InjSet : public set<Injection*> {
	//unsigned counter;
public:
	//const Injection& chooseRandom() const;
	//unsigned count() const;

};*/



} /* namespace simulation */

#endif /* SRC_SIMULATION_INJECTION_H_ */
