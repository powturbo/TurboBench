#!/usr/bin/env bash
# The standalone C decoder (c/aceapex_decode.c) against the fixtures: whole-archive decode
# by sha256 on every fixture, then 200 random ranges through the batch API compared with
# slices of the decoded bytes. No corpus needed. Output: claim_id <TAB> verdict <TAB> measured
set -uo pipefail; shopt -s nullglob
F=verify/fixtures; EXP=$(cat $F/chr1_4MiB.sha256); T=$(mktemp -d)
if ! ${CC:-gcc} -std=c99 -O2 -DACEAPEX_ENV_TUNING -o $T/axdec c/axdec.c c/aceapex_decode.c -lzstd 2>$T/build.err; then
  printf 'head_cdecoder_fixtures\tfail\tbuild failed: %s\n' "$(head -c 200 $T/build.err | tr '\n\t' '  ')"
  rm -rf $T; exit 0
fi
ok=0; n=0; msg=""
for a in $F/chr1_4MiB.zstd-*.aet; do
  n=$((n+1)); v=${a##*zstd-}; v=${v%.aet}
  env -i PATH="$PATH" $T/axdec $a $T/o_$v >/dev/null 2>&1
  got=$(sha256sum $T/o_$v 2>/dev/null | cut -c1-64)
  if [ "$got" = "$EXP" ]; then ok=$((ok+1)); else msg="$msg $v:${got:0:12}"; fi
done
[ $n -gt 0 ] && [ $ok = $n ] && r=pass || r=fail
printf 'head_cdecoder_fixtures\t%s\t%d/%d fixtures decode to sha256 %s..%s\n' "$r" $ok $n "${EXP:0:12}" "$msg"
A=$(ls $F/chr1_4MiB.zstd-*.aet | head -n 1); O=$T/o_${A##*zstd-}; O=${O%.aet}
python3 - "$A" "$O" "$T" <<'PY'
import random, subprocess, sys
a,o,t=sys.argv[1:4]; ref=open(o,'rb').read(); N=len(ref); random.seed(2026)
rg=[]
for _ in range(200):
    l=random.choice([1,17,4096,16384,70000,300000]); rg.append((random.randrange(0,N-l+1),l))
open(t+'/r.txt','w').write(''.join(f'{o} {l}\n' for o,l in rg))
r=subprocess.run([t+'/axdec',a,t+'/r.out','-r',t+'/r.txt'],capture_output=True)
got=open(t+'/r.out','rb').read() if r.returncode==0 else b''
exp=b''.join(ref[o:o+l] for o,l in rg)
v='pass' if got==exp else 'fail'
print(f"head_cdecoder_ranges_200\t{v}\t200 random ranges via aceapex_decompress_ranges, {len(exp)} bytes, rc={r.returncode}")
PY
if [ -f $F/empty.aet ]; then
  $T/axdec $F/empty.aet $T/ce.out >/dev/null 2>&1; rc=$?
  [ $rc = 0 ] && [ "$(stat -c%s $T/ce.out 2>/dev/null)" = 0 ] && r=pass || r=fail
  printf 'head_cdecoder_empty\t%s\tC decoder on empty.aet: rc=%s, %s bytes\n' "$r" "$rc" "$(stat -c%s $T/ce.out 2>/dev/null)"
fi
if [ -f $F/conf/manifest.tsv ]; then
  ok=0; n=0; bad=""
  while IFS=$'\t' read -r name sz sha denv; do
    n=$((n+1)); E=""; [ "$denv" != "-" ] && E="$denv"
    env -i PATH="$PATH" $E $T/axdec $F/conf/$name.aet $T/cc_$name >/dev/null 2>&1
    got=$(sha256sum $T/cc_$name 2>/dev/null | cut -c1-64)
    if [ "$got" = "$sha" ]; then ok=$((ok+1)); else bad="$bad $name"; fi
  done < $F/conf/manifest.tsv
  [ $n -gt 0 ] && [ $ok = $n ] && r=pass || r=fail
  printf 'head_conformance_cdecoder\t%s\t%d/%d fixtures decode to manifest sha256%s\n' "$r" $ok $n "${bad:+; failed:$bad}"
fi
# the pip package compiles its own copy of the decoder: it must be byte-identical to c/
if cmp -s c/aceapex_decode.c python/csrc/aceapex_decode.c && cmp -s c/aceapex_decode.h python/csrc/aceapex_decode.h \
   && cmp -s src/ax_rans.h c/ax_rans.h && cmp -s src/ax_rans.h python/csrc/ax_rans.h \
   && cmp -s src/ax_lit_open.h c/ax_lit_open.h && cmp -s src/ax_lit_open.h python/csrc/ax_lit_open.h \
   && cmp -s src/ax_fast.h c/ax_fast.h && cmp -s src/ax_fast.h python/csrc/ax_fast.h; then r=pass; else r=fail; fi
printf 'head_python_csrc_in_sync\t%s\tpython/csrc == c/ (aceapex_decode.c, .h); ax_rans.h, ax_lit_open.h and ax_fast.h identical in src/, c/, python/csrc/\n' "$r"
# the GPU rANS chunk decoder (k_rans, per-lane steps in src/ax_rans_warp.h) run on the CPU
# lane by lane against axr_decode: round-trips, the conformance rANS chunks, mutations
if ${CXX:-g++} -std=c++17 -O2 -Isrc -o $T/rans_emu scripts/rans_warp_emu.cpp 2>$T/emu.err; then
  $T/rans_emu $F/conf/*.aet 2>/dev/null || true
else printf 'head_rans_warp_emu\tfail\tbuild failed: %s\n' "$(head -c 150 $T/emu.err | tr '\n\t' '  ')"; fi
# the GPU open-pack decoder (k_open_cse / k_open_exc steps in src/ax_open_warp.h, host framing
# axo_parse) run on the CPU against axo_dna_decode (ADR-019)
if ${CXX:-g++} -std=c++17 -O2 -Isrc -o $T/open_emu scripts/open_warp_emu.cpp 2>$T/oemu.err; then
  $T/open_emu $F/conf/*.aet 2>/dev/null || true
else printf 'head_open_warp_emu\tfail\tbuild failed: %s\n' "$(head -c 150 $T/oemu.err | tr '\n\t' '  ')"; fi
# library round-trip through the C++ API (the CLI never exercises aceapex_compress):
# sizes across the adaptive block-size boundaries, both levels, several thread counts
if ${CXX:-g++} -std=c++17 -O2 -Isrc -o $T/api_rt scripts/api_roundtrip.cpp src/aceapex_api.cpp -lzstd -lpthread 2>$T/api.err; then
  $T/api_rt 2>/dev/null || true
else printf 'head_api_roundtrip\tfail\tbuild failed: %s\n' "$(head -c 150 $T/api.err | tr '\n\t' '  ')"; fi
# library calls from several threads at once (per-call state is thread-local since 2.2.0)
if ${CXX:-g++} -std=c++17 -O2 -Isrc -o $T/api_cc scripts/api_concurrent.cpp src/aceapex_api.cpp -lzstd -lpthread 2>$T/apic.err; then
  $T/api_cc 2>/dev/null || true
else printf 'head_api_concurrent\tfail\tbuild failed: %s\n' "$(head -c 150 $T/apic.err | tr '\n\t' '  ')"; fi
# thread budget: with threads=1 compress, decompress and region start no thread (2.2.1)
if ${CXX:-g++} -std=c++17 -O2 -DACEAPEX_ENV_TUNING -Isrc -o $T/api_th scripts/api_threads.cpp -lzstd -lpthread 2>$T/apit.err; then
  $T/api_th 2>/dev/null || true
else printf 'head_enc_threads\tfail\tbuild failed: %s\n' "$(head -c 150 $T/apit.err | tr '\n\t' '  ')"; fi
# the GPU library's XXH3 split (src/ax_xxh3.h: parallel block terms + scramble chain) against XXH3_64bits
if ${CXX:-g++} -std=c++17 -O2 -Isrc -o $T/xxh3_split_emu scripts/xxh3_split_emu.cpp 2>$T/xse.err; then
  $T/xxh3_split_emu 2>/dev/null || true
else printf 'head_gpu_xxh3_emu\tfail\tbuild failed: %s\n' "$(head -c 150 $T/xse.err | tr '\n\t' '  ')"; fi
# the GPU library's plan (src/aceapex_gpu_plan.h) executed job by job on the CPU: full, ranges, mutations
if ${CXX:-g++} -std=c++17 -O2 -DACEAPEX_ENV_TUNING -Isrc -o $T/gpu_plan_emu scripts/gpu_plan_emu.cpp src/aceapex_api.cpp -lzstd -lpthread 2>$T/gpe.err; then
  $T/gpu_plan_emu 2>/dev/null || true
else printf 'head_gpu_plan_emu\tfail\tbuild failed: %s\n' "$(head -c 150 $T/gpe.err | tr '\n\t' '  ')"; printf 'head_gpu_flip_emu\tfail\tbuild failed\n'; printf 'head_gpu_zstd_validate\tfail\tbuild failed\n'; fi
# the GPU C ABI's host part (src/aceapex_gpu_abi.cpp): flags, last error, version - no CUDA needed
if ${CXX:-g++} -std=c++17 -O2 -Isrc -o $T/gpu_abi_test scripts/gpu_abi_test.cpp src/aceapex_gpu_abi.cpp 2>$T/gab.err; then
  $T/gpu_abi_test 2>/dev/null || true
else printf 'head_gpu_abi\tfail\tbuild failed: %s\n' "$(head -c 150 $T/gab.err | tr '\n\t' '  ')"; fi
# lzbench #336: the library built without ACEAPEX_ENV_TUNING ignores the environment - five rows of tuning variables
# give the same bytes at level 1 / one thread (text + silesia/xml when ~/CORPORA/silesia.tar is there), and no thread
# is started at threads=1 (strace: no clone)
if ${CXX:-g++} -std=c++17 -O2 -Isrc -o $T/env_test scripts/env_test.cpp src/aceapex_api.cpp -lzstd -lpthread 2>$T/env.err; then
  X=""; [ -f "$HOME/CORPORA/silesia.tar" ] && tar -xOf "$HOME/CORPORA/silesia.tar" xml > $T/xml 2>/dev/null && X=$T/xml
  if command -v strace >/dev/null; then
    out=$(strace -f -qq -e trace=clone,clone3 -o $T/st.txt $T/env_test $X 2>/dev/null); rc=$?; nc=$(grep -c 'clone' $T/st.txt 2>/dev/null); nc=${nc:-0}
    [ $rc = 0 ] && [ "$nc" = 0 ] && r=pass || r=fail
    printf 'head_env_ignored\t%s\tlibrary without ACEAPEX_ENV_TUNING: 6 rows of tuning variables -> the same bytes at level 1 / 1 thread (%s differing or failed; sizes:%s), threads started at threads=1: %s (strace)\n' "$r" "${out%% *}" "${out#* }" "$nc"
  else printf 'head_env_ignored\tdeclared\tstrace not installed\n'; fi
else printf 'head_env_ignored\tfail\tbuild failed: %s\n' "$(head -c 150 $T/env.err | tr '\n\t' '  ')"; fi
# streaming decoder: bit-perfect and a peak memory that does not grow with the archive (16 vs 256 MiB in a child process)
if ${CXX:-g++} -std=c++17 -O2 -DACEAPEX_ENV_TUNING -Isrc -o $T/stream_test scripts/stream_test.cpp src/aceapex_api.cpp -lzstd -lpthread 2>$T/stt.err; then
  $T/stream_test 2>/dev/null || true
else printf 'head_stream\tfail\tbuild failed: %s\n' "$(head -c 150 $T/stt.err | tr '\n\t' '  ')"; fi
# T-H4 on the CPU (scripts/gpu_stress_emu.cpp): the saved corrupt copy that hung the GPU (verify/repro/gpu_hang) is refused
# by the plan, and the first 1000 of the 10 000 copies (918 among them) through plan + executor: 0 hangs, 0 silent with XXH3
if [ -s $HOME/golden/genome/chr1.fa ] && [ -x ./aceapex ] && ${CXX:-g++} -std=c++17 -O2 -Isrc -Iscripts -o $T/semu scripts/gpu_stress_emu.cpp src/aceapex_api.cpp -lzstd -lpthread 2>$T/semu.err; then
  tail -c +100000001 $HOME/golden/genome/chr1.fa | head -c 16777216 > $T/stress.fa
  env -i PATH=$PATH ACEAPEX_BS=16384 LIT_CHUNK=65536 AX_PROFILE=open FSE_CHUNK=65536 ./aceapex c --in $T/stress.fa --out $T/stress.aet >/dev/null 2>&1   # the archive of the Colab run: 64 KiB token chunks
  rp=$($T/semu --check verify/repro/gpu_hang/th4_copy00918.aet); se=$($T/semu $T/stress.aet $T/stress.fa 1000 | grep '^STRESSEMU')
  if [ "$rp" = refused ] && [ "$(md5sum < $T/stress.aet | cut -c1-32)" = 84eba8783bbb5f91508308429c4c9d1c ] && [ "$(echo "$se" | cut -f9)" = ok ]; then
    printf 'head_gpu_stress_emu\tpass\tT-H4 copy 918 (ncse 2 986 345 694 for raw 65 536) refused by the plan; 1000 corrupt copies through plan + executor: refused %s, hangs %s, silent with XXH3 %s\n' "$(echo "$se" | cut -f3)" "$(echo "$se" | cut -f8)" "$(echo "$se" | cut -f7)"
  else printf 'head_gpu_stress_emu\tfail\trepro: %s; run: %s\n' "$rp" "$se"; fi
fi
# open archives written before 02.10 (64 KiB token chunks; the open profile writes 16 KiB since): chr1 in the old open
# profile (FSE_CHUNK=65536 gives its bytes, md5 pinned) decodes byte for byte with the C++ CLI, the C99 axdec, the Python
# reader (ctypes over the C99 library) and the GPU plan + executor
if [ -s $HOME/golden/genome/chr1.fa ] && [ -x ./aceapex ] && [ -x $T/semu ]; then
  env -i PATH=$PATH ACEAPEX_BS=16384 LIT_CHUNK=65536 AX_PROFILE=open FSE_CHUNK=65536 ./aceapex c --in $HOME/golden/genome/chr1.fa --out $T/old.aet >/dev/null 2>&1
  want=$(md5sum < $HOME/golden/genome/chr1.fa | cut -c1-32); am=$(md5sum < $T/old.aet | cut -c1-32)
  m1=$(./aceapex d --in $T/old.aet -c 2>/dev/null | md5sum | cut -c1-32)
  m2=$($T/axdec $T/old.aet $T/old.c99 >/dev/null 2>&1 && md5sum < $T/old.c99 | cut -c1-32); rm -f $T/old.c99
  [ -s $T/libaceapex_decode.so ] || ${CC:-gcc} -std=c99 -O2 -fPIC -shared -DACEAPEX_ENV_TUNING -Ic -o $T/libaceapex_decode.so c/aceapex_decode.c -lzstd 2>/dev/null
  m3=$(ACEAPEX_DECODE_SO=$T/libaceapex_decode.so PYTHONPATH=python python3 -c "import aceapex,hashlib,sys; a=aceapex.open(sys.argv[1]); print(hashlib.md5(a.decompress()).hexdigest())" $T/old.aet 2>/dev/null)
  m4=$($T/semu --decode $T/old.aet $HOME/golden/genome/chr1.fa)
  if [ "$am" = 347ccd8b950cfb4e499249302bb809f8 ] && [ "$m1" = "$want" ] && [ "$m2" = "$want" ] && [ "$m3" = "$want" ] && [ "$m4" = bit-perfect ]; then
    printf 'head_open_compat\tpass\told open archive of chr1 (64 KiB token chunks, md5 %s) byte for byte with the C++ CLI, C99 axdec, Python reader and the GPU plan + executor\n' "${am:0:12}"
  else printf 'head_open_compat\tfail\tarchive %s, cli %s, c99 %s, python %s, gpu-emu %s\n' "${am:0:12}" "${m1:0:12}" "${m2:0:12}" "${m3:0:12}" "$m4"; fi
  rm -f $T/old.aet
fi
# windows batch of the GPU library (aceapex_gpu_decompress_windows_async) on the CPU: the device selection replayed from
# the plan, the jobs and blocks by the plan executor, every window of 8 batches (1 / 8 / 32 KiB x 256 / 4096 / 65536)
# == the original; chr1 in the open profile
if [ -s $HOME/golden/genome/chr1.fa ] && [ -x ./aceapex ] && ${CXX:-g++} -std=c++17 -O2 -Isrc -Iscripts -o $T/wemu scripts/gpu_windows_emu.cpp src/aceapex_api.cpp -lzstd -lpthread 2>$T/wemu.err; then
  env -i PATH=$PATH ACEAPEX_BS=16384 LIT_CHUNK=65536 AX_PROFILE=open ./aceapex c --in $HOME/golden/genome/chr1.fa --out $T/w.aet >/dev/null 2>&1
  r=$($T/wemu $T/w.aet $HOME/golden/genome/chr1.fa | grep '^H5EMU'); rm -f $T/w.aet
  if [ "$(echo "$r" | awk -F'\t' '{print $NF}')" = ok ]; then printf 'head_gpu_windows_emu\tpass\twindows batch, device selection on the CPU, chr1 open: %s\n' "$(echo "$r" | cut -f2 | cut -c1-300)"
  else printf 'head_gpu_windows_emu\tfail\t%s\n' "$(echo "$r" | cut -c1-300)"; fi
fi
# aceapex faidx against samtools faidx (1000 regions, exit codes, -r, .fai)
[ -x ./aceapex ] && bash scripts/faidx_test.sh ./aceapex 2>/dev/null
# AX_LINEMODEL (tuning builds): bit-perfect on 4 kinds of input, full and regions, fused and two-pass decode
if ${CXX:-g++} -std=c++17 -O2 -DACEAPEX_ENV_TUNING -Isrc -o $T/lmtest scripts/linemodel_test.cpp src/aceapex_api.cpp -lzstd -lpthread 2>$T/lmt.err; then
  l1=$($T/lmtest 2>/dev/null); l2=$(AX_LIT_TILE=0 $T/lmtest 2>/dev/null)
  if echo "$l1" | grep -q $'^head_linemodel\tpass' && echo "$l2" | grep -q $'^head_linemodel\tpass'; then echo "$l1" | sed 's/$/; also with AX_LIT_TILE=0 (two-pass)/'
  else printf 'head_linemodel\tfail\t%s | AX_LIT_TILE=0: %s\n' "$(echo "$l1" | cut -f2- | cut -c1-200)" "$(echo "$l2" | cut -f2- | cut -c1-200)"; fi
else printf 'head_linemodel\tfail\tbuild failed: %s\n' "$(head -c 150 $T/lmt.err | tr '\n\t' '  ')"; fi
# AX_REFSEG prototype (tuning build, own container): assembly B = A with 1/1000 substitutions, its third MiB
# reverse-complemented and 70 columns; both FASTA files back byte for byte from the container, B under 10 % of A with references,
# and the default encoder bytes untouched by the hook (A's part identical with and without AX_REFSEG)
if ${CXX:-g++} -std=c++17 -O2 -DACEAPEX_ENV_TUNING -Isrc -o $T/refseg scripts/refseg.cpp -lzstd -lpthread 2>$T/rsg.err; then
  python3 - "$T" <<'PYEOF'
import random, sys
T = sys.argv[1]; random.seed(11); B = b'ACGT'
a = bytearray(random.choice(B) for _ in range(6_000_000))
for s in range(0, len(a), 50_000): a[s:s+300] = a[s:s+300].lower()
b = bytearray(a)
for i in random.sample(range(len(b)), len(b) // 1000): b[i] = random.choice(B) | (b[i] & 0x20)
comp = bytes.maketrans(b'ACGTacgt', b'TGCAtgca')
b[2 << 20:3 << 20] = bytes(b[2 << 20:3 << 20]).translate(comp)[::-1]     # one block reverse-complemented (one segment per block)
for nm, x, w in (('A', a, 60), ('B', b, 70)):
    with open(f'{T}/rs_{nm}.fa', 'wb') as f:
        f.write(b'>' + nm.encode() + b' test\n')
        for i in range(0, len(x), w): f.write(bytes(x[i:i+w]) + b'\n')
PYEOF
  r0=$(AX_REFSEG=0 AX_HASH12=1 $T/refseg $T/rs0.axr $T/rs_A.fa $T/rs_B.fa 2>/dev/null | grep REFSEGROW)
  r1=$(AX_REFSEG=1 AX_HASH12=1 $T/refseg $T/rs1.axr $T/rs_A.fa $T/rs_B.fa 2>/dev/null | grep REFSEGROW)
  a0=$(echo "$r0" | tr '\t' '\n' | grep '^rs_A.fa:' | cut -d: -f2); a1=$(echo "$r1" | tr '\t' '\n' | grep '^rs_A.fa:' | cut -d: -f2)
  b1=$(echo "$r1" | tr '\t' '\n' | grep '^rs_B.fa:' | cut -d: -f2)
  ok0=$(echo "$r0" | awk -F'\t' '{print $NF}'); ok1=$(echo "$r1" | awk -F'\t' '{print $NF}')
  if [ "$ok0" = ok ] && [ "$ok1" = ok ] && [ -n "$a1" ] && [ "$a0" = "$a1" ] && [ $((b1 * 100)) -lt $((a1 * 10)) ]; then
    printf 'head_refseg\tpass\tAX_REFSEG prototype: A and B (1/1000 substitutions, a reverse-complemented MiB) bit-perfect from the container; B %s B with references against A %s B alone; A unchanged by the hook\n' "$b1" "$a1"
  else printf 'head_refseg\tfail\tno refs: %s | refs: %s\n' "$(echo $r0 | cut -c1-120)" "$(echo $r1 | cut -c1-120)"; fi
else printf 'head_refseg\tfail\tbuild failed: %s\n' "$(head -c 150 $T/rsg.err | tr '\n\t' '  ')"; fi
# python layer over the same fixtures, without installing: ctypes loads a fresh .so
if python3 -c "import pytest" 2>/dev/null; then
  if ${CC:-gcc} -std=c99 -O2 -fPIC -shared -DACEAPEX_ENV_TUNING -Ic -o $T/libaceapex_decode.so c/aceapex_decode.c -lzstd 2>/dev/null \
     && ACEAPEX_DECODE_SO=$T/libaceapex_decode.so PYTHONPATH=python python3 -m pytest -q python/tests >$T/py.log 2>&1; then r=pass; else r=fail; fi
  printf 'head_python_tests\t%s\t%s\n' "$r" "$(tail -n 1 $T/py.log | tr '\t' ' ' | head -c 120)"
else
  printf 'head_python_tests\tdeclared\tpytest not installed on this host\n'
fi
rm -rf $T
