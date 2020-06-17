/*
 * SomeValue.h
 *
 *  Created on: Oct 29, 2018
 *      Author: naxo100
 */

#ifndef SRC_EXPRESSIONS_SOMEVALUE_H_
#define SRC_EXPRESSIONS_SOMEVALUE_H_

#include "../util/params.h"

namespace expressions {

enum Type {
	FLOAT, INT, BOOL, SMALL_ID, SHORT_ID, NONE
};

class SomeValue {
private:
public:
	FL_TYPE fVal;
	union {
		int iVal;
		small_id smallVal;
		short_id shortVal;
		bool bVal;
		//std::string* sVal;
	};
	Type t;

	SomeValue() : fVal(0.0),t(NONE) {}
	SomeValue(FL_TYPE f) : fVal(f), t(FLOAT) {}
	SomeValue(int i) : fVal(i), iVal(i),t(INT) {}
	SomeValue(short_id id) : fVal(id),iVal(id),t(SHORT_ID){}
	SomeValue(small_id id) : fVal(id),iVal(id), t(SMALL_ID) {}
	SomeValue(bool b) : fVal(b),iVal(b), t(BOOL) {}
	//SomeValue(const std::string &s);
	//SomeValue(const std::string *s);

	template<typename T>
	void set(T val);

	template<typename T>
	void safeSet(T val);

	template<typename T>
	T valueAs() const;

	bool operator!=(const SomeValue&) const;
	bool operator==(const SomeValue&) const;

	std::string toString() const;
	friend std::ostream& operator<<(std::ostream& out, const SomeValue& val);
};

template <>
inline FL_TYPE SomeValue::valueAs<FL_TYPE>() const {
	return fVal;
}

template<typename T>
inline T SomeValue::valueAs() const {
	switch (t) {
	case FLOAT:
		return static_cast<T>(fVal);
	case INT:
		return static_cast<T>(iVal);
	case BOOL:
		return static_cast<T>(bVal);
	default:
		throw std::invalid_argument("SomeValue::valueAs(): not sense in convert id-like values.");
	}
	return 0;
}


std::ostream& operator<<(std::ostream& out, const SomeValue& val);

}

#endif /* SRC_EXPRESSIONS_SOMEVALUE_H_ */
