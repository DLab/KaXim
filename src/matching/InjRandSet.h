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


/** \brief Generic container for Injections. */
template <typename InjType>
class InjRandContainer {
	friend class state::InternalState;
	friend class InjLazyContainer<InjType>;
	friend class state::State;
protected:
	list<InjType*> freeInjs;			///> Empty-Injection pool to avoid construction
	const pattern::Pattern& ptrn;		///> Pattern of the injections stored

	/** SUMS */
	mutable list<pair<const BE*,FL_TYPE>> sumList;			///> Sumatory of internal values of this pattern that need to be tracked.
	mutable unordered_map<const BE*,FL_TYPE&> trackedSums;	///> Map of sumatories for quick access.

	/** \brief introduce a new injection in the container. */
	virtual list<pair<const BE*,FL_TYPE>>& insert(InjType* inj,const state::State& state) = 0;
	/** \brief deletes the injection from the container. */
	virtual void del(Injection* inj) = 0;
	/** @returns a new empty injection (not from the pool). */
	virtual InjType* newInj() const = 0;

	/** \brief Sets an empty Injection from the container to
	 * the State Auxiliary Map.						 */
	InjType* selectInj(const state::State& state);

public:

	InjRandContainer(const pattern::Pattern& _ptrn);
	virtual ~InjRandContainer();
	/** \brief Choose a random Injection from this container.
	 * Every Injection has the same probability.
	 * \param randGen The random generator.
	 * \return the chosen injection.	 */
	virtual InjType& chooseRandom(RNG& randGen) const = 0;
	/// Returns the id-th injection in this container.
	virtual InjType& choose(unsigned id) const  = 0;
	/// The counter of embedding pointed by these injections.
	/** SubNodes count for several embedding. */
	virtual size_t count() const = 0;
	/// Calculates the partial reactivity of this pattern in a rule.
	/** For simple rules, this is just same as count(). But in rules where each CC instance have their own
	 * weight, this have to be calculated.	 */
	virtual FL_TYPE partialReactivity() const = 0;

	/// Tries to create and insert a new Injection in the container.
	/** If reuse() returns true, the new Injection is stored in this container.
	 * The sumatories are updated if any.
	 * \return nullptr if fails, else *new injection.	 */
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
	/// Tries to create and insert a new injection based in a copy from another injection.
	virtual InjType* emplace(Injection* base_inj,map<Node*,Node*>& mask,
			const state::State& state);

	/// Tries to erase \param inj from this container
	inline virtual void erase(Injection* inj,const simulation::SimContext& context);
	//inline virtual void erase(unsigned address,const simulation::SimContext& context);

	/// Remove all injections from this container.
	virtual void clear() = 0;

	/// Sets Rule and CC match for this pattern.
	/** When the same pattern is used in more than one rule's CC,
	 * methods like partialReactivity will need to call this first.	 */
	virtual void selectRule(int rid,small_id cc) const;
	/// Calculates the second momentum of this container.
	/** Explanation will be here */
	virtual FL_TYPE getM2() const;

	/// Return the sum of an expression applied to each embedding */
	virtual FL_TYPE sumInternal(const expressions::BaseExpression* aux_func,
			const simulation::SimContext& context) const;
	//vector<CcInjection*>::iterator begin();
	//vector<CcInjection*>::iterator end();

	/// Applies \param func to each element in this container.
	virtual void fold(const function<void (const Injection*)> func) const = 0;
};

/** The most basic type of container for Injections.
 * It uses a vector<InjType> to store injections. */
template <typename InjType>
class InjRandSet : public InjRandContainer<InjType> {
	size_t counter;					///> Count of embeddings in the container.
	size_t multiCount;				///> Count of Injections to multinodes.
	vector<InjType*> container;		///> Stored Injections.

	/// Insert a new injection in vector container.
	list<pair<const BE*,FL_TYPE>>& insert(InjType* inj,const state::State& state) override;
	/// Returns a new InjType for this container.
	virtual InjType* newInj() const override;
public:
	InjRandSet(const pattern::Pattern& _cc);
	~InjRandSet();
	/// Choose and returns a random injection.
	InjType& chooseRandom(RNG& randGen) const override;
	/// Returns the id-th injection.
	InjType& choose(unsigned id) const override;
	/// Returns the number of embeddings.
	size_t count() const override;
	/// Returns the number of embeddings. (cause every Inj. has partial reactivity = 1)
	virtual FL_TYPE partialReactivity() const;

	/// Deletes the injection inj.
	void del(Injection* inj) override;
	virtual void clear() override;

	virtual void fold(const function<void (const Injection*)> func) const;
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



/// This container is used when CC pattern holds a weight that depends on its internal values.
/** LHS patterns depending on internal values or used in rules where rate is in function
 * of internal values may need to use this container to store injection.   */
template <typename InjType>
class InjRandTree : public InjRandContainer<InjType> {
	static const float MAX_INVALIDATIONS;	///> Max invalidations (insert,delete) allowed for M2. Set to 3%.
	list<InjType*> infList;

	/** \brief A root of the rule-rate associated with a CC	 */
	struct Root {
		/** a tree structure containing injs ordered by partial-rate value */
		distribution_tree::DistributionTree<CcValueInj>* tree;
		/** the partial-rate of the CC in the rule. */
		const expressions::BaseExpression* partial_rate;
		/** second-moment of the injs collection, useful for approximations*/
		two<FL_TYPE> second_moment;//(sum x*x,sum x) at validation
		bool is_valid;		///> false when second moment need to be recalculated.

		Root(const expressions::BaseExpression* rate);
	};
	unsigned counter;

	// (r_id,cc_index) -> contribution to rule-rate per cc
	map<pair<int,small_id>,Root*> roots,roots_to_push;

	mutable int invalidations; ///> Count insert and deletes since last validation.

	mutable pair<int,small_id> selected_root;	///> Rule and CC selected to operate.

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


/// Container user for lazy initialization, useful for speedup.
/** This container will be destroyed and replaced as soon some
 * method need to access to its injections.			 */
template <typename InjType>
class InjLazyContainer : public InjRandContainer<InjType> {
	const state::State& state;			///> Used to replace this container.
	const state::SiteGraph& graph;		///> used to replace this container.
	mutable InjRandContainer<InjType> **holder;		///> Address of the pointer holding this container for replacement.
	mutable bool loaded;				///> True when replacement is already done.

	void loadContainer() const;			///> Replace container and fill it with injections to graph.
	void loadEmptyContainer() const;	///> Replace with an empty container.


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
