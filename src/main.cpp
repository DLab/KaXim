/*
 * main.cpp
 *
 *  Created on: Dec 16, 2015
 *      Author: naxo
 */

//#include "grammar/KappaLexer.h"
//#include "grammar/KappaParser.hpp"
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/positional_options.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include <omp.h>
#include <vector>
#include "grammar/KappaDriver.h"
#include "grammar/ast/KappaAst.h"
#include "pattern/Environment.h"
#include "simulation/Simulation.h"
#include "simulation/Parameters.h"
#include "util/Warning.h"



#include <ctime>
#include <random>
#include <time.h>

using namespace boost::program_options;
using namespace simulation;
using namespace std;

int main(int argc, char* argv[]){
	const string version("2.2.09");
	const string v_msg("KaXim "+version);
	const string usage_msg("Simple usage is \n$ "
			"KaXim ([-i] kappa_file)+ -t time [-p points] [-r runs]");

	auto t = time(nullptr);

	//printing program name and all arguments
	for(int i = 0; i < argc; i++) cout << argv[i] << " "; cout << endl;

	//eval given arguments to the program
	auto& params = simulation::Parameters::singleton;
	params.makeOptions(v_msg,usage_msg,"Allowed options");
	params.evalOptions(argc, argv);

	//Reading kappa-model files (if any)
	grammar::KappaDriver *driver;
	driver = new grammar::KappaDriver(params.inputFiles);

	//Parsing and building AST
	try{
		driver->parse();
	} catch(const exception &e) {
		cerr << "A parser error found: " << e.what() << endl;
		exit(1);
	}

	grammar::ast::KappaAst &ast = driver->getAst();
	//ast.show();cout << "\n\n" ;

	//building the environment and (global) vars
	pattern::Environment env;//just to delete vars after env
	SimContext base_context(env,params.seed);
	NamesMap<expressions::Auxiliar,FL_TYPE> empty_aux_values;
	base_context.setAuxMap(&empty_aux_values);
	//auto& vars = base_context.getVars();
	try{
		ast.evaluateParams(env,base_context,params.modelParams);
		ast.evaluateDeclarations(env,base_context,true);//constants
		ast.evaluateCompartments(env,base_context);
		ast.evaluateUseExpressions(env,base_context);
		ast.evaluateSignatures(env,base_context);
		ast.evaluateDeclarations(env,base_context,false);//vars
		ast.evaluateChannels(env,base_context);
		ast.evaluateRules(env,base_context);
		ast.evaluatePerts(env,base_context);
		ast.evaluateInits(env,base_context);
	}
	catch(const exception &e){
		cerr << "An exception found while building the environment.\n" << e.what() << endl;
		exit(1);
	}
	env.buildFreeSiteCC();
	env.buildInfluenceMap(base_context);

	//TODO building the connection map for compartments
	map<pair<int,int>,double> edges;
	for(size_t i = 0; i < env.size<pattern::Channel>(); i++ ){
		for(auto& channel : env.getChannels(i)){
			for(auto cells : channel.getConnections(base_context)){
				int src_id = cells.front();
				cells.pop_front();
				for(auto trgt_id : cells){
					edges[pair<int,int>(src_id,trgt_id)] = 1.0;
				}
			}
		}
	}
	//cout << "total cells: " << pattern::Compartment::getTotalCells() << endl;
	//for(auto edge : edges)
	//	cout << edge.first.first << "->" << edge.first.second << ": " << edge.second << endl;
	auto cells = Simulation::allocCells(1,vector<double>(pattern::Compartment::getTotalCells(),1.0),edges,-1);
	//simulation::Simulation sim(env);
	//for(auto i : cells[0])
	//	cout << i << ", ";
	//cout << endl;




	if(params.verbose)//verbose > 0
		env.show(base_context);


	//auto sims = new simulation::Simulation*[params.runs];

	//initialize number of thread (TODO a better way?)
#ifdef DEBUG
	omp_set_num_threads(1);
#endif
	int k = 1;
#	pragma omp parallel
	{
		k = omp_get_num_threads();
	}

#ifndef DEBUG
	omp_set_num_threads(min(params.runs,k));
#endif

#	pragma omp parallel for
	for(int i = 0; i < params.runs; i++){
		Simulation sim(base_context,i);
		//try{
			sim.initialize(cells,ast);
		/*}
		catch(const exception &e){
			cerr << "An exception found on initialization of simulation["
					<< i << "]: " << e.what() << endl;
			exit(1);
		}*/

		if(i == 0)
			WarningStack::getStack().show();
		if( env.getRules().size() < 1){
			cout << "No rules to execute a simulation. Aborting." << endl;
			exit(1);
		}
		if(params.verbose > 0){//only print this if if verbose is set for init or more
			if(i == 0){
				sim.print();
				cout << difftime(time(nullptr),t) << " seconds loading initial state.\n";
				cout << "------------------------------------\n";
			}
			if(params.runs > 1){
				cout << difftime(time(nullptr),t) << " seconds loading initial state";
				cout << " of sim[" << i << "]\n";
			}
		}

		try{
			sim.run(params);
		}catch(exception &e){
			cerr << "An exception found when running simulation[" << i <<"]:\n" << e.what() << endl;
			exit(1);
		}
	}

	cout << "Total running time: " << difftime(time(nullptr),t)  << " seconds." << endl;

	/*delete &env;
	for(auto var_it = vars.rbegin(); var_it != vars.rend() ; var_it++)
		delete *var_it;
	*/
	/* TODO
	 * Environment env = ast.evaluateGlobals();
	 * MPI::Bcast((void*)&env,Nbytes,MPI::Datatype.PACKED, 0);
	 * env = ast.evaluateEnvironment(env,my_rank);
	 * State state = ast.evaluateState(env,my_rank);
	 *
	 * state.run(env) ?
	 */

	delete driver;
	return 0;
}
