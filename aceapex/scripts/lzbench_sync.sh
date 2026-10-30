#!/usr/bin/env bash
# Bring lz+entropy/aceapex in an lzbench checkout up to this tree (PR "aceapex X.Y.Z").
# usage: scripts/lzbench_sync.sh <path-to-lzbench>   (run from the aceapex repo root)
# Layout of lzbench master since 2.4.1 (27-30.09.2026): codecs with an entropy stage live in
# lz+entropy/<codec>/, each codec's build in mk/<codec>.mk (aceapex: one object,
# lz+entropy/aceapex/aceapex_lzbench.o, which #includes the library sources). Copies the codec
# sources verbatim (since ADR-018/019 also ax_rans.h, ax_lit_open.h), points the wrapper at
# aceapex_decompress_mt (threads=1 spawns nothing under lzbench -T) and sets the codec name.
# The CUDA decoder (lz+entropy/aceapex/cuda, aceapex_cuda 0.9) is not touched: its
# aceapex_streams_t has the same layout as the one in aceapex.h. Levels, algorithm string,
# README and CHANGELOG are edited by hand in lzbench style. Idempotent. 2.4.0: copies ax_fast*.h and adds the codec
# aceapex_fast (levels 1..8 = library 11..18) to bench/lzbench.h, bench/codecs.h and the wrapper.
set -euo pipefail
LZ=${1:?lzbench dir}; V=$(sed -n 's/#define ACEAPEX_VERSION_STRING "\(.*\)"/\1/p' src/aceapex.h)
D="$LZ/lz+entropy/aceapex"
[ -d "$D" ] && [ -f "$LZ/mk/aceapex.mk" ] || { echo "$LZ: no lz+entropy/aceapex or mk/aceapex.mk (lzbench before 2.4.1?)"; exit 1; }
for f in aceapex.h aceapex_api.cpp aceapex_main.cpp lit_fse.cpp ax_align.h ax_env.h ax_rans.h ax_lit_open.h ax_fast.h ax_fast_enc.h ax_fast_glue.h xxhash.h; do cp src/$f "$D/$f"; done
python3 - "$LZ" "$V" <<'PY'
import sys, re
lz, v = sys.argv[1], sys.argv[2]
p = lz + '/lz+entropy/aceapex/aceapex_lzbench.cpp'; s = open(p).read()
s = s.replace("""    int thr = opts->threads > 0 ? opts->threads : (s ? s->threads : 1);
    int64_t r = aceapex_decompress(inbuf, insize, outbuf, outsize, thr);
    return r >= 0 ? (int64_t)outsize : -1;""",
"""    // thr == 1 (lzbench -T: one codec copy per pool thread) decodes on the calling
    // thread and spawns nothing; -I# gives the codec its own budget.
    int thr = opts->threads > 0 ? opts->threads : (s ? s->threads : 1);
    int64_t r = aceapex_decompress_mt(inbuf, insize, outbuf, outsize, thr);
    return r >= 0 ? r : -1;""")
open(p, 'w').write(s)
# 2.4.0: the FAST profile as its own codec aceapex_fast, levels 1..8 = library levels 11..18 (one init that adds 10;
# compress and decompress are aceapex's). A separate entry keeps `-eaceapex` at its three default-profile levels.
if 'lzbench_aceapex_fast_init' not in s:
    s += '''
// aceapex_fast: the FAST profile (aceapex 2.4.0, header version 3), levels 1..8 = aceapex_compress levels 11..18
char* lzbench_aceapex_fast_init(size_t insize, size_t level, size_t threads) {
    return lzbench_aceapex_init(insize, level + 10, threads);
}
'''
    open(p, 'w').write(s)
p = lz + '/lz+entropy/aceapex/aceapex_lzbench.h'; s = open(p).read()
if 'lzbench_aceapex_fast_init' not in s:
    s += 'char* lzbench_aceapex_fast_init(size_t insize, size_t level, size_t threads);\n'; open(p, 'w').write(s)
p = lz + '/bench/codecs.h'; s = open(p).read()
if 'lzbench_aceapex_fast_init' not in s:
    s = s.replace('    void lzbench_aceapex_deinit(char* workmem);\n',
                  '    void lzbench_aceapex_deinit(char* workmem);\n    char* lzbench_aceapex_fast_init(size_t insize, size_t level, size_t threads);\n', 1)
    s = s.replace('    #define lzbench_aceapex_init NULL\n', '    #define lzbench_aceapex_init NULL\n    #define lzbench_aceapex_fast_init NULL\n', 1)
    open(p, 'w').write(s)
p = lz + '/bench/lzbench.h'; s = open(p).read()
s = re.sub(r'\{ "aceapex",    "aceapex [0-9.]+",', '{ "aceapex",    "aceapex %s",' % v, s)
if '"aceapex_fast"' not in s:
    m = re.search(r'\n(    \{ "aceapex",    [^\n]*\n)', s)
    fast = ('    { "aceapex_fast","aceapex %s fast",      "LZ77",                        1,   8,    0, FULL_THREADING, '
            'lzbench_aceapex_compress,    lzbench_aceapex_decompress,    lzbench_aceapex_fast_init, lzbench_aceapex_deinit },\n') % v
    s = s[:m.end()] + fast + s[m.end():]
else:
    s = re.sub(r'"aceapex [0-9.]+ fast"', '"aceapex %s fast"' % v, s)
open(p, 'w').write(s)
PY
cd "$LZ" && git status --short && echo "lz+entropy/aceapex == aceapex $V; build: make -j\$(nproc); CI set: ./lzbench -eLZ -v5 ./lzbench; -eLZ+ENTROPY -v5 ./lzbench; -eSYMMETRIC -v5 ./lzbench; -eFASTEST -t0,0 -T2 -jr ."
