#! /usr/bin/env bash

BENCHMARKS=(
    "403.gcc"
    "429.mcf"
    "434.zeusmp"
    "436.cactusADM"
    "445.gobmk"
    "450.soplex"
    "456.hmmer"
    "458.sjeng"
    "459.GemsFDTD"
    "462.libquantum"
    "470.lbm"
    "471.omnetpp"
    "473.astar"
    "483.xalancbmk"
)

./run_branch_predictors.sh 4.1 cslab_branch_stats.so ${BENCHMARKS[@]}