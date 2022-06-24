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

namespace binds {

simulation::Results& run_kappa_model(std::map<std::string,std::string> args,
		std::map<std::string,float> ka_params = std::map<std::string,float>());

} /* namespace matching */

#endif /* SRC_BINDINGS_PYTHONBIND_H_ */
