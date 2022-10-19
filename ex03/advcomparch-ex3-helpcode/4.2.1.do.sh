#! /usr/bin/env bash

## Modify the following paths appropriately
## CAUTION: use only absolute paths below!!!
SNIPER_EXE="/home/nick/arch-ntua/ex03/sniper-7.3/run-sniper"
SNIPER_CONFIG="/home/nick/arch-ntua/ex03/advcomparch-ex3-helpcode/ask3.cfg"
BINARY_CODE="/home/nick/arch-ntua/ex03/advcomparch-ex3-helpcode"

OUTPUT_DIR_BASE="/home/nick/arch-ntua/ex03/outputs/4.2.1"
mkdir -p $OUTPUT_DIR_BASE
ARCHITECTURES="4_4_share-all 1_4_share-L3 1_1_share-nothing"

LOCKTYPES="tas_cas tas_ts ttas_cas ttas_ts mutex"
ITERATIONS=1000

for LOCKTYPE in $LOCKTYPES; do
	EXECUTABLE=${BINARY_CODE}/locks-sniper-${LOCKTYPE}
	echo "Running: $(basename ${EXECUTABLE})"
	BENCHOUTDIR=${OUTPUT_DIR_BASE}/$BENCH
	mkdir -p $BENCHOUTDIR

	for architecture in $ARCHITECTURES; do
		NTHREADS=4
		L2=$(echo $architecture | cut -d'_' -f1)
		L3=$(echo $architecture | cut -d'_' -f2)
		NAME=$(echo $architecture | cut -d'_' -f3)

		for GRAIN_SIZE in 1; do
				OUT_DIR=$(printf "%s.NTHREADS_%02d-GRAIN_%03d-NAME_%s.out" $LOCKTYPE $NTHREADS $GRAIN_SIZE $NAME)
				OUT_DIR="${OUTPUT_DIR_BASE}/${OUT_DIR}"

				SNIPER_CMD="${SNIPER_EXE} \\
					-c ${SNIPER_CONFIG} \\
					-n ${NTHREADS} \\
					-d ${OUT_DIR} \\
					--roi \\
					-g --perf_model/l1_icache/shared_cores=1 \\
					-g --perf_model/l1_dcache/shared_cores=1 \\
					-g --perf_model/l2_cache/shared_cores=$L2 \\
					-g --perf_model/l3_cache/shared_cores=$L3 \\
					-- ${EXECUTABLE} ${NTHREADS} ${ITERATIONS} ${GRAIN_SIZE}"
					echo -e "CMD: $SNIPER_CMD\n"
					/bin/bash -c "time $SNIPER_CMD"
		done
	done
done
