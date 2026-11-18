#!/bin/sh
# Ф0: timedemo demo1 in the ISS for a set of cache sizes, in parallel: run_f0.sh <fw.bin> <org> <out_dir> [WAD]
#   writes <out_dir>/sweep_<I$>_<D$>.txt (per-frame counters) and prof.txt (4K/4K run, pc samples)
FW=$1; ORG=$2; OUT=$3; WAD=${4:-$HOME/Downloads/DOOM2.WAD}
mkdir -p "$OUT"
D=$(dirname "$0")
for c in 4096:4096 8192:8192 16384:16384 32768:16384 32768:32768; do
  i=${c%:*}; d=${c#*:}
  extra=""; [ "$c" = 4096:4096 ] && extra="--prof $OUT/prof.txt"
  "$D/iss" "$FW" --org "$ORG" --wad "$WAD" --icache $i --dcache $d $extra > "$OUT/sweep_${i}_${d}.txt" 2>&1 &
done
wait
