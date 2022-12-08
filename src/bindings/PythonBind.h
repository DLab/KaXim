/*
 * PythonBind.h
 *
 *  Created on: Oct 18, 2021
 *      Author: naxo
 */

#ifndef SRC_BINDINGS_PYTHONBIND_H_
#define SRC_BINDINGS_PYTHONBIND_H_

#include <map>

namespace simulation { class Results;}

/** Functions and classes to bind with python library.
 *
 */
namespace binds {

/** Runs simulations using given parameters, like in KaSim comand-line.
 * Used to bind with python library.
 * \param args string to string map (dict) with param. name -> value, exactly as it was called
 * from command-line.
 * \param ka_params string to float map (dict) with the values of kappa parameters
 * (from the kappa model). These values will overwrite other values given in the model
 * or in args.
 * \return A simulation::Result object containing the data of the simulation. */
simulation::Results& run_kappa_model(std::map<std::string,std::string> args,
		std::map<std::string,float> ka_params = std::map<std::string,float>());

} /* namespace matching */

#endif /* SRC_BINDINGS_PYTHONBIND_H_ */
