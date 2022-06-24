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

simulation::Results& run_kappa_model(map<string,string> args,map<string,float> ka_params){
	int argc = 1;
	const char * argv[300];
	argv[0] = "KaXim-Py";
	if(args.count("-o"))
		args["-o"] += ".data";
	else
		args["--out"] += ".data";

	for(auto& arg : args){
		argv[argc++] = arg.first.c_str();
		size_t i = 0;
		auto cstr = arg.second.c_str();
		while(true){
			argv[argc++] = cstr+i;
			if((i = arg.second.find(" ",i)) == string::npos)
				break;
			arg.second[i] = '\0';
			++i;
		}
	}

	return *run(argc,argv,ka_params);
}


} /* namespace matching */
