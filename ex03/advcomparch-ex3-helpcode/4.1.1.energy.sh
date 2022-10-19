#! /usr/bin/env bash

OUTPUT_DIR_BASE="/home/nick/arch-ntua/ex03/outputs/4.1.1"
SNIPER_DIR="/home/nick/arch-ntua/ex03/sniper-7.3"

echo "Outputs to be processed located in: $OUTPUT_DIR_BASE"

for BENCHDIR in $OUTPUT_DIR_BASE/*; do
  BENCH=$(basename $BENCHDIR)
  echo -e "\nProcessing directory: $BENCH"

  cmd="${SNIPER_DIR}/tools/advcomparch_mcpat.py -d $BENCHDIR -t total -o $BENCHDIR/info > $BENCHDIR/power.total.out"
  echo CMD: $cmd
  /bin/bash -c "$cmd"
done
