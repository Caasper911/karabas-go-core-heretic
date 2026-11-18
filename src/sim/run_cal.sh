#!/bin/sh
# Ф0.4 calibration: the same frames in the ISS (counters) and in Verilator (cycles), from one dump.
#   run_cal.sh <dump.bin> <out_dir> [frames=20] [CPU=VexRiscv]
DUMP=$1; OUT=$2; N=${3:-20}; CPU=${4:-VexRiscv}
mkdir -p "$OUT"
D=$(dirname "$0")
TB=$D/obj_$CPU/tb_soc
"$D/iss" x --resume "$DUMP" --frames $N > "$OUT/iss.txt" 2>&1 &
"$TB" "$DUMP" --frames $N --ideal > "$OUT/vl_ideal.txt" 2>&1 &
"$TB" "$DUMP" --frames $N > "$OUT/vl_wait.txt" 2>&1 &
"$TB" "$DUMP" --frames $N --wbuf > "$OUT/vl_wbuf.txt" 2>&1 &
"$TB" "$DUMP" --frames $N --wbuf --sd-first 12 > "$OUT/vl_wbuf_sd12.txt" 2>&1 &
"$TB" "$DUMP" --frames $N --wbuf --sr-wr 2 --sd-wr 4 > "$OUT/vl_wbuf_fastwr.txt" 2>&1 &
wait
