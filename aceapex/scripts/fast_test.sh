#!/usr/bin/env bash
# fast_test.sh - the FAST profile (2.4.0, header version 3, docs/FORMAT_FAST.md) judged without a corpus. Output lines:
# claim_id <TAB> verdict <TAB> measured. Usage: bash scripts/fast_test.sh [aceapex binary]
#  head_fast_cli_roundtrip   a generated 6 MB mix (text, DNA with repeats, random, zeros) through c / d at levels 11, 14, 15 and
#                            2 threads, the streaming decode (d --out -), the pipe (c --in - --out - | d), a region across a block
#                            boundary, --profile fast --level 1 == --level 11 byte for byte
#  head_fast_fixture_encode  the five fast fixtures re-encoded from their regenerated inputs == the archives in verify/fixtures/conf
#  head_fast_cdecoder        the C99 decoder (axdec) on the level-11 archive: whole file and 200 random ranges
#  head_fast_api             scripts/fast_api_test.cpp through the library (levels 11..18, sizes, threads, regions, ranges, refusals)
#  head_fast_fuzz            scripts/fast_fuzz.cpp under ASan/UBSan, 20 s, plus its negative control (a short buffer is reported)
#  head_fast_v2_reader       the 2.3.1 C99 decoder (git show v2.3.1:c/...) rejects a fast archive; 'declared' without the tag
set -uo pipefail; shopt -s nullglob
B=${1:-./aceapex}; F=verify/fixtures; T=$(mktemp -d); trap 'rm -rf $T' EXIT
python3 - "$T" <<'PY'
import random, sys
T = sys.argv[1]; r = random.Random(2026); out = bytearray()
words = [''.join(r.choice('abcdefghijklmnopqrstuvwxyz') for _ in range(r.randint(2, 9))) for _ in range(300)]
while len(out) < 1500000: out += (' '.join(r.choice(words) for _ in range(12)) + '\n').encode()
while len(out) < 3500000:
    k = r.choice([1, 1, 50, 300, 2000]); x = r.random()
    out += bytes(r.choice(b'ACGT') for _ in range(k)) if x < 0.85 else (bytes(r.choice(b'acgt') for _ in range(k)) if x < 0.95 else b'N' * k)
for i in range(300):
    a = r.randrange(1500000, len(out) - 5000); b = r.randrange(1500000, len(out) - 5000); out[b:b+3000] = out[a:a+3000]
out += bytes(r.getrandbits(8) for _ in range(1500000)); out += b'\0' * 1000000
open(T + '/in', 'wb').write(bytes(out[:6000000]))
PY
S=$T/in; bad=""; n=0
for L in 11 14 15; do n=$((n+1))   # level 15: the optimal parse, ~6 s per MiB on one core
  env -i PATH="$PATH" $B c --in $S --out $T/f$L.aet --threads 2 --level $L >/dev/null 2>&1 || bad="$bad c$L"
  env -i PATH="$PATH" $B d --in $T/f$L.aet --out $T/o$L --threads 2 >/dev/null 2>&1 && cmp -s $S $T/o$L || bad="$bad d$L"
done
n=$((n+1)); env -i PATH="$PATH" $B d --in $T/f11.aet --out - --threads 2 2>/dev/null | cmp -s - $S || bad="$bad stream"
n=$((n+1)); cat $S | env -i PATH="$PATH" $B c --in - --out - --level 11 2>/dev/null | env -i PATH="$PATH" $B d --in - --out - 2>/dev/null | cmp -s - $S || bad="$bad pipe"
n=$((n+1)); env -i PATH="$PATH" $B r --in $T/f14.aet --out $T/reg --region 1048000 3000 >/dev/null 2>&1; dd if=$S bs=1000 skip=1048 count=3 2>/dev/null | cmp -s - $T/reg || bad="$bad region"
n=$((n+1)); env -i PATH="$PATH" $B c --in $S --out $T/p1.aet --threads 2 --profile fast --level 1 >/dev/null 2>&1; cmp -s $T/p1.aet $T/f11.aet || bad="$bad profile_flag"
n=$((n+1)); [ "$(python3 -c "import struct,sys; b=open(sys.argv[1],'rb').read(68); print(struct.unpack('<I',b[8:12])[0], struct.unpack('<II',b[36:44]))" $T/f11.aet)" = "3 (1, 16)" ] || bad="$bad header"
sz=$(stat -c%s $T/f11.aet)
[ -z "$bad" ] && printf 'head_fast_cli_roundtrip\tpass\t%d/%d checks on a 6 MB generated mix: c/d at levels 11, 14, 15 bit-perfect, stream and pipe bit-perfect, region across the block boundary, --profile fast --level 1 == --level 11, header version 3 / profile 1 / min match 16 (level 11: %s B)\n' $n $n "$sz" \
  || printf 'head_fast_cli_roundtrip\tfail\tfailed:%s\n' "$bad"
