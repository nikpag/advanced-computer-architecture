#! /usr/bin/env python

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
from scipy.stats.mstats import gmean

base_in_dir = "/home/nick/arch-ntua/ex02/outputs/4.5"
base_out_dir = "/home/nick/arch-ntua/ex02/graphs/4.5"

in_file_rel_names = [
    "403.gcc.cslab_branch_predictors.out",
    "429.mcf.cslab_branch_predictors.out",
    "434.zeusmp.cslab_branch_predictors.out",
    "436.cactusADM.cslab_branch_predictors.out",
    "445.gobmk.cslab_branch_predictors.out",
    "450.soplex.cslab_branch_predictors.out",
    "456.hmmer.cslab_branch_predictors.out",
    "458.sjeng.cslab_branch_predictors.out",
    "459.GemsFDTD.cslab_branch_predictors.out",
    "462.libquantum.cslab_branch_predictors.out",
    "470.lbm.cslab_branch_predictors.out",
    "471.omnetpp.cslab_branch_predictors.out",
    "473.astar.cslab_branch_predictors.out",
    "483.xalancbmk.cslab_branch_predictors.out",
]

mpki_table = []
x_table = []

in_file_names = [f"{base_in_dir}/{in_file_rel_name}" for in_file_rel_name in in_file_rel_names]
for in_file_name, in_file_rel_name in zip(in_file_names, in_file_rel_names):
    with open(in_file_name) as f:
        predictor_strings = iter([
            "Static AlwaysTaken",
            "Static BTFNT",
            "Nbit-8K-4",
            "Pentium-M",
            "Local-History #1",
            "Local-History #2",
            "Global-History #1",
            "Global-History #2",
            "Global-History #3",
            "Global-History #4",
            "Alpha 21264",
            "Tournament #1",
            "Tournament #2",
            "Tournament #3",
            "Tournament #4",
        ])

        x_Axis = []
        mpki_Axis = []

        for line in f.readlines():
            tokens = line.split()
            if line.startswith("Total Instructions:"):
                total_ins = int(tokens[2])
            elif line == "\n":
                pass
            elif line.startswith("RAS:"):
                pass
            elif line.startswith("Branch Predictors:"):
                pass
            elif line.startswith("BTB Predictors:"):
                pass
            else:
                predictor_string = next(predictor_strings)
                correct_predictions = int(tokens[1])
                incorrect_predictions = int(tokens[2])
                x_Axis.append(predictor_string)
                mpki_Axis.append(round(incorrect_predictions / (total_ins / 1000.0), 3))

        x_table.append(x_Axis)
        mpki_table.append(mpki_Axis)

x_Axis = x_table[0]
mpki_Axis = [gmean([mpki_table[i][j] for i in range(0, len(mpki_table))]) for j in range(0, len(mpki_table[0]))]
mpki_Axis = [round(item, 3) for item in mpki_Axis]

fig, ax1 = plt.subplots()
ax1.xaxis.grid(True)

together = list(zip(x_Axis, mpki_Axis))

together = sorted(together, key=lambda t: -t[1])

labels, values = list(zip(*together))

xAx = np.arange(len(x_Axis))
ax1.xaxis.set_ticks(np.arange(0, max(mpki_Axis)+10, 10))
ax1.set_xlim(0, 1.1*max(mpki_Axis))
colors = [
    "lightcoral",
    "chocolate",
    "gold",
    "palegreen",
    "turquoise",
    "deepskyblue",
    "mediumblue",
    "lightpink",
    "lightsteelblue",
    "lightsalmon",
]
bars = ax1.barh(labels, values, label="mpki", color=colors)
ax1.bar_label(bars)



plt.title("MPKI")
plt.savefig(f"{base_out_dir}/gmean.pdf", bbox_inches="tight")