/*
 * Node.h
 *
 *  Created on: Oct 18, 2017
 *      Author: naxo
 */

#ifndef SRC_STATE_NODE_H_
#define SRC_STATE_NODE_H_

#include <utility>
#include <map>
#include <queue>
#include <unordered_set>
#include "../pattern/Signature.h"
#include "../data_structs/SimpleSet.h"
#include "../pattern/mixture/Mixture.h"
#include "../expressions/Vars.h"
#include "../pattern/Action.h"
#include "../matching/Injection.h"
//#include <boost/fusion/container/set.hpp>
//#include <boost/fusion/include/set.hpp>
//#include <boost/fusion/container/set/set_fwd.hpp>
//#include <boost/fusion/include/set_fwd.hpp>


/*namespace matching {
class Injection;
class InjRandContainer;
class InjSet;
}*/

//typedef data_structs::SimpleSet<matching::Injection*> InjSet;
//typedef unordered_set<matching::Injection*> InjSet;
//typedef boost::container::set<matching::Injection*> InjSet;

namespace state {

class SiteGraph;
struct EventInfo;
class Node;
//class State;

using namespace std;
using namespace expressions;
using InjSet = matching::InjSet;


class InternalState {
protected:
	static InjSet* EMPTY_INJSET;
	mutable two<InjSet*> deps;	///< (Value-Injections,Bind-Injections). (pointers to accept forward declaration of InjSet)

public:
	InternalState(InjSet*,InjSet*);
	static InternalState* createState(char type);
	static void negativeUpdate(EventInfo& ev,matching::InjRandContainer** injs,InjSet* deps,const State& state);

	virtual ~InternalState();

	virtual InternalState* clone(const map<Node*,Node*>& mask) const = 0;

	virtual void setValue(SomeValue val);
	virtual SomeValue getValue() const;

	virtual void setLink(Node* n,small_id i);
	virtual pair<Node*,small_id> getLink() const;

	inline two<InjSet*>& getDeps() const { return deps; }

	virtual void negativeUpdateByValue(EventInfo& ev,matching::InjRandContainer** injs,const State& state);
	virtual void negativeUpdateByBind(EventInfo& ev,matching::InjRandContainer** injs,const State& state);
	virtual void negativeUpdate(EventInfo& ev,matching::InjRandContainer** injs,const State& state) = 0;


	virtual string toString(const pattern::Signature::Site& s,bool show_binds = false,map<const Node*,bool> *visit = nullptr) const = 0;

};


class Node {
protected:
	friend struct InternalState;
	short_id signId;
	big_id address;
	InternalState* *interface;
	small_id intfSize;
	InjSet* deps;

	Node(const Node& node) = delete;
	Node& operator=(const Node& node) = delete;
public:
	Node();
	Node(const pattern::Signature& sign);

	/** \brief The copy constructor with a mask of nodes for bindings.
	 *
	 * The mask*/
	Node(const Node& node,const map<Node*,Node*>& mask);
	virtual ~Node();

	void copyDeps(const Node& node,EventInfo& ev,matching::InjRandContainer** injs,
			const expressions::EvalArgs& args);//unsafe
	void alloc(big_id addr);
	big_id getAddress() const;

	/** \brief static vector with the basic rule action methods.
	 */
	static void (Node::*applyAction[6])(State&);
	virtual void removeFrom(State& state);
	virtual void changeIntState(State& state);
	virtual void assign(State& state);
	virtual void unbind(State& state);
	virtual void bind(State& state);


	//inline?? TODO
	InternalState** begin();
	InternalState** end();

	void setCount(pop_size q);
	virtual pop_size getCount() const;

	//INLINE METHODS
	inline short_id getId() const {return signId;};
	inline void addDep(matching::Injection* inj){
		deps->emplace(inj);
	}
	inline bool removeDep(matching::Injection* inj){
		return deps->erase(inj);
	}
	//unsafe
	inline const InternalState* getInternalState(small_id st) const{
		return interface[st];
	}
	inline SomeValue getInternalValue(small_id st) const{
		return interface[st]->getValue();
	}
	inline pair<Node*,small_id> getInternalLink(small_id st) const{
		return interface[st]->getLink();
	}
	inline void setInternalValue(small_id st,SomeValue val){
		interface[st]->setValue(val);
	}
	inline void setInternalLink(small_id st,Node* lnk,small_id site_trgt){
		interface[st]->setLink(lnk,site_trgt);
	}
	inline two<InjSet*>& getLifts(small_id st){
		return interface[st]->getDeps();
	}
	//DEBUG
	virtual string toString(const pattern::Environment &env,bool show_binds = false,map<const Node*,bool> *visit = nullptr) const;

};

//class ValueState;
//class BindState;
//class ValueBindState;

class ValueState : public virtual InternalState {
protected:
	SomeValue value;
public:
	ValueState();
	virtual ~ValueState();
	virtual InternalState* clone(const map<Node*,Node*>& mask) const override;
	virtual void setValue(SomeValue val) override;
	virtual SomeValue getValue() const override;

