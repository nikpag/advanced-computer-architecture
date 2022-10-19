#! /usr/bin/env bash

ARGS=($@)

EXERCISE_PART=${ARGS[0]}
OBJFILE=${ARGS[1]}
BENCHMARKS=(${ARGS[@]:2})

echo EXERCISE_PART: $EXERCISE_PART
echo OBJFILE: $OBJFILE

for BENCH in ${BENCHMARKS[@]}; do
    echo BENCH: $BENCH
done