# fixtures: regenerated inputs re-encoded with the manifest's level must give the stored archive
if python3 scripts/make_fixtures.py --regen --only text_fast_200K,rand_fast_70K,zeros_fast_1500K,dna_fast_opt_2MiB,rand_fast_7B >/dev/null 2>&1 && [ -d /tmp/conf_inputs ]; then
  ok=0; m=0; bad=""
  for c in text_fast_200K:14 rand_fast_70K:11 zeros_fast_1500K:11 dna_fast_opt_2MiB:15 rand_fast_7B:18; do
    name=${c%%:*}; L=${c##*:}; m=$((m+1))
    env -i PATH="$PATH" $B c --in /tmp/conf_inputs/$name --out $T/re_$name.aet --threads 2 --level $L >/dev/null 2>&1
    cmp -s $T/re_$name.aet $F/conf/$name.aet && ok=$((ok+1)) || bad="$bad $name"
  done
  [ $ok = $m ] && r=pass || r=fail
  printf 'head_fast_fixture_encode\t%s\t%d/%d fast fixtures re-encoded from regenerated inputs == verify/fixtures/conf%s\n' "$r" $ok $m "${bad:+; failed:$bad}"
else printf 'head_fast_fixture_encode\tdeclared\tscripts/make_fixtures.py --regen failed\n'; fi
# C99 decoder
if ${CC:-gcc} -std=c99 -O2 -Wall -DACEAPEX_ENV_TUNING -Ic -o $T/axdec c/axdec.c c/aceapex_decode.c -lzstd 2>$T/b.err; then
  $T/axdec $T/f11.aet $T/c99 >/dev/null 2>&1 && cmp -s $S $T/c99 && r1=ok || r1=FAIL
  python3 - "$T" "$S" <<'PY'
import random, subprocess, sys
T, S = sys.argv[1:3]; ref = open(S, 'rb').read(); N = len(ref); random.seed(2026); rg = []
for _ in range(200):
    l = random.choice([1, 17, 4096, 16384, 70000, 300000, 1500000]); rg.append((random.randrange(0, N - l + 1), l))
open(T + '/r.txt', 'w').write(''.join(f'{o} {l}\n' for o, l in rg))
r = subprocess.run([T + '/axdec', T + '/f11.aet', T + '/r.out', '-r', T + '/r.txt'], capture_output=True)
got = open(T + '/r.out', 'rb').read() if r.returncode == 0 else b''
open(T + '/r.verdict', 'w').write('ok' if got == b''.join(ref[o:o+l] for o, l in rg) else 'FAIL')
PY
  r2=$(cat $T/r.verdict)
  [ "$r1" = ok ] && [ "$r2" = ok ] && r=pass || r=fail
  printf 'head_fast_cdecoder\t%s\tC99 axdec on the level-11 archive: whole file %s, 200 random ranges %s\n' "$r" "$r1" "$r2"
else printf 'head_fast_cdecoder\tfail\tbuild failed: %s\n' "$(head -c 200 $T/b.err | tr '\n\t' '  ')"; fi
# library
if ${CXX:-g++} -std=c++17 -O2 -Isrc -o $T/fapi scripts/fast_api_test.cpp src/aceapex_api.cpp -lzstd -lpthread 2>$T/a.err; then
  $T/fapi 2>/dev/null || true
else printf 'head_fast_api\tfail\tbuild failed: %s\n' "$(head -c 200 $T/a.err | tr '\n\t' '  ')"; fi
# fuzz under the sanitizers + the negative control
if ${CXX:-g++} -O1 -g -std=c++17 -fsanitize=address,undefined -fno-sanitize-recover=all -Isrc -o $T/fuzz scripts/fast_fuzz.cpp src/aceapex_api.cpp -lzstd -lpthread 2>$T/z.err \
   && ${CXX:-g++} -O1 -g -std=c++17 -fsanitize=address,undefined -fno-sanitize-recover=all -DAXF_NEGATIVE -Isrc -o $T/fuzzneg scripts/fast_fuzz.cpp src/aceapex_api.cpp -lzstd -lpthread 2>>$T/z.err; then
  line=$($T/fuzz $S ${FAST_FUZZ_SECONDS:-20} 1 2>/dev/null); rc=$?
  $T/fuzzneg $S 5 1 >/dev/null 2>&1; nrc=$?
  if [ $rc = 0 ] && [ $nrc != 0 ]; then printf '%s; negative control (output one byte short) reported, rc=%d\n' "$line" $nrc
  else printf 'head_fast_fuzz\tfail\tfuzz rc=%d (%s); negative control rc=%d (0 = not caught)\n' $rc "$(echo "$line" | cut -c1-200)" $nrc; fi
else printf 'head_fast_fuzz\tfail\tbuild failed: %s\n' "$(head -c 200 $T/z.err | tr '\n\t' '  ')"; fi
# a version-2 reader rejects the archive (the 2.3.1 C99 decoder from git, when the tag is in the clone)
if git show v2.3.1:c/aceapex_decode.c >$T/old.c 2>/dev/null && git show v2.3.1:c/aceapex_decode.h >$T/aceapex_decode.h 2>/dev/null \
   && git show v2.3.1:c/ax_rans.h >$T/ax_rans.h 2>/dev/null && git show v2.3.1:c/ax_lit_open.h >$T/ax_lit_open.h 2>/dev/null && git show v2.3.1:c/ax_env.h >$T/ax_env.h 2>/dev/null \
   && git show v2.3.1:c/axdec.c >$T/oldmain.c 2>/dev/null && ${CC:-gcc} -std=c99 -O2 -I$T -o $T/axdec231 $T/oldmain.c $T/old.c -lzstd 2>/dev/null; then
  $T/axdec231 $T/f11.aet $T/old.out >/dev/null 2>&1; rc=$?
  [ $rc != 0 ] && [ ! -s $T/old.out ] && r=pass || r=fail
  printf 'head_fast_v2_reader\t%s\t2.3.1 C99 decoder on a fast archive: rc=%d, %s bytes written (must reject)\n' "$r" $rc "$(stat -c%s $T/old.out 2>/dev/null || echo 0)"
else printf 'head_fast_v2_reader\tdeclared\ttag v2.3.1 not in this clone (shallow checkout)\n'; fi
