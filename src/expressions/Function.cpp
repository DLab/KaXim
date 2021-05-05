/*
 * Function.cpp
 *
 *  Created on: Dec 30, 2019
 *      Author: naxo100
 */


#include <iostream>
#include <sstream>
#include <string>

#include "Function.h"
#include "Constant.h"
#include "../state/State.h"

#include <random>
#include <boost/math/special_functions/beta.hpp>


namespace sftrabbit {

  template <typename RealType = double>
  class beta_distribution
  {
    public:
      typedef RealType result_type;

      class param_type
      {
        public:
          typedef beta_distribution distribution_type;

          explicit param_type(RealType a = 2.0, RealType b = 2.0)
            : a_param(a), b_param(b) { }

          RealType a() const { return a_param; }
          RealType b() const { return b_param; }

          bool operator==(const param_type& other) const
          {
            return (a_param == other.a_param &&
                    b_param == other.b_param);
          }

          bool operator!=(const param_type& other) const
          {
            return !(*this == other);
          }

        private:
          RealType a_param, b_param;
      };

      explicit beta_distribution(RealType a = 2.0, RealType b = 2.0)
        : a_gamma(a), b_gamma(b) { }
      explicit beta_distribution(const param_type& param)
        : a_gamma(param.a()), b_gamma(param.b()) { }

      void reset() { }

      param_type param() const
      {
        return param_type(a(), b());
      }

      void param(const param_type& param)
      {
        a_gamma = gamma_dist_type(param.a());
        b_gamma = gamma_dist_type(param.b());
      }

      template <typename URNG>
      result_type operator()(URNG& engine)
      {
        return generate(engine, a_gamma, b_gamma);
      }

      template <typename URNG>
      result_type operator()(URNG& engine, const param_type& param)
      {
        gamma_dist_type a_param_gamma(param.a()),
                        b_param_gamma(param.b());
        return generate(engine, a_param_gamma, b_param_gamma);
      }

      result_type min() const { return 0.0; }
      result_type max() const { return 1.0; }

      result_type a() const { return a_gamma.alpha(); }
      result_type b() const { return b_gamma.alpha(); }

      bool operator==(const beta_distribution<result_type>& other) const
      {
        return (param() == other.param() &&
                a_gamma == other.a_gamma &&
                b_gamma == other.b_gamma);
      }

      bool operator!=(const beta_distribution<result_type>& other) const
      {
        return !(*this == other);
      }

    private:
      typedef std::gamma_distribution<result_type> gamma_dist_type;

      gamma_dist_type a_gamma, b_gamma;

      template <typename URNG>
      result_type generate(URNG& engine,
        gamma_dist_type& x_gamma,
        gamma_dist_type& y_gamma)
      {
        result_type x = x_gamma(engine);
        return x / (x + y_gamma(engine));
      }
  };

  template <typename CharT, typename RealType>
  std::basic_ostream<CharT>& operator<<(std::basic_ostream<CharT>& os,
    const beta_distribution<RealType>& beta)
  {
    os << "~Beta(" << beta.a() << "," << beta.b() << ")";
    return os;
  }

  template <typename CharT, typename RealType>
  std::basic_istream<CharT>& operator>>(std::basic_istream<CharT>& is,
    beta_distribution<RealType>& beta)
  {
    std::string str;
    RealType a, b;
    if (std::getline(is, str, '(') && str == "~Beta" &&
        is >> a && is.get() == ',' && is >> b && is.get() == ')') {
      beta = beta_distribution<RealType>(a, b);
    } else {
      is.setstate(std::ios::failbit);
    }
    return is;
  }

}










