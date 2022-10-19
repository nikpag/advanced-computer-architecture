#!/usr/bin/env python

import sys, os
import itertools, operator
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np

def get_params_from_basename(basename):
	tokens = basename.split('.')
	lock_type = tokens[0].replace('d', '')
	n_threads = int(tokens[1].split('-')[0].split('_')[1])
	grain_size = int(tokens[1].split('-')[1].split('_')[1])
	name = tokens[1].split('_')[3]
	return (lock_type, n_threads, grain_size, name)

def get_time_from_output_file(output_file):
	time = -999
	fp = open(output_file, "r")
	line = fp.readline()
	while line:
		if line.strip().startswith("Cycles"):
			time = float(line.split()[2])
		line = fp.readline()

	fp.close()
	return time

def get_energy_from_output_file(output_file):
	EDP_1 = 0
	fp = open(output_file, "r")
	line = fp.readline()
	while line:
			if 'total' in line:
					power = float(line.split()[1])
					if (line.split()[2] == 'kW'):
							power = power*1000.0
					elif (line.split()[2] == 'mW'):
							power = power/1000.0

					energy = float(line.split()[3])
					if (line.split()[4] == 'kJ'):
							energy = energy*1000.0
					elif (line.split()[4] == 'mJ'):
							energy = energy/1000.0
					delay = energy/power
					EDP_1 = energy*delay

			line = fp.readline()
	fp.close()
	return (energy, EDP_1)

if len(sys.argv) < 2:
	print("usage:", sys.argv[0], "<output_directories>")
	sys.exit(1)

results_tuples = {}
for dirname in sys.argv[1:]:
	if dirname.endswith("/"):
		dirname = dirname[0:-1]
	basename = os.path.basename(dirname)
	output_file = dirname + "/sim.out"
	output_file2 = dirname + "/power.total.out"

	(lock_type, n_threads, grain, name) = get_params_from_basename(basename)
	time = get_time_from_output_file(output_file)
	(en, edp) = get_energy_from_output_file(output_file2)
	results_tuples.setdefault((name), []).append((lock_type, time, en, edp))

fig, ax = plt.subplots(figsize=(15, 10))
ax.set_xlabel(r"$Synchronization\ Mechanism$", fontsize=14)
ax.set_ylabel(r"$Cycles$", fontsize=14)

i = 0
width = 0.20
colors = ['tab:blue', 'tab:orange', 'tab:green', 'tab:red', 'tab:purple']

for (lock_type, values_tuples) in results_tuples.items():
	y = [val for (typ, val, _, _)  in sorted(values_tuples, key=lambda x: x[0])]
	topologies = [typ for (typ, val, _, _)  in sorted(values_tuples, key=lambda x: x[0])]
	x = np.arange(len(topologies))

	print(y)
	print(topologies)
	ax.bar(x + i*(width), y, color=colors[i], width=width, label=lock_type, zorder=4)
	i += 1

axes = plt.gca()
axes.yaxis.grid(zorder=1)
plt.xticks(x + 1.5*width, topologies)
plt.legend()
# Shrink current axis by 20%
box = ax.get_position()
ax.set_position([box.x0, box.y0, box.width * 0.8, box.height])

# Put a legend to the right of the current axis
ax.legend(loc='center left', bbox_to_anchor=(1, 0.5))
output_base = '../graphs/4.2.1/'
output = output_base + 'topology-time-analysis.pdf'
print("Saving: " 	+ output)
plt.savefig(output, bbox_inches='tight')


fig, ax = plt.subplots(figsize=(15, 10))
ax.set_xlabel(r"$Synchronization\ Mechanism$", fontsize=14)
ax.set_ylabel(r"$Energy\ (J)$", fontsize=14)
i = 0
for (lock_type, values_tuples) in results_tuples.items():
	y = [val for (typ, _, val, _)  in sorted(values_tuples, key=lambda x: x[0])]
	topologies = [typ for (typ, _, _, val)  in sorted(values_tuples, key=lambda x: x[0])]
	x = np.arange(len(topologies))

	print(y)
	print(topologies)
	ax.bar(x + i*(width), y, color=colors[i], width=width, label=lock_type, zorder=4)
	i += 1

axes = plt.gca()
axes.yaxis.grid(zorder=1)
plt.xticks(x + 1.5*width, topologies)
plt.legend()
# Shrink current axis by 20%
box = ax.get_position()
ax.set_position([box.x0, box.y0, box.width * 0.8, box.height])
ax.legend(loc='center left', bbox_to_anchor=(1, 0.5))
output = output_base + 'topology-energy-analysis.pdf'
print("Saving: " 	+ output)
plt.savefig(output, bbox_inches='tight')

fig, ax = plt.subplots(figsize=(15, 10))
ax.set_xlabel(r"$Synchronization\ Mechanism$", fontsize=14)
ax.set_ylabel(r"$EDP\ (J*s)$", fontsize=14)
i = 0
for (lock_type, values_tuples) in results_tuples.items():
	y = [val for (typ, _, _, val)  in sorted(values_tuples, key=lambda x: x[0])]
	topologies = [typ for (typ, _, _, val)  in sorted(values_tuples, key=lambda x: x[0])]
	x = np.arange(len(topologies))

	print(y)
	print(topologies)
	ax.bar(x + i*(width), y, color=colors[i], width=width, label=lock_type, zorder=4)
	i += 1

axes = plt.gca()
axes.yaxis.grid(zorder=1)
plt.xticks(x + 1.5*width, topologies)
plt.legend()
# Shrink current axis by 20%
box = ax.get_position()
ax.set_position([box.x0, box.y0, box.width * 0.8, box.height])
ax.legend(loc='center left', bbox_to_anchor=(1, 0.5))
output = output_base + 'topology-edp-analysis.pdf'
print("Saving: " 	+ output)
plt.savefig(output, bbox_inches='tight')
