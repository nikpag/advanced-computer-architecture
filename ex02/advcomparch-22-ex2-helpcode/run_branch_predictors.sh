#! /usr/bin/env bash

# Usage: ./run_branch_predictors.sh <EXERCISE_PART> <OBJFILE> <BENCHMARKS>
# Example: ./run_branch_predictors.sh 4.1 cslab_branch_stats.so 403.gcc 429.mcf

## Execute this script in the helpcode directory.
## Modify the following paths appropriately
## CAUTION: use only absolute paths below!!!

ARGS=($@)

EXERCISE_PART=${ARGS[0]}
OBJFILE=${ARGS[1]}
BENCHMARKS=(${ARGS[@]:2})

PIN_EXE=/home/nick/arch-ntua/ex01/pin-3.22-98547-g7a303a835-gcc-linux/pin
PIN_TOOL=/home/nick/arch-ntua/ex02/advcomparch-22-ex2-helpcode/pintool/obj-intel64/$OBJFILE
outDir=/home/nick/arch-ntua/ex02/outputs/$EXERCISE_PART

for BENCH in ${BENCHMARKS[@]}; do
	cd spec_execs_train_inputs/$BENCH

	line=$(cat speccmds.cmd)
	stdout_file=$(echo $line | cut -d' ' -f2)
	stderr_file=$(echo $line | cut -d' ' -f4)
	cmd=$(echo $line | cut -d' ' -f5-)

	pinOutFile="$outDir/${BENCH}.cslab_branch_predictors.out"
	pin_cmd="$PIN_EXE -t $PIN_TOOL -o $pinOutFile -- $cmd 1> $stdout_file 2> $stderr_file"
	echo "PIN_CMD: $pin_cmd"
	/bin/bash -c "time $pin_cmd"

	cd ../../

done
