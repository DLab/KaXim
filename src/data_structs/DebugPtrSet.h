/*
 * SafePtrSet.h
 *
 *  Created on: Jul 7, 2021
 *      Author: naxo
 */

#ifndef SRC_DATA_STRUCTS_DEBUGPTRSET_H_
#define SRC_DATA_STRUCTS_DEBUGPTRSET_H_

#include <set>

#ifdef DEBUG
template <typename T>
class DebugPtrSet : public std::set<T*> {
public:
	inline auto emplace(T* elem) {
		for(auto elem_it : *this)
			if(*elem == *elem_it)
				throw std::invalid_argument("DebugPtrSet::emplace(): same element twice.");
		return std::set<T*>::emplace(elem);
	}
	using std::set<T*>::erase;
};

#else
template <typename T>
class DebugPtrSet : public std::set<T*>{};
#endif




#endif /* SRC_DATA_STRUCTS_DEBUGPTRSET_H_ */
