#! /usr/bin/env bash

## Modify the following paths appropriately
## CAUTION: use only absolute paths below!!!
BINARY_CODE="/home/nick/arch-ntua/ex03/advcomparch-ex3-helpcode"
OUTPUT_DIR_BASE="/home/nick/arch-ntua/ex03/outputs/4.1.4"

LOCKTYPES="tas_cas tas_ts ttas_cas ttas_ts mutex"
ITERATIONS=150000000
for LOCKTYPE in $LOCKTYPES; do
	EXECUTABLE=${BINARY_CODE}/locks-real-${LOCKTYPE}
	echo "Running: $(basename ${EXECUTABLE})"
	BENCHOUTDIR=${OUTPUT_DIR_BASE}/$BENCH
	mkdir -p $BENCHOUTDIR
	for NTHREADS in 1 2 4 8; do
		for GRAIN_SIZE in 1 10 100; do
				OUT_DIR=$(printf "%s.NTHREADS_%02d-GRAIN_%03d.out" $LOCKTYPE $NTHREADS $GRAIN_SIZE)
				OUT_DIR="${OUTPUT_DIR_BASE}/${OUT_DIR}"
				mkdir -p $OUT_DIR
				OUTFILE=$OUT_DIR/info.out
				touch $OUTFILE
				CMD="${EXECUTABLE} ${NTHREADS} ${ITERATIONS} ${GRAIN_SIZE}"
				echo -e "CMD: $CMD\n"
				/bin/bash -c "(time $CMD) &> $OUTFILE"
				cat $OUTFILE
		done
	done
done
