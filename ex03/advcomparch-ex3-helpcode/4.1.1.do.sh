#! /usr/bin/env bash

## Modify the following paths appropriately
## CAUTION: use only absolute paths below!!!
SNIPER_EXE="/home/nick/arch-ntua/ex03/sniper-7.3/run-sniper"
SNIPER_CONFIG="/home/nick/arch-ntua/ex03/advcomparch-ex3-helpcode/ask3.cfg"
BINARY_CODE="/home/nick/arch-ntua/ex03/advcomparch-ex3-helpcode"
OUTPUT_DIR_BASE="/home/nick/arch-ntua/ex03/outputs/4.1.1"

ARCHITECTURES="1_1_1 2_2_2 4_4_4 8_4_8 16_1_8"
LOCKTYPES="tas_cas tas_ts ttas_cas ttas_ts mutex"
ITERATIONS=1000

for LOCKTYPE in $LOCKTYPES; do
	EXECUTABLE=${BINARY_CODE}/locks-sniper-${LOCKTYPE}
	echo "Running: $(basename ${EXECUTABLE})"
	BENCHOUTDIR=${OUTPUT_DIR_BASE}/
	mkdir -p $BENCHOUTDIR

	for ARCHITECTURE in $ARCHITECTURES; do
		NTHREADS=$(echo $ARCHITECTURE | cut -d'_' -f1)
		L2=$(echo $ARCHITECTURE | cut -d'_' -f2)
		L3=$(echo $ARCHITECTURE | cut -d'_' -f3)

		for GRAIN_SIZE in 1 10 100; do
				OUTDIR=$(printf "%s.NTHREADS_%02d-GRAIN_%03d.out" $LOCKTYPE $NTHREADS $GRAIN_SIZE)
				OUTDIR="${OUTPUT_DIR_BASE}/${OUTDIR}"

				SNIPER_CMD="${SNIPER_EXE} \\
					-c ${SNIPER_CONFIG} \\
					-n ${NTHREADS} \\
					-d ${OUTDIR} \\
					--roi -g --perf_model/l2_cache/shared_cores=$L2 \\
					-g --perf_model/l3_cache/shared_cores=$L3 \\
					-- ${EXECUTABLE} ${NTHREADS} ${ITERATIONS} ${GRAIN_SIZE}"
					echo -e "CMD: $SNIPER_CMD\n"
					/bin/bash -c "time $SNIPER_CMD"
		done
	done
done
