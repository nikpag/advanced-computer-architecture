#! /usr/bin/env python

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
from scipy.stats.mstats import gmean

base_in_dir = "/home/nick/arch-ntua/ex02/outputs/4.4"
base_out_dir = "/home/nick/arch-ntua/ex02/graphs/4.4"

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
                predictor_string = tokens[1][1:]
                correct_predictions = int(tokens[3])
                incorrect_predictions = int(tokens[4])
                x_Axis.append(predictor_string)
                mpki_Axis.append(incorrect_predictions / (total_ins / 1000.0))

        x_table.append(x_Axis)
        mpki_table.append(mpki_Axis)

x_Axis = x_table[0]
mpki_Axis = [gmean([mpki_table[i][j] for i in range(0, len(mpki_table))]) for j in range(0, len(mpki_table[0]))]


fig, ax1 = plt.subplots()
ax1.grid(True)

xAx = np.arange(len(x_Axis))
ax1.xaxis.set_ticks(np.arange(0, len(x_Axis), 1))
ax1.set_xticklabels(x_Axis, rotation=45)
ax1.set_xlim(-0.5, len(x_Axis) - 0.5)
ax1.set_xlabel("Entries in RAS")
ax1.set_ylim(min(mpki_Axis) - 0.05, max(mpki_Axis) + 0.05)
ax1.set_ylabel("$MPKI$")
line1 = ax1.plot(mpki_Axis, label="mpki", color="tab:blue", marker="x")

plt.title("MPKI")
plt.savefig(f"{base_out_dir}/gmean.pdf", bbox_inches="tight")