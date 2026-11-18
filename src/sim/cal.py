#!/usr/bin/env python3
# Ф0.4: Verilator cycles vs ISS counters on the same frames (run_cal.sh dirs), skipping the first
# SKIP frames (cold caches after the resume). Prints effective CPI per memory config and the
# SDRAM first-word events seen by Verilator ((sd12 - wbuf) / 4) next to the ISS line fills.
import glob, os, sys
SKIP = 3
def fr(fn, iss=False):
    d = {}
    for l in open(fn):
        if l.startswith('F,'):
            p = list(map(int, l.strip().split(',')[1:]))
            d[p[0]] = p[1:]
    return d
cfgs = ['vl_ideal', 'vl_wait', 'vl_wbuf', 'vl_wbuf_sd12', 'vl_wbuf_fastwr']
print(f"{'segment':>8} {'kinstr/fr':>9} {'miss/kI':>7} {'st/kI':>6}  " + ' '.join(f"{c[3:]:>11}" for c in cfgs) + "   first-word ev / ISS fills")
for d in sys.argv[1:]:
    I = fr(os.path.join(d, 'iss.txt'))
    keys = sorted(I)[SKIP:]
    ins = sum(I[k][0] for k in keys); miss = sum(sum(I[k][1:5]) for k in keys); st = sum(I[k][5] + I[k][6] for k in keys)
    cyc = {}
    for c in cfgs:
        V = fr(os.path.join(d, c + '.txt'))
        cyc[c] = sum(V[k][0] for k in keys if k in V)
    ev = (cyc['vl_wbuf_sd12'] - cyc['vl_wbuf']) / 4
    print(f"{os.path.basename(d):>8} {ins/len(keys)/1e3:9.0f} {1000*miss/ins:7.1f} {1000*st/ins:6.0f}  " +
          ' '.join(f"{cyc[c]/ins:11.3f}" for c in cfgs) + f"   {ev/miss:.2f}")
