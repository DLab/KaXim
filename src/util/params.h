
#ifndef PARAMS_H
#define PARAMS_H

//#define DEBUG true //in make command

#include <utility>
#include <stdexcept>
template<typename T> using two = std::pair<T,T>;

typedef double FL_TYPE;
typedef long INT_TYPE;
typedef unsigned long UINT_TYPE;
#define FRMT_FL "f"
#define FRMT_UINT "lu"

typedef unsigned long big_id;
typedef unsigned int mid_id;
typedef unsigned short short_id;
typedef unsigned char small_id;
typedef two<small_id> ag_st_id;

typedef unsigned int pop_size;

#include <random>
typedef std::mt19937_64 RNG;
//#include <boost/random/mersenne_twister.hpp>
//typedef boost::mt19937_64 RNG;


#include <string>
class False : public std::exception {
	virtual const char* what() const _GLIBCXX_USE_NOEXCEPT override {return "False exception";}
};


#endif
