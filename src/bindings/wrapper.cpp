/*
 * wrapper.cpp
 *
 *  Created on: Oct 18, 2021
 *      Author: naxo
 */


#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/eigen.h>
#include <pybind11/iostream.h>
#include "PythonBind.h"
#include "../simulation/Results.h"
#include "../simulation/Simulation.h"

namespace binds {

namespace py = pybind11;
using namespace simulation;

#ifdef DEBUG
#define M_NAME KaXimDebug
#else
#define M_NAME KaXim
#endif

PYBIND11_MODULE(M_NAME, m) {
    m.doc() = "KaXim library"; // Optional module docstring
    m.def("run", &run_kappa_model,
    	"Run a kappa model using same parameters as KaXim program. Arguments and model-params are given as Dict.",
		py::arg("args"),py::arg("ka_params") = std::map<std::string,float>());
    m.def("noisy_func", []() {
        py::scoped_ostream_redirect stream(
            std::cout,                               // std::ostream&
            py::module_::import("sys").attr("stdout") // Python output
        );
        py::scoped_ostream_redirect l_stream(
                std::clog,                               // std::ostream&
                py::module_::import("sys").attr("stdout") // Python output
        );
        py::scoped_estream_redirect e_stream();
        std::cout << "c++: cout verbose" << std::endl;
        std::clog << "c++: clog verbose" << std::endl;
        std::cerr << "c++: cerr verbose" << std::endl;
    });
    /*m.attr("redirect_output") = py::capsule(new py::scoped_ostream_redirect(),
        [](void *sor) { delete static_cast<py::scoped_ostream_redirect *>(sor); });
    m.attr("redirect_cerr") = py::capsule(new py::scoped_estream_redirect(),
            [](void *sor) { delete static_cast<py::scoped_estream_redirect *>(sor); });*/
    //m.attr("redirect_clog") = py::capsule(new py::scoped_ostream_redirect(),
     //       [](void *sor) { delete static_cast<py::scoped_ostream_redirect *>(sor); });

    py::class_<Results>(m, "Results")
        .def(py::init<>())
        .def("getTrajectory", &Results::getTrajectory)
        .def("getAvgTrajectory", &Results::getAvgTrajectory)
        .def("getSpatialTrajectories", &Results::getSpatialTrajectories)
		.def("getSimulation", &Results::getSimulation)
		.def("collectHistogram",&Results::collectHistogram)
		.def("collectRawData",&Results::collectRawData)
		.def("getRawData",&Results::getRawData,py::return_value_policy::copy)
		.def("listTabs",&Results::listTabs)
		.def("printTabNames",&Results::printTabNames)
		.def("getTabs",&Results::getTabs)
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



