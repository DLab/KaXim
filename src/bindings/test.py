

import sys
sys.path.append("/git/KaXim/Lib")


import KaXim
import pandas as pd


params = {
	"-i"	:	"../../../KaXim-Tutorial/models/SEIR.xka",
	"-r"	:	"2",
	"-t"	:	"150",
	"-p"	:	"150",
	"--verbose"	:	"4",
	"--params"	:	"10000"
}

result = KaXim.run(params)

#result.getSimulation(0).print()
traj = result.getTrajectory(0)
avg = result.getAvgTrajectory()
result.collectHistogram()
result.listTabs()
tab = result.getTab("Histogram of R0",0)
df_hist = tab.asDataFrame()

print(df_hist)
print(traj.asDataFrame())

#%matplotlib auto
#import matplotlib.pyplot as plt
#import numpy as np



