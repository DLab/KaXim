/*
 * InjRandSet.h
 *
 *  Created on: Jun 26, 2018
 *      Author: naxo100
 */

#ifndef SRC_MATCHING_INJRANDSET_H_
#define SRC_MATCHING_INJRANDSET_H_

#include "Injection.h"
#include "CcInjection.h"
#include "../data_structs/DistributionTree.h"
#include "../simulation/Rule.h"

//#include <unordered_map>


namespace matching {

//using Node = state::Node;
template <typename InjType>
class InjLazyContainer;
using BE = expressions::BaseExpression;

template <typename InjType>
class InjRandContainer {
	friend class state::InternalState;
	friend class InjLazyContainer<InjType>;
	friend class state::State;
protected:
	list<InjType*> freeInjs;			///> Empty-object pool to avoid construction
	const pattern::Pattern& ptrn;	///> Pattern of the injections stored

	/** SUMS */
	mutable unordered_map<const BE*,FL_TYPE&> trackedSums;
	mutable list<pair<const BE*,FL_TYPE>> sumList;

	/** \brief introduce a new injection in the container. */
	virtual list<pair<const BE*,FL_TYPE>>& insert(InjType* inj,const state::State& state) = 0;
	/** \brief deletes the injection from the container. */
	virtual void del(Injection* inj) = 0;
	/** @returns a new empty injection (not from the pool). */
	virtual InjType* newInj() const = 0;

	InjType* selectInj(const state::State& state);

public:

	InjRandContainer(const pattern::Pattern& _ptrn);
	virtual ~InjRandContainer();
	virtual InjType& chooseRandom(RNG& randGen) const = 0;
	virtual InjType& choose(unsigned id) const  = 0;
	virtual size_t count() const = 0;
	virtual FL_TYPE partialReactivity() const = 0;

	template <typename... Args>
	InjType* emplace(const state::State& state, Args&... args){
		auto inj = selectInj(state);
#ifdef DEBUG
		if(inj == nullptr) throw invalid_argument("InjRandContainer::emplace(): null inj to reuse.");
#endif
		if(inj->reuse(state,args...) == false)
			return nullptr;
		auto sum_list = insert(inj,state);
		//this may have been deleted from here
		for(auto& s : sum_list) //replacing sumList because this may be deleted
			s.second += inj->evalAuxExpr(state,s.first);

		return inj;
	}
	virtual InjType* emplace(Injection* base_inj,map<Node*,Node*>& mask,
			const state::State& state);

	inline virtual void erase(Injection* inj,const simulation::SimContext& context);
	//inline virtual void erase(unsigned address,const simulation::SimContext& context);
	virtual void clear() = 0;

	virtual void selectRule(int rid,small_id cc) const;
	virtual FL_TYPE getM2() const;

	virtual FL_TYPE sumInternal(const expressions::BaseExpression* aux_func,
			const simulation::SimContext& context) const;
	//vector<CcInjection*>::iterator begin();
	//vector<CcInjection*>::iterator end();


	virtual void fold(const function<void (const Injection*)> func) const = 0;
};

template <typename InjType>
class InjRandSet : public InjRandContainer<InjType> {
	size_t counter;
	size_t multiCount;
	vector<InjType*> container;
	list<pair<const BE*,FL_TYPE>>& insert(InjType* inj,const state::State& state) override;
	virtual InjType* newInj() const override;
public:
	InjRandSet(const pattern::Pattern& _cc);
	~InjRandSet();
	InjType& chooseRandom(RNG& randGen) const override;
	InjType& choose(unsigned id) const override;
	size_t count() const override;
	virtual FL_TYPE partialReactivity() const;

	/*Injection* emplace(Node& node,two<std::list<state::Internal*> > &port_lists,
			const state::State& state,small_id root = 0) override;*/
	//Injection* emplace(Injection* base_inj,map<Node*,Node*>& mask) override;
	void del(Injection* inj) override;
	virtual void clear() override;

	//FL_TYPE sumInternal(const expressions::BaseExpression* aux_func,
	//		const expressions::EvalArgs& args) const override;
	virtual void fold(const function<void (const Injection*)> func) const;
	//vector<CcInjection*>::iterator begin();
	//vector<CcInjection*>::iterator end();
};


template <>
class InjRandSet<MixInjection> : public InjRandContainer<MixInjection> {
	InjRandSet<MixInjection>* next;
	size_t counter;
	size_t multiCount;
	vector<MixInjection*> container;
	int radius;				///> max-radius this container can store
	list<pair<const BE*,FL_TYPE>>& insert(MixInjection* inj,const state::State& state) override;
	virtual MixInjection* newInj() const override;
public:
	InjRandSet(const pattern::Mixture& mix,InjRandSet<MixInjection>* nxt = nullptr);
	~InjRandSet();
	MixInjection& chooseRandom(RNG& randGen) const override;
	MixInjection& choose(unsigned id) const override;
	size_t count() const override;
	virtual FL_TYPE partialReactivity() const;

