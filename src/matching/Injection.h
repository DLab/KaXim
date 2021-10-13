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
#include "../pattern/mixture/Mixture.h"
#include "../data_structs/DistributionTree.h"
#include "../expressions/BaseExpression.h"
//#include "../state/Node.h"
#include "../data_structs/DebugPtrSet.h"
//#include "../simulation/SimContext.h"

namespace simulation {
class SimContext;
}

namespace state {
	//class State;
	class Node;
	//class Internal;
}
//namespace expressions {class AuxMixEmb;class AuxCcEmb;}

namespace matching {

using namespace std;

using Node = state::Node;

template <typename T>
class InjRandContainer;
template <typename T>
class InjRandSet;

/*struct try_match {
	const pattern::Mixture::Component* comp;
	Node* node;
	two<list<Internal*> > port_lists;
	small_id root;
};*/

class InjSet;

class Injection {
protected:
	friend class InjSet;
	const pattern::Pattern& ptrn;
	unsigned address;
	mutable int depCount;///> times this inj has been add to node site deps
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
	inline FL_TYPE evalAuxExpr(const simulation::SimContext& context,
			const expressions::BaseExpression* aux_expr) const;

	/** \brief Returns a new injection based on this one but
	 * with the nodes mapped by mask. */
	virtual Injection* clone(const map<Node*,Node*>& mask) const = 0;
	/** \brief Returns the nodes pointed by this Injection. */
	virtual const vector<Node*>* getEmbedding() const = 0;

	/** \brief Fills injs and cod with the nodes of this injection.
	 *
	 * @returns false if a node is already in cod. */
	virtual bool codomain(vector<Node*>* injs,set<Node*>& cod) const = 0;

	virtual size_t count() const = 0;
	//bool operator< (const Injection& inj) const;
	virtual bool operator==(const Injection& inj) const = 0;

protected:
	void incDeps() const {
		depCount++;
	}
	auto decDeps() const {
		return --depCount;
	}

};



class MixInjection : public Injection {
	const Injection* ccInjs[2];
	vector<Node*> emb[2];
	int distance;//radius
	//bool rev;

public:
	MixInjection(const pattern::Mixture& m) :
		Injection(m),ccInjs{nullptr,nullptr},distance(0){}
		//rev(&m.getComponent(0) > &m.getComponent(1) ? true : false){};
	MixInjection(const Injection* inj1,const Injection* inj2,const pattern::Mixture& m);
	MixInjection(const Injection* &injs,const pattern::Mixture& m) :
		MixInjection((&injs)[0],(&injs)[1],m) {}

	bool reuse(const simulation::SimContext& context,const Injection* inj1,const Injection* inj2,int d){
		/*if(rev){
			ccInjs[0] = inj2;ccInjs[1] = inj1;
		}
		else {}*/
		ccInjs[0] = inj1; ccInjs[1] = inj2;
		emb[0] = *ccInjs[0]->getEmbedding();
		emb[1] = *ccInjs[1]->getEmbedding();
		distance = d;
		return true;
	}
	Injection* clone(const map<Node*,Node*>& mask) const;
	void copy(const MixInjection* inj,const map<Node*,Node*>& mask);
	const vector<Node*>* getEmbedding() const {
		return emb;
	}
	bool codomain(vector<Node*>* injs,set<Node*>& cod) const {
		if(ccInjs[0]->isTrashed() || ccInjs[1]->isTrashed())
			return 5;//invalid injection
		return ccInjs[0]->codomain(injs,cod) || ccInjs[1]->codomain(&injs[1],cod);
	}

	size_t count() const {
		return ccInjs[0]->count();
	}
	bool operator==(const Injection& inj) const;

	int getDistance() const {
		return distance;
	}
	two<const Injection*> getInjs() const {
		return two<const Injection*>(ccInjs[0],ccInjs[1]);
	}

};


/***************** Injection inlines ****************/

static auto trshd = unsigned(-1);//TODO a global static?

inline Injection::Injection(const pattern::Pattern& _ptrn) : ptrn(_ptrn),
		address(unsigned(-1)),depCount(0) {
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

inline FL_TYPE Injection::evalAuxExpr(const simulation::SimContext& context,
		const expressions::BaseExpression* aux_func) const {
	//expressions::AuxCcEmb aux_map(getEmbedding());
	//expressions::EvalArgs args(&_args.getState(),&args.getVars(),&aux_map);
	return aux_func->getValue(context).valueAs<FL_TYPE>();
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