namespace expressions {

using namespace std;

template<typename T>
T (*FunctionPointers<T>::funcs[1])(const vector<SomeValue>&,const SimContext&)= {
	[](const vector<SomeValue>& args,const SimContext& context) {
		//static std::uniform_real_distribution<double> unif(0,1);
		//double p = unif(state.getRandomGenerator());
		//return boost::math::ibeta_inv(args[0].valueAs<FL_TYPE>(), args[1].valueAs<FL_TYPE>(), p);
		sftrabbit::beta_distribution<> beta(args[0].valueAs<FL_TYPE>(), args[1].valueAs<FL_TYPE>());
		//std::cout << beta << endl;
		return beta(context.getRandomGenerator());
	}, // Beta
};





template <typename T>
Function<T>::Function(BaseExpression::Funcs f,const list<BaseExpression*>& l)
		: f_name(f),func(FunctionPointers<T>::funcs[f]),args(l){}

template <typename T>
Function<T>::~Function(){
	for(auto arg : args)
		delete arg;
}

template <typename T>
T Function<T>::evaluate(const SimContext& context) const {
	vector<SomeValue> v;
	v.reserve(args.size());
	for(auto arg : args)
		v.push_back(arg->getValue(context));
	return func(v,context);
}
template <typename T>
T Function<T>::evaluateSafe(const SimContext& context) const {
	vector<SomeValue> v;
	v.reserve(args.size());
	for(auto arg : args)
		v.push_back(arg->getValue(context));
	return func(v,context);
}

template <typename T>
FL_TYPE Function<T>::auxFactors(unordered_map<string, FL_TYPE> &factor) const{
	return 0.0;
}

template <typename T>
BaseExpression::Reduction Function<T>::factorize(const map<string,small_id> &aux_cc) const{
	using Unfactorizable = BaseExpression::Unfactorizable;
	//auto VARDEP = BaseExpression::VarDep::VARDEP;
	//auto MULT = BaseExpression::AlgebraicOp::MULT;
	//auto make_binary = BaseExpression::makeBinaryExpression<false>;
	//auto make_unary = BaseExpression::makeUnaryExpression;
	BaseExpression::Reduction res;

	list<BaseExpression*> clones;
	set<small_id> ccs;
	for(auto arg : args){
		BaseExpression::Reduction r(arg->factorize(aux_cc));
		clones.push_back(arg->clone());
		switch(f_name){
		case BaseExpression::BETA:
			//TODO
			for(auto& elem : r.aux_functions)
				ccs.emplace(elem.first);
			if(ccs.size() > 1)
				throw Unfactorizable("Cannot factorize: Applying Beta"+
						string("f_name")+" to more than one cc-aux function.");
			if(ccs.size() == 0)
				res.factor = new Function<T>(f_name,clones);
			else{ //(1 cc_aux)
				res.factor = ONE_FL_EXPR->clone();
				res.aux_functions[*ccs.begin()] = new Function<T>(f_name,clones);
			}
			break;
		}
	}
	return res;
}

template <typename T>
BaseExpression* Function<T>::clone() const{
	list<BaseExpression*> clones;
	for(auto arg : args)
		clones.push_back(arg->clone());
	return new Function<T>(f_name,clones);
}

template <typename T>
BaseExpression* Function<T>::reduce(SimContext& context){
	switch(f_name){
	case BaseExpression::BETA:
		for(auto& arg : args){
			auto r = arg->reduce(context);
			if(r != arg)
				delete arg;
			arg = r;
		}
		break;
	}
	return this;
}

//std::set<std::string> getAuxiliars() const override;
template <typename T>
bool Function<T>::operator==(const BaseExpression& expr) const{
	try {
		auto& f = dynamic_cast<const Function<T>&>(expr);
		if (f.f_name == f_name){
			return f.args == args;
		}
	} catch (bad_cast &ex) {
	}
	return false;
}

/*template <typename T>
void Function<T>::getNeutralAuxMap(
		std::unordered_map<std::string, FL_TYPE>& aux_map) const{

}*/

//return a flag of VarDep
template <typename T>
char Function<T>::getVarDeps() const{
	char vd = 0;
	for(auto arg : args)
		vd |= arg->getVarDeps();
	return vd;
}

template <typename T>
string Function<T>::toString() const{
	return string("function TODO!!");
}


template class FunctionPointers<FL_TYPE>;
template class Function<FL_TYPE>;

} /* namespace pattern */
