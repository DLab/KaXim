
/** \file */

#ifndef PARAMS_H
#define PARAMS_H

//#define DEBUG true //in make command

#include <utility>
#include <stdexcept>
template<typename T> using two = std::pair<T,T>;	///< pair<T,T>

typedef double FL_TYPE;		///< The float type used for the whole simulation. Using float could speedup, but make approximation errors.
typedef long INT_TYPE;
typedef unsigned long UINT_TYPE;
#define FRMT_FL "d"
#define FRMT_UINT "lu"

typedef unsigned long big_id;
typedef unsigned int mid_id;
typedef unsigned short short_id;
typedef unsigned char small_id;
typedef two<small_id> ag_st_id;

typedef unsigned int pop_size;

#include <random>
typedef std::mt19937_64 RNG;		///<
//#include <boost/random/mersenne_twister.hpp>
//typedef boost::mt19937_64 RNG;


#include <string>
class False : public std::exception {
	virtual const char* what() const _GLIBCXX_USE_NOEXCEPT override {return "False exception";}
};


//Debugging shortcuts
#ifdef DEBUG
	#define IF_DEBUG_LVL(lvl,to_do) \
		if(params->verbose >= lvl) { to_do; }
#else
	#define IF_DEBUG_LVL(lvl,to_do) /* do not debug */
#endif

#ifdef DEBUG
	#define DEBUG_LOG(lvl,id,msg) \
		if(simulation::Parameters::get().verbose >= lvl)\
			std::clog << "[Sim " << id << "]: " << msg << std::endl;
#else
	#define DEBUG_LOG(lvl,id,msg) /*do not log*/
#endif

#endif //params.h