	virtual void negativeUpdateByValue(EventInfo& ev,matching::InjRandContainer** injs,const State& state) override;
	virtual void negativeUpdate(EventInfo& ev,matching::InjRandContainer** injs,const State& state) override;

	virtual string toString(const pattern::Signature::Site& s,bool show_binds = false,map<const Node*,bool> *visit = nullptr) const override;
};

class BindState : public virtual InternalState {
protected:
	pair<Node*,small_id> target;	///< Target node and site number
public:
	BindState();
	virtual ~BindState();
	virtual InternalState* clone(const map<Node*,Node*>& mask) const override;
	virtual void setLink(Node* n,small_id i) override;
	virtual pair<Node*,small_id> getLink() const override;

	virtual void negativeUpdateByBind(EventInfo& ev,matching::InjRandContainer** injs,const State& state) override;
	virtual void negativeUpdate(EventInfo& ev,matching::InjRandContainer** injs,const State& state) override;

	virtual string toString(const pattern::Signature::Site& s,bool show_binds = false,map<const Node*,bool> *visit = nullptr) const override;
};

class ValueBindState : public ValueState, public BindState {
public:
	ValueBindState();

	virtual InternalState* clone(const map<Node*,Node*>& mask) const override;

	virtual void negativeUpdate(EventInfo& ev,matching::InjRandContainer** injs,const State& state) override;

	virtual string toString(const pattern::Signature::Site& s,bool show_binds = false,map<const Node*,bool> *visit = nullptr) const override;
};

class MultiNode;

class SubNode : public Node {
	MultiNode& cc;
	small_id multiId;
public:
	SubNode(const pattern::Signature& sign,MultiNode& multi);
	~SubNode();
	pop_size getCount() const override;


	void removeFrom(State& state) override;
	void changeIntState(State& state) override;
	void assign(State& state) override;
	void unbind(State& state) override;
	void bind(State& state) override;

	/*bool test(const pair<small_id,pattern::Mixture::Site>& id_site,
			std::queue<pair<small_id,Node&> > &match_queue,
			two<list<Internal*> > &port_list) const;*/

	string toString(const pattern::Environment& env,bool show_binds = false,map<const Node*,bool> *visit = nullptr) const override;
};

class MultiNode {
	pop_size population;

	SubNode** cc;
	small_id count;

	~MultiNode();
public:
	MultiNode(unsigned n,pop_size pop);

	pop_size getPopulation() const;

	small_id add(SubNode* node);
	void remove();
	void dec(EventInfo& ev);

	class OutOfInstances : exception {};
};


/** \brief Structure that stores the information related to an event.
 */
struct EventInfo {
	pattern::Action act;
	//map of emb LHS [cc_id][ag_id]
	vector<Node*>* emb;
	small_id cc_count;
	//map of new_nodes RHS
	//vector<Node*> fresh_emb;
	//node_address,site_id
	std::multimap<Node*,small_id> side_effects;
	//perturbation_ids
	std::set<mid_id> pert_ids;
	//null events
	//int warns;
	multimap<const Node*,int> null_actions;

	/** \brief new cc derived from multinode or used in insert new nodes.
	 *
	 * [n -> n] means the node was erased.
	 * [n -> null] means the new node is not created yet.*/
	map<Node*,Node*> new_cc;
	//mask for new injections, nullptr are erased injs
	map<matching::Injection*,matching::Injection*> inj_mask;
	//aux_values
	AuxMixEmb aux_map;

	set<const pattern::Pattern*> to_update;
	set<small_id> rule_ids;

	EventInfo();
	~EventInfo();

	void clear();

};

} /* namespace state */

#endif /* SRC_STATE_NODE_H_ */
