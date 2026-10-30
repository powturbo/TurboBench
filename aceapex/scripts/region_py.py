#!/usr/bin/env python3
"""region_py.py - one region per call through the Python reader (aceapex.Archive.read: persistent C99 handle), one
thread: the T-H3 region list (name:start-end, 1-based; offsets from the FASTA's .fai), every result compared with the
FASTA, p50 / p99 / mean in microseconds, 200 warm-up calls.
Usage: ACEAPEX_DECODE_SO=./libaceapex_decode.so PYTHONPATH=python python3 scripts/region_py.py <archive.aet> <fasta> <regions.txt>"""
import sys, time
import aceapex

def main():
    arc, fasta, regs = sys.argv[1:4]
    fai = {}
    for line in open(fasta + ".fai"):
        n, l, o, b, w = line.split("\t")[:5]; fai[n] = (int(l), int(o), int(b), int(w))
    q = []
    for line in open(regs):
        r = line.strip()
        if not r: continue
        name, se = r.rsplit(":", 1); s, e = (int(x) for x in se.split("-"))
        _, o, b, w = fai[name]
        lo = o + (s - 1) // b * w + (s - 1) % b; hi = o + (e - 1) // b * w + (e - 1) % b + 1
        q.append((lo, hi - lo))
    with open(fasta, "rb") as fh: ref = fh.read()
    a = aceapex.Archive(arc)
    for lo, n in q[:200]: a.read(lo, n)
    t = []; bad = 0
    for lo, n in q:
        t0 = time.perf_counter_ns(); got = a.read(lo, n); t.append((time.perf_counter_ns() - t0) / 1e3)
        if got != ref[lo:lo + n]: bad += 1
    # no a.close(): Archive.close on a mapped file raises BufferError (exported pointers), noted separately
    s = sorted(t); p = lambda x: s[min(len(s) - 1, int(x * len(s)))]
    print(f"[rp] Python aceapex.Archive.read (handle)     p50 {p(.5):7.1f} us  p99 {p(.99):7.1f} us  mean {sum(t)/len(t):7.1f} us  {'DIFFERS' if bad else '== FASTA'}")
    print(f"RPROW\t{len(q)}\t{p(.5):.1f}/{p(.99):.1f}\t{'FAILED' if bad else 'ok'}")
    return 5 if bad else 0

if __name__ == "__main__":
    sys.exit(main())
