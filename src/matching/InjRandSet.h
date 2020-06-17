/*
 * InjRandSet.h
 *
 *  Created on: Jun 26, 2018
 *      Author: naxo100
 */

#ifndef SRC_MATCHING_INJRANDSET_H_
#define SRC_MATCHING_INJRANDSET_H_

#include "Injection.h"
#include "../data_structs/DistributionTree.h"
#include "../simulation/Rule.h"

//#include <unordered_map>


namespace matching {

//using Node = state::Node;

class InjRandContainer {
protected:
	list<CcInjection*> freeInjs;
	const pattern::Pattern::Component& cc;
	virtual void insert(CcInjection* inj,const expressions::EvalArgs& args) = 0;
	virtual void del(Injection* inj) = 0;
	virtual CcInjection* newInj() const = 0;

	mutable unordered_map<const expressions::BaseExpression*,FL_TYPE&> trackedSums;
	mutable list<pair<const expressions::BaseExpression*,FL_TYPE>> sumList;
public:

	InjRandContainer(const pattern::Pattern::Component& _cc);
	virtual ~InjRandContainer();
	virtual const Injection& chooseRandom(RNG& randGen) const = 0;
	virtual const Injection& choose(unsigned id) const  = 0;
	virtual size_t count() const = 0;
	virtual FL_TYPE partialReactivity() const = 0;

	virtual Injection* emplace(Node& node,map<int,InjSet*> &port_list,
			const expressions::EvalArgs& args,small_id root = 0);
	virtual Injection* emplace(Injection* base_inj,map<Node*,Node*>& mask,
			const expressions::EvalArgs& args);
	inline virtual void erase(Injection* inj,const expressions::EvalArgs& args);
	virtual void clear() = 0;

	virtual void selectRule(int rid,small_id cc) const;
	virtual FL_TYPE getM2() const;

	virtual FL_TYPE sumInternal(const expressions::BaseExpression* aux_func,
			const expressions::EvalArgs& args) const;
	//vector<CcInjection*>::iterator begin();
	//vector<CcInjection*>::iterator end();


	virtual void fold(const function<void (const Injection*)> func) const = 0;
};


class InjRandSet : public InjRandContainer {
	size_t counter;
	size_t multiCount;
	vector<CcInjection*> container;
	void insert(CcInjection* inj,const expressions::EvalArgs& args) override;
	virtual CcInjection* newInj() const override;
public:
	InjRandSet(const pattern::Pattern::Component& _cc);
	~InjRandSet();
	const Injection& chooseRandom(RNG& randGen) const override;
	const Injection& choose(unsigned id) const override;
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


class InjRandTree : public InjRandContainer {
	static const float MAX_INVALIDATIONS;//3%
	//list<FL_TYPE> hints;

	//list<CcInjection*> freeInjs;
	list<CcInjection*> infList;

	struct Root {
		distribution_tree::DistributionTree<CcValueInj>* tree;
		two<FL_TYPE> second_moment;//(sum x*x,sum x) at validation
		bool is_valid;

		Root();
	};
	unsigned counter;
	// (r_id,cc_index) -> contribution to rule-rate per cc
	map<pair<int,small_id>,Root*> roots,roots_to_push;
	mutable int invalidations;

	mutable pair<int,small_id> selected_root;

	void insert(CcInjection *inj,const expressions::EvalArgs& args) override;
	virtual CcInjection* newInj() const override;

public:
	InjRandTree(const pattern::Pattern::Component& cc,const vector<simulation::Rule::Rate*>& rates);
	~InjRandTree();
	const Injection& chooseRandom(RNG& randGen) const override;
	const Injection& choose(unsigned id) const override;
	size_t count() const override;
	virtual FL_TYPE partialReactivity() const override;

	FL_TYPE getM2() const override;

	void del(Injection* inj) override;
	virtual void clear() override;


	void selectRule(int rid,small_id cc) const override;

	FL_TYPE sumInternal(const expressions::BaseExpression* aux_func,
				const expressions::EvalArgs& args) const override;
	virtual void fold(const function<void (const Injection*)> func) const;
	//void addHint(FL_TYPE value);
};



class InjLazyContainer : public InjRandContainer {
	const state::State& state;
	const state::SiteGraph& graph;
	mutable InjRandContainer **holder;
	mutable bool loaded;

	void loadContainer() const;


	virtual void insert(CcInjection* inj,const expressions::EvalArgs& args) override;
	virtual CcInjection* newInj() const override;

public:
	InjLazyContainer(const pattern::Mixture::Component& cc,const state::State& state,
			const state::SiteGraph& graph,InjRandContainer *&holder);
	~InjLazyContainer();

	virtual const Injection& chooseRandom(RNG& randGen) const override;
	virtual const Injection& choose(unsigned id) const override;
	virtual size_t count() const override;
	virtual FL_TYPE partialReactivity() const override;

	virtual Injection* emplace(Node& node,map<int,InjSet*> &port_lists,
			const expressions::EvalArgs& args,small_id root = 0) override;
	virtual Injection* emplace(Injection* base_inj,map<Node*,Node*>& mask,
			const expressions::EvalArgs& args) override;
	virtual void erase(Injection* inj,const expressions::EvalArgs& args) override;
	virtual void del(Injection* inj) override;
	virtual void clear() override;

	virtual void selectRule(int rid,small_id cc) const override;
	virtual FL_TYPE getM2() const override;

	virtual FL_TYPE sumInternal(const expressions::BaseExpression* aux_func,
			const expressions::EvalArgs& args) const override;

	virtual void fold(const function<void (const Injection*)> func) const override;
};



inline void InjRandContainer::erase(Injection* inj,const expressions::EvalArgs& args){
	for(auto& s : sumList)
		s.second -= inj->evalAuxExpr(args,s.first);
	del(inj);
	freeInjs.push_back(static_cast<CcInjection*>(inj));
	inj->dealloc();//dealloc
}


}

#endif /* SRC_MATCHING_INJRANDSET_H_ */
