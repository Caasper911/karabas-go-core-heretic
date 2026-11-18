#!/usr/bin/env python3
# Ф0 cycle model over the iss per-frame counters (timedemo: one frame per game tic):
#   cycles = BASE*instr + (im + dm)*LINE + st_sdram*ST_SDRAM + st_sram*ST_SRAM
# BASE: VexRiscv CPI without memory stalls (not yet fitted for Doom: pico8 Lua fit 1.85, range given).
# LINE: 32-byte line fill from the 16-bit SDRAM at the CPU clock (ACT + CAS 2 + 16 beats + overhead).
# Stores (write-through D$): no write buffer = the CPU waits for each one (SDRAM ~7 clocks, SRAM
# 75 ns per halfword as archie_sram); posted = 1 clock (a FIFO in front of the memory).
# usage: model.py <out_dir of run_f0.sh> [skip frames]
import glob, math, os, sys
OUT = sys.argv[1]
SKIP = int(sys.argv[2]) if len(sys.argv) > 2 else 5
def frames(fn):
    return [list(map(int, l.strip().split(',')[1:])) for l in open(fn) if l.startswith('F,')][SKIP:]
def fps(fr, mhz, base, line, st_sd, st_sr):
    per = sorted(base * f[1] + (f[2] + f[3] + f[4] + f[5]) * line + f[7] * st_sd + f[6] * st_sr for f in fr)
    mean = sum(per) / len(per)
    p95 = per[len(per) * 95 // 100]
    return mhz * 1e6 / mean, mhz * 1e6 / p95
print(f"{'I$/D$':>12} {'Minstr/fr':>9} {'miss/kI':>8}   " +
      "  ".join(f"{m}MHz CPI{b} {'wbuf' if w else 'wait'}" for m, b, w in [(60, 1.6, 0), (60, 1.6, 1), (80, 1.6, 1), (80, 1.9, 1)]))
for fn in sorted(glob.glob(os.path.join(OUT, 'sweep_*.txt')), key=lambda f: [int(x) for x in os.path.basename(f)[6:-4].split('_')]):
    fr = frames(fn)
    if not fr: continue
    i, d = os.path.basename(fn)[6:-4].split('_')
    ins = sum(f[1] for f in fr) / len(fr)
    miss = sum(f[2] + f[3] + f[4] + f[5] for f in fr) / len(fr)
    cols = []
    for mhz, base, wbuf in [(60, 1.6, 0), (60, 1.6, 1), (80, 1.6, 1), (80, 1.9, 1)]:
        line = 2 + 2 + 16 + 3
        st_sd = 1 if wbuf else 7
        st_sr = 1 if wbuf else math.ceil(75e-3 * mhz)
        m, p = fps(fr, mhz, base, line, st_sd, st_sr)
        cols.append(f"{m:5.1f} / {p:4.1f} fps   ")
    print(f"{int(i)//1024:>5}K/{int(d)//1024:>3}K {ins/1e6:9.2f} {1000*miss/ins:8.1f}   " + "".join(cols))
print("(mean / p95 frame; Doom caps at 35 fps)")
