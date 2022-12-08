/*
 * distributionTreeTest.cpp
 *
 *  Created on: Dec 5, 2022
 *      Author: naxo
 */

//#include <gtest/gtest.h>
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE MatchingSuite
#include <boost/test/unit_test.hpp>
#include "../../src/data_structs/DistributionTree.cpp"
//#include "../../src/matching/CcInjection.h"

namespace test {
using namespace distribution_tree;

class FakeValueInj {
	int n,address;
public:
	FakeValueInj(int _n = 1,FL_TYPE _v = 1.0) : n(_n),address(-1),value(_v) {}
	void addContainer(DistributionTree<FakeValueInj>&,int) {}
	bool removeContainer(DistributionTree<FakeValueInj>&) { return !(--address);}
	int count() { return n;}
	int getAddress() { return address; }
	void alloc() {address++;}

	FL_TYPE value;
};

//TEST(MatchingSuite,DistributionTreeTest){
BOOST_AUTO_TEST_CASE( LeafTest ) {
	auto node = new Node<FakeValueInj>(5.0);
	Leaf<FakeValueInj> leaf(node);		//fix tree1 as parent node, nothing more

	// Testing constructor and init
	BOOST_REQUIRE_NE(node,nullptr);
	BOOST_CHECK_EQUAL(node->count(),0);
	BOOST_CHECK_EQUAL(leaf.count(),0);
	BOOST_CHECK_EQUAL(leaf.getLevel(),1);
	BOOST_CHECK_EQUAL(leaf.treeHeight(),2);

	// Testing modifiers
	FakeValueInj first(1,0.5),mid[10],last(1,0.8),multi(100,0.1);

	leaf.push(&first,first.value);
	BOOST_CHECK_EQUAL(leaf.count(),1);
	int mcap = Leaf<FakeValueInj>::MAX_LVL0 << leaf.getLevel();
	for(int i = 0; i < mcap-1; i++){
		leaf.push(&mid[i],mid[i].value);
		BOOST_CHECK_EQUAL(leaf.count(),i+2);
	}
	BOOST_CHECK_THROW(leaf.push(&last,last.value),std::bad_alloc);
	BOOST_CHECK_THROW(leaf.erase(mcap),std::out_of_range);

	// Testing access
	BOOST_CHECK_EQUAL(leaf.total(),0.5 + 1.0*(mcap-1));
	BOOST_CHECK_EQUAL(leaf.choose(unsigned(0)).first,&first);
	BOOST_CHECK_EQUAL(leaf.choose(unsigned(2)).first,mid+1);
	BOOST_CHECK_EQUAL(leaf.choose(unsigned(mcap-1)).first,mid+mcap-2);
	BOOST_CHECK_THROW(leaf.choose(unsigned(mcap)),std::out_of_range);
	BOOST_CHECK_EQUAL(&leaf.choose(FL_TYPE(0.3)),&first);
	BOOST_CHECK_EQUAL(&leaf.choose(FL_TYPE(3.0)),&mid[2]);
	BOOST_CHECK_THROW(leaf.choose(FL_TYPE(3.5)),std::out_of_range);

	BOOST_CHECK_EQUAL(leaf.sumInternal([](const FakeValueInj* inj){return inj->value*inj->value;}),0.25+mcap-1);
	float sum = 0;
	leaf.fold([&](const FakeValueInj* inj){sum += inj->value*inj->value;});
	BOOST_CHECK_EQUAL(sum,0.25+mcap-1);
	BOOST_CHECK_EQUAL(leaf.squares(),(0.25+0.5+1)/3);

	BOOST_CHECK_THROW(leaf.erase(mcap),std::out_of_range);


	delete node;
}

}
