/*
 * PythonBind.cpp
 *
 *  Created on: Oct 18, 2021
 *      Author: naxo
 */

#include "PythonBind.h"
#include <iostream>
#include "../main.h"

namespace binds {

using namespace std;

simulation::Results run_kappa_model(map<string,string> params){
	int argc = 1;
	const char * argv[300];
	argv[0] = "KaXim-Py";
	if(params.count("-o"))
		params["-o"] += ".data";
	else
		params["--out"] += ".data";

	for(auto& param : params){
		argv[argc++] = param.first.c_str();
		size_t i = 0;
		auto cstr = param.second.c_str();
		while(true){
			argv[argc++] = cstr+i;
			if((i = param.second.find(" ",i)) == string::npos)
				break;
			param.second[i] = '\0';
			++i;
		}
	}

	return run(argc,argv);
}


} /* namespace matching */
