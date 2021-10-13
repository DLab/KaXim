/*
 * SimpleTree.cpp
 *
 *  Created on: Sep 21, 2021
 *      Author: naxo
 */

#include "SimpleTree.h"


namespace data_structs {

SimpleTree::SimpleTree(int n) : size(n) {
	acts = new FL_TYPE[n];
	sum = 0;
}

SimpleTree::~SimpleTree() {
	delete[] acts;
}

/** \brief Choose random node from tree with probability = weight/total().
 */
std::pair<int,FL_TYPE> SimpleTree::chooseRandom() {
	FL_TYPE select = rand() * sum / RAND_MAX;
	//static int rnd(0);
	//FL_TYPE select = (rnd++)%100 * sum / 100.0;
	int i = 0;
	do {
		if(select < acts[i])
			return std::make_pair(i,acts[i]);
		select -= acts[i];
		i++;
	} while(i < size);
	throw std::invalid_argument("at SimpleTree::chooseRandom()");
}

}

