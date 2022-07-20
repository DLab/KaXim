import sys
if('../cv19gm/models' not in sys.path):
    sys.path.append('../src/bindings/')

import KaXim

run_params = {
	"-i"	:	"~/git/covid19geomodeller/cv19gm/models/kappa/SEIRHVD-meta.xka",#"../examples/spatial-test.ka3",
	"-r"	:	"1",
	"-t"	:	"10",
	"-p"	:	"100",
	"--verbose"	: "4"
}

results = KaXim.run(run_params)

tabs = results.getSpatialTrajectories(0)


for name,tab in tabs.items():
	print(name)
	print(tab.asDataFrame())


