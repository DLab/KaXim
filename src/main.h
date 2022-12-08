/** \file */


#ifndef MAIN_H_
#define MAIN_H_


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
#include "simulation/Results.h"
#include "simulation/Parameters.h"
#include "util/Warning.h"


#include <ctime>
#include <random>
#include <time.h>

/** Parse command-line arguments to produce a simulation.
 * \param argc count of arguments in **argv**.
 * \param argv command-line arguments.
 * \param ka_params kappa-parameters values, only when calling from python.
 * \return Results object, with all the data collected by the simulation. */
simulation::Results* run(int argc, const char * const argv[],const map<string,float>& ka_params);

/** Parse command-line arguments to produce a simulation.
 * \param argc count of arguments in **argv**.
 * \param argv command-line arguments.   */
int main(int argc, char* argv[]);


#endif
