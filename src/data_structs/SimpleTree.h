/*
 * SimpleTree.h
 *
 *  Created on: Sep 21, 2021
 *      Author: naxo
 */

#ifndef SRC_DATA_STRUCTS_SIMPLETREE_H_
#define SRC_DATA_STRUCTS_SIMPLETREE_H_

#include "RandomTree.h"

namespace data_structs {

class SimpleTree: public RandomTree {
	int size;
	FL_TYPE* acts;
	FL_TYPE sum;
public:
	SimpleTree(int n);
	virtual ~SimpleTree();

	/** Update all the nodes.
	*/
	virtual void update() {};

	/** \brief Number of nodes of the Chain.
	 */
	virtual int getSize() const {
		return size;
	}

	/** \brief Calculate total sum of nodes.
	 */
	virtual FL_TYPE total() {
		return sum;
	}

	/** \brief Add or change a value of a node in the chain.
	 */
	virtual void add(int id, FL_TYPE val){
		sum += val-acts[id];
		acts[id] = val;
	}

	/** \brief Choose random node from tree with probability = weight/total().
	 */
	virtual std::pair<int,FL_TYPE> chooseRandom();

	/** \brief Check if the rule "i" has an infinity probability.
	*/
	virtual bool isInfinite(int i) {return false;};

	/** \brief Return the probability of the rule "i"
	*/
	virtual FL_TYPE find(int i) {
		return acts[i];
	}
};


}

#endif /* SRC_DATA_STRUCTS_SIMPLETREE_H_ */
