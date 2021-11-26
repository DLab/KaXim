/*
 * wrapper.cpp
 *
 *  Created on: Oct 18, 2021
 *      Author: naxo
 */


#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/eigen.h>
#include "PythonBind.h"
#include "../simulation/Results.h"
#include "../simulation/Simulation.h"

namespace binds {

namespace py = pybind11;
using namespace simulation;

PYBIND11_MODULE(KaXim, m) {
    m.doc() = "KaXim library"; // Optional module docstring
    m.def("run", &run_kappa_model, "Run a kappa model using same parameters as KaXim program. Parameters are given in a Dict.");

    py::class_<Results>(m, "Results")
        .def(py::init<>())
        .def("getTrajectory", &Results::getTrajectory)
        .def("getAvgTrajectory", &Results::getAvgTrajectory)
		.def("getSimulation", &Results::getSimulation)
		.def("collectHistogram",&Results::collectHistogram)
		.def("listTabs",&Results::listTabs)
		.def("getTab",&Results::getTab)
		.def("__expr__",[] (const Results& r){r.toString(); });

    py::class_<DataTable>(m, "DataTable")
        .def(py::init<>())
		.def("getData", &DataTable::getData, py::return_value_policy::reference_internal)
        .def_readwrite("data", &DataTable::data)
        .def_readwrite("col_names", &DataTable::col_names)
		.def_readwrite("row_names", &DataTable::row_names)
		.def("asDataFrame", [] (const DataTable& tab){
    		static py::module_ pd = py::module_::import("pandas");
    		std::list<double> row_index;
    		for(auto val : tab.row_names)
    			row_index.push_back(std::stod(val));
    		return pd.attr("DataFrame")(tab.data,row_index,tab.col_names);
    });

    py::class_<Simulation>(m, "Simulation")
    	.def("print",&Simulation::print);
}



}



