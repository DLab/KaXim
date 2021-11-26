


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


simulation::Results run(int argc, const char * const argv[]);




#endif
