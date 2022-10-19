#!/usr/bin/env python

import sys
import numpy as np
from scipy.stats import gmean

## We need matplotlib:
## $ apt-get install python-matplotlib
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

x_Axis = []
ipc_Axis = []
mpki_Axis = []

outFiles = {}

for outFile in sys.argv[1:]:
	benchmark = outFile.split('/')[-1].split('.')[0]
	if benchmark in outFiles:
		outFiles[benchmark].append(outFile)
	else:
		outFiles[benchmark] = [outFile] 

is_first_measurement = True
first_assoc = None
first_size = None

def real_ipc(ipc, curr_assoc, curr_size, ref_assoc, ref_size):
	global first_assoc, first_size, is_first_measurement

	if is_first_measurement:
		is_first_measurement = False
		first_assoc = curr_assoc
		first_size = curr_size
		return ipc

	M = int(curr_assoc) / int(ref_assoc)
	N = int(curr_size) / int(ref_size)

	new_ipc = ipc / (1.10**(np.log2(M)) * 1.15**(np.log2(N)))
		
	return new_ipc



for i in range(len(list(outFiles.values())[0])):
	ipc_list = []
	mpki_list = []
	for benchmark, benchmarkList in outFiles.items():
		outFile = benchmarkList[i]
		ipc = None
		l2_size = None
		l2_assoc = None
		fp = open(outFile)
		line = fp.readline()
		while line:
			tokens = line.split()
			if (line.startswith("Total Instructions: ")):
				total_instructions = int(tokens[2])
			elif (line.startswith("IPC:")):
				ipc = float(tokens[1])
			elif (line.startswith("  L2-Data Cache")):
				sizeLine = fp.readline()
				l2_size = sizeLine.split()[1]
				bsizeLine = fp.readline()
				l2_bsize = bsizeLine.split()[2]
				assocLine = fp.readline()
				l2_assoc = assocLine.split()[1]
			elif (line.startswith("L2-Total-Misses")):
				l2_total_misses = int(tokens[1])
				l2_miss_rate = float(tokens[2].split('%')[0])
				mpki = l2_total_misses / (total_instructions / 1000.0)


			line = fp.readline()

		fp.close()

		new_ipc = real_ipc(ipc=ipc, curr_assoc=l2_assoc, curr_size=l2_size, ref_assoc=first_assoc, ref_size=first_size)
		ipc_list.append(new_ipc)
		mpki_list.append(mpki)

	l2ConfigStr = '{}K.{}.{}B'.format(l2_size,l2_assoc,l2_bsize)
	print(l2ConfigStr)
	x_Axis.append(l2ConfigStr)
	ipc_Axis.append(gmean(ipc_list))
	mpki_Axis.append(gmean(mpki_list))

print(x_Axis)
print(ipc_Axis)
print(mpki_Axis)

fig, ax1 = plt.subplots()
ax1.grid(True)
ax1.set_xlabel("CacheSize.Assoc.BlockSize")

xAx = np.arange(len(x_Axis))
ax1.xaxis.set_ticks(np.arange(0, len(x_Axis), 1))
ax1.set_xticklabels(x_Axis, rotation=45)
ax1.set_xlim(-0.5, len(x_Axis) - 0.5)
ax1.set_ylim(min(ipc_Axis) - 0.05 * min(ipc_Axis), max(ipc_Axis) + 0.05 * max(ipc_Axis))
ax1.set_ylabel("$IPC$")
line1 = ax1.plot(ipc_Axis, label="ipc", color="red",marker='x')

ax2 = ax1.twinx()
ax2.xaxis.set_ticks(np.arange(0, len(x_Axis), 1))
ax2.set_xticklabels(x_Axis, rotation=45)
ax2.set_xlim(-0.5, len(x_Axis) - 0.5)
ax2.set_ylim(min(mpki_Axis) - 0.05 * min(mpki_Axis), max(mpki_Axis) + 0.05 * max(mpki_Axis))
ax2.set_ylabel("$MPKI$")
line2 = ax2.plot(mpki_Axis, label="L2D_mpki", color="green",marker='o')

lns = line1 + line2
labs = [l.get_label() for l in lns]

plt.title("IPC vs MPKI")
lgd = plt.legend(lns, labs)
lgd.draw_frame(False)
plt.savefig("graphs/L2_geoavg_real.png", bbox_inches="tight")
