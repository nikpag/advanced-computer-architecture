#! /usr/bin/env python

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

base_in_dir = "/home/nick/arch-ntua/ex02/outputs/4.1"
base_out_dir = "/home/nick/arch-ntua/ex02/graphs/4.1"

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

in_file_names = [f"{base_in_dir}/{in_file_rel_name}" for in_file_rel_name in in_file_rel_names]


for in_file_name, in_file_rel_name in zip(in_file_names, in_file_rel_names):
    with open(in_file_name) as f:
        branch_stats = dict()
        branch_stat_percents = dict()

        for line in f.readlines():
            tokens = line.split()
            if line.startswith("Total Instructions:"):
                total_ins = int(tokens[2])
            elif line == "\n":
                pass
            elif line.startswith("Branch statistics:"):
                pass
            elif line.startswith("  Total-Branches:"):
                total_branches = int(tokens[1])
            else:
                branch_stats[tokens[0].replace(":", "")] = int(tokens[1])

        branch_stat_percents = {tag: round(value / total_branches * 100, 2) for tag, value in branch_stats.items()}

        labels = [f"{tag} ({percent}%)" for tag, percent in branch_stat_percents.items()]

        p, t = plt.pie(list(branch_stat_percents.values()), colors=[f"tab:{color}" for color in ["blue", "orange", "green", "red", "purple"]])
        plt.title(f"Total-Branches: {total_branches} ({round(total_branches / total_ins * 100, 2)}% of Total Instructions)")
        plt.legend(p, labels, bbox_to_anchor=(1,-0.1), loc="lower right", bbox_transform=plt.gcf().transFigure)
        plt.savefig(f"{base_out_dir}/{in_file_rel_name}.pdf", bbox_inches="tight")