#!/usr/bin/env bash

BASE_DIR=/home/nick/arch-ntua/ex01/advcomparch-ex1-helpcode
GRAPHS_DIR=/home/nick/arch-ntua/ex01/parsec-3.0/parsec_workspace/graphs
DEVS="L1 L2 TLB PRF"
BENCHMARKS="blackscholes bodytrack canneal fluidanimate freqmine rtview streamcluster swaptions"

rm $GRAPHS_DIR/*

for DEV in $DEVS; do 
    GEO_SCRIPT=plot_${DEV,,}_geoavg.py
    GEO_REAL_SCRIPT=plot_${DEV,,}_geoavg_real.py
    
    for BENCH in $BENCHMARKS; do
        SCRIPT=plot_${DEV,,}.py
        REAL_SCRIPT=plot_${DEV,,}_real.py
        python $BASE_DIR/$SCRIPT outputs/$BENCH*$DEV*.out
        python $BASE_DIR/$REAL_SCRIPT outputs/$BENCH*$DEV*.out
    done

    python $BASE_DIR/$GEO_SCRIPT outputs/*$DEV*.out
    python $BASE_DIR/$GEO_REAL_SCRIPT outputs/*$DEV*.out

done