	/*Injection* emplace(Node& node,two<std::list<state::Internal*> > &port_lists,
			const state::State& state,small_id root = 0) override;*/
	//MixInjection* emplace(Injection* inj1,Injection* inj2,int d) override;
	void del(Injection* inj) override;
	virtual void clear() override;

	//FL_TYPE sumInternal(const expressions::BaseExpression* aux_func,
	//		const expressions::EvalArgs& args) const override;
	virtual void fold(const function<void (const Injection*)> func) const;
	//vector<CcInjection*>::iterator begin();
	//vector<CcInjection*>::iterator end();
};



template <typename InjType>
class InjRandTree : public InjRandContainer<InjType> {
	static const float MAX_INVALIDATIONS;//3%
	//list<FL_TYPE> hints;

	//list<CcInjection*> freeInjs;
	list<InjType*> infList;

	/** \brief A root of the rule-rate associated with a CC	 */
	struct Root {
		/** a tree structure containing injs ordered by partial-rate value */
		distribution_tree::DistributionTree<CcValueInj>* tree;
		/** the partial-rate of the CC in the rule. */
		const expressions::BaseExpression* partial_rate;
		/** second-moment of the injs collection, useful for approximations*/
		two<FL_TYPE> second_moment;//(sum x*x,sum x) at validation
		bool is_valid;

		Root(const expressions::BaseExpression* rate);
	};
	unsigned counter;
	// (r_id,cc_index) -> contribution to rule-rate per cc
	map<pair<int,small_id>,Root*> roots,roots_to_push;
	mutable int invalidations;

	mutable pair<int,small_id> selected_root;

	list<pair<const BE*,FL_TYPE>>& insert(InjType *inj,const state::State& state) override;
	virtual InjType* newInj() const override;

public:
	InjRandTree(const pattern::Pattern::Component& cc,const vector<simulation::Rule::Rate*>& rates);
	~InjRandTree();
	InjType& chooseRandom(RNG& randGen) const override;
	InjType& choose(unsigned id) const override;
	size_t count() const override;
	virtual FL_TYPE partialReactivity() const override;

	FL_TYPE getM2() const override;

	void del(Injection* inj) override;
	virtual void clear() override;


	void selectRule(int rid,small_id cc) const override;

	FL_TYPE sumInternal(const expressions::BaseExpression* aux_func,
				const simulation::SimContext& context) const override;
	virtual void fold(const function<void (const Injection*)> func) const;
	//void addHint(FL_TYPE value);
};


template <typename InjType>
class InjLazyContainer : public InjRandContainer<InjType> {
	const state::State& state;
	const state::SiteGraph& graph;
	mutable InjRandContainer<InjType> **holder;
	mutable bool loaded;

	void loadContainer() const;
	void loadEmptyContainer() const;


	list<pair<const BE*,FL_TYPE>>& insert(InjType* inj,const state::State& state) override;
	virtual InjType* newInj() const override;

public:
	InjLazyContainer(const pattern::Mixture::Component& cc,const state::State& state,
			const state::SiteGraph& graph,InjRandContainer<InjType> *&holder);
	~InjLazyContainer();

	virtual InjType& chooseRandom(RNG& randGen) const override;
	virtual InjType& choose(unsigned id) const override;
	virtual size_t count() const override;
	virtual FL_TYPE partialReactivity() const override;

	/*virtual InjType* emplace(Node& node,map<int,InjSet*> &port_lists,
			const state::State& state,small_id root = 0) override;
	virtual InjType* emplace(Injection* base_inj,map<Node*,Node*>& mask,
			const state::State& state) override;*/
	virtual void erase(Injection* inj,const simulation::SimContext& context) override;
	virtual void del(Injection* inj) override;
	virtual void clear() override;

	virtual void selectRule(int rid,small_id cc) const override;
	virtual FL_TYPE getM2() const override;

	virtual FL_TYPE sumInternal(const expressions::BaseExpression* aux_func,
			const simulation::SimContext& context) const override;

	virtual void fold(const function<void (const Injection*)> func) const override;
};


template <typename InjType>
inline void InjRandContainer<InjType>::erase(Injection* inj,const simulation::SimContext& context){
	for(auto& s : sumList)
		s.second -= inj->evalAuxExpr(context,s.first);
	del(inj);
	//freeInjs.push_back(static_cast<InjType*>(inj));//TODO repair this!!!!
	inj->dealloc();//dealloc
}

//template <>
//void InjRandTree<CcValueInj>::insert(CcValueInj* inj,const state::State& state);

}

#endif /* SRC_MATCHING_INJRANDSET_H_ */
