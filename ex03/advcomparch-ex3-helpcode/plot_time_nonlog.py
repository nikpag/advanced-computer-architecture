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

global_ws = [1,2,4,8,16,32,64,96,128,192,256,384]

if len(sys.argv) < 2:
	print("usage:", sys.argv[0], "<output_directories>")
	sys.exit(1)

results_tuples = {}

for dirname in sys.argv[1:]:
	if dirname.endswith("/"):
		dirname = dirname[0:-1]
	basename = os.path.basename(dirname)
	output_file = dirname + "/sim.out"

	(lock_type, n_threads, grain) = get_params_from_basename(basename)
	time = get_time_from_output_file(output_file)
	results_tuples.setdefault(grain, []).append((lock_type, n_threads, time))

for (grain_size, res_tuples) in results_tuples.items():
	markers = ['.', 'o', 'v', '*', 'D']
	fig, ax = plt.subplots()
	plt.grid(True)
	ax.set_xlabel(r"$Number\ of\ Threads$", fontsize=14)
	ax.set_ylabel(r"$Time\ (ms)$", fontsize=14)

	i = 0
	#print res_tuples
	tuples_by_lock_type = get_tuples_by_lock_type(res_tuples)


	for tuple in tuples_by_lock_type:
		#print tuple
		lock_type = tuple[0]
		nthread_axis = tuple[1][0]
		time_axis = tuple[1][1]
		x_ticks = [(2**x) for x in np.arange(0, len(time_axis))]
		print(x_ticks)
		x_labels = map(str, x_ticks)
		ax.xaxis.set_ticks(x_ticks)
		ax.xaxis.set_ticklabels(x_labels)

		ax.plot(x_ticks, time_axis, linewidth=1, label=str(lock_type), marker=markers[i%len(markers)])
		i = i + 1

	# Shrink current axis by 20%
	box = ax.get_position()
	#ax.set_position([box.x0, box.y0, box.width * 0.8, box.height])
	# Put a legend to the right of the current axis
	lgd = ax.legend(ncol=1, loc='upper left', bbox_to_anchor=(0, 0.9), prop={'size':10})
	plt.title('Grain Size: ' + str(grain_size))
	output_base = '../graphs/'
	output = output_base + 'grain-' + str(grain_size) + '.pdf'
	print("Saving: " 	+ output)
	plt.savefig(output, bbox_extra_artists=(lgd,), bbox_inches='tight')
