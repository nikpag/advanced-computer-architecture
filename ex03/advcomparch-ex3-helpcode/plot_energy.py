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
	return (lock_type, n_threads, grain_size)

def get_energy_from_output_file(output_file):
	EDP_1 = 0
	fp = open(output_file, "r")
	line = fp.readline()
	while line:
			if 'total' in line:
					power = float(line.split()[1])
					print(line.split()[4])
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


def get_time_from_output_file(output_file):
	time = -999
	fp = open(output_file, "r")
	line = fp.readline()
	while line:
		if line.strip().startswith("Time"):
			time = float(line.split()[3])/10**6
		line = fp.readline()

	fp.close()
	return time

def get_tuples_by_lock_type(tuples):
	ret = []
	tuples_sorted = sorted(tuples, key=operator.itemgetter(0))
	for key,group in itertools.groupby(tuples_sorted,operator.itemgetter(0)):
		ret.append((key, list(zip(*map(lambda x: x[1:], list(group))))))
	return ret

if len(sys.argv) < 2:
	print("usage:", sys.argv[0], "<output_directories>")
	sys.exit(1)

energy_tuples = {}
edp_tuples = {}

for dirname in sys.argv[1:]:
	if dirname.endswith("/"):
		dirname = dirname[0:-1]
	basename = os.path.basename(dirname)
	output_file = dirname + "/power.total.out"
	output_file2 = dirname + "/sim.out"

	(lock_type, n_threads, grain) = get_params_from_basename(basename)
	(energy, edp) = get_energy_from_output_file(output_file)

	energy_tuples.setdefault(grain, []).append((lock_type, n_threads, energy))
	edp_tuples.setdefault(grain, []).append((lock_type, n_threads, edp))

for grain_size in energy_tuples.keys():
	markers = ['.', 'o', 'v', '*', 'D']
	fig, ax = plt.subplots(2, figsize=(10, 17))
	fig.suptitle('Grain Size: ' + str(grain_size))

	ax[0].set_xlabel(r"$Number\ of\ Threads$", fontsize=14)
	ax[0].set_ylabel(r"$Energy\ (J)$", fontsize=14)
	ax[1].set_xlabel(r"$Number\ of\ Threads$", fontsize=14)
	ax[1].set_ylabel(r"$EDP\ (J*sec)$", fontsize=14)

	energy_tuples_by_lock_type = get_tuples_by_lock_type(energy_tuples[grain_size])
	edp_tuples_by_lock_type = get_tuples_by_lock_type(edp_tuples[grain_size])


	i = 0
	for tuple in energy_tuples_by_lock_type:
		nthread_axis = tuple[1][0]
		lock_type = tuple[0]
		energy_axis = tuple[1][1]
		x_ticks = 2**np.arange(0, len(energy_axis))
		ax[0].plot(x_ticks, energy_axis, linewidth=1, label=str(lock_type), marker=markers[i%len(markers)])
		i = i + 1

	i = 0
	for tuple in edp_tuples_by_lock_type:
		lock_type = tuple[0]
		energy_axis = tuple[1][1]
		x_ticks = 2**np.arange(0, len(energy_axis))
		ax[1].plot(x_ticks, energy_axis, linewidth=1, label=str(lock_type), marker=markers[i%len(markers)])
		i = i + 1

	ax[0].grid(b=True)
	ax[1].grid(b=True)
	x_labels = map(str, nthread_axis)
	x_ticks = 2**np.arange(0, len(energy_axis))
	plt.setp(ax, xticks=x_ticks, xticklabels=x_labels)
	lgd = ax[1].legend(ncol=len(energy_tuples_by_lock_type), bbox_to_anchor=(0.94, -0.08), prop={'size':12})
	lgd = ax[0].legend(ncol=len(energy_tuples_by_lock_type), bbox_to_anchor=(0.94, -0.08), prop={'size':12})

	output_base = '../graphs/4.1.3/'
	output = output_base + 'grain-' + str(grain_size) + '.pdf'
	print("Saving: " 	+ output)
	plt.savefig(output, bbox_extra_artists=(lgd,), bbox_inches='tight')
