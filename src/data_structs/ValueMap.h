/*
 * AuxValueMap.h
 *
 *  Created on: Dec 10, 2020
 *      Author: naxo100
 */

#ifndef SRC_DATA_STRUCTS_VALUEMAP_H_
#define SRC_DATA_STRUCTS_VALUEMAP_H_

#include <unordered_map>
#include <vector>


/** \brief Class used to map aux to values in different ways */
template <template<typename> class M,typename V>
class ValueMap {
public:
	virtual ~ValueMap() {};
	virtual V& operator[](const M<V>& a) = 0;
	virtual V at(const M<V>& a) const = 0;
	virtual void clear() = 0;
	virtual size_t size() const = 0;
	virtual std::string toString() const = 0;
};



template <template<typename> class M,typename V>
class CoordsMap : public ValueMap<M,V> {
	std::unordered_map<int,V> m;
public:
	V& operator[](const M<V>& a) override {
		auto coords(a.getCoords());
		return m.std::unordered_map<int,V>::operator [](coords.ag_pos+coords.st_id*sizeof(small_id));
	}
	V& operator[](const two<small_id>& ag_st) {
		return m.std::unordered_map<int,V>::operator [](ag_st.first+ag_st.second*sizeof(small_id));
	}
	V at(const M<V>& a) const override {
		auto coords(a.getCoords());
		return m.std::unordered_map<int,V>::at(coords.ag_pos+coords.cc_pos*sizeof(small_id));
	}
	void clear() override {
		m.std::unordered_map<int,V>::clear();
	}
	size_t size() const {
		return m.std::unordered_map<int,V>::size();
	}
	std::string toString() const {
		std::string ret("CoordsMap {\n");
		int n = sizeof(small_id);
		for(auto& var : m)
			std::cout << "\t(" << var.first / n << "," << var.first % n << ") -> " << var.second << " ,\n";
		return ret + "}";
	}
};


template <template<typename> class M,typename V>
class NamesMap : public ValueMap<M,V> {
	std::unordered_map<std::string,V> m;
public:
	V& operator[](const M<V>& a) override {
		return m.std::unordered_map<std::string,V>::operator [](a.toString());
	}
	V& operator[](const std::string &s){
		return m.std::unordered_map<std::string,V>::operator [](s);
	}
	V at(const M<V>& a) const override {
		return m.std::unordered_map<std::string,V>::at(a.toString());
	}
	V at(const std::string& s) const {
		return m.std::unordered_map<std::string,V>::at(s);
	}
	void clear() override {
		m.std::unordered_map<std::string,V>::clear();
	}
	size_t size() const {
		return m.std::unordered_map<std::string,V>::size();
	}
	auto begin() const {
		return m.begin();
	}
	auto end() const {
		return m.end();
	}

	std::string toString() const {
		std::string ret("NamesMap {\n");
		int n = sizeof(small_id);
		for(auto& var : m)
			std::cout << "\t" << var.first << " -> " << var.second << " ,\n";
		return ret + "}";
	}
};


template <template<typename> class M,typename V,class N>
class CcEmbMap : public ValueMap<M,V> {
	const std::vector<N*>* emb;
public:
	CcEmbMap(): emb(nullptr){}
	CcEmbMap(const std::vector<N*>& _emb) : emb(&_emb) {}
	void setEmb(const std::vector<N*>& _emb) {
		emb = &_emb;
	}
	V& operator[](const M<V>& a) override {
		throw std::invalid_argument("Cannot call [] on AuxEmb.");
	}
	V at(const M<V>& a) const override {
		//const pattern::Mixture::AuxCoord coord(a.getCoords());
		#ifdef DEBUG
			//cout << "sdfsdf" << endl;
			if(emb)
				if(emb->at(a.getCoords().ag_pos)){
					if(!emb->at(a.getCoords().ag_pos)->getInternalState(a.getCoords().st_id))
						throw std::invalid_argument("AuxMixEmb::at(): not a valid site coordinate.");
				}else
					throw std::invalid_argument("AuxMixEmb::at(): agent is null in embedding.");
			else
				throw std::invalid_argument("AuxMixEmb::at(): emb is null.");
		#endif
		return emb->operator[](a.getCoords().ag_pos)->getInternalValue(a.getCoords().st_id).expressions::SomeValue::valueAs<FL_TYPE>();

	}
	void clear() override {
		//nothing for now
	}
	size_t size() const {
		std::invalid_argument("Cannot call size() on AuxEmb.");
		return 0;
	}
	std::string toString() const {
		std::string ret("CcEmbMap {\n");
		int n = sizeof(small_id);
		//for(auto& var : *emb)
		//	cout << "\t(" << var.first / n << "," << var.first % n << ") -> " << var.second << " ,\n";
		return ret + "}";
	}
};

template <template<typename> class M,typename V,class N>
class MixEmbMap : public ValueMap<M,V> {
	const std::vector<N*>* emb;
public:
	MixEmbMap() : emb(nullptr){}
	MixEmbMap(const std::vector<N*>* _emb) : emb(_emb) {}
	void setEmb(const std::vector<N*>* _emb) {
		emb = _emb;
	}
	V& operator[](const M<V>& a) override {
		throw std::invalid_argument("Cannot call [] on AuxEmb.");
	}

	V at(const M<V>& a) const override {
		auto coord(a.getCoords());
#ifdef DEBUG
		if(emb)
			if(emb[coord.cc_pos].size())
				if(emb[coord.cc_pos].at(coord.ag_pos)){
					if(!emb[coord.cc_pos].at(coord.ag_pos)->getInternalState(coord.st_id))
						throw std::invalid_argument("AuxMixEmb::at(): not a valid site coordinate.");
				}else
					throw std::invalid_argument("AuxMixEmb::at(): agent is null in embedding.");
			else
				throw std::invalid_argument("AuxMixEmb::at(): CC is null in embedding.");
		else
			throw std::invalid_argument("AuxMixEmb::at(): embedding is null.");
#endif
		return emb[coord.cc_pos][coord.ag_pos]->
				getInternalState(coord.st_id)->
				getValue().
				expressions::SomeValue::valueAs<V>();

	}

	void clear() override {
		emb = nullptr;
	}
	size_t size() const {
		std::invalid_argument("Cannot call size() on AuxEmb.");
		return 0;
	}
	std::string toString() const {
		std::string ret("MixEmbMap {\n");
		int n = sizeof(small_id);
		//for(auto& var : m)
		//	cout << "\t(" << var.first / n << "," << var.first % n << ") -> " << var.second << " ,\n";
		return ret + "}";
	}
};


#endif /* SRC_DATA_STRUCTS_VALUEMAP_H_ */
