#!/usr/bin/env bash
# package_linux.sh - a static ACEAPEX CLI for Linux x86-64 (any CPU from x86-64-v2 on: SSE4.2/POPCNT, 2009+; the AVX2
# rANS is chosen at run time where the CPU has it), zstd built from a release tree and linked in, a README, VERSION
# and SHA256SUMS, packed as aceapex-<id>-linux-x86_64.tar.gz. Checks before packing: the binary is static, the
# conformance inputs and (optionally) a FASTA come back byte for byte in the default and the open profile, and
# `faidx` answers.
# Usage: [RELEASE=1] bash scripts/package_linux.sh <zstd source tree> <out dir> [test FASTA]
#   zstd: a release tarball of https://github.com/facebook/zstd/releases (1.5.7: sha256 eb33e51f...ac24fc1b7ee09e6fa3)
set -euo pipefail
Z=$1; O=$2; FA=${3:-}
V=$(sed -n 's/^#define ACEAPEX_VERSION_STRING "\(.*\)"/\1/p' src/aceapex.h)
ID=${RELEASE:+$V}; ID=${ID:-$V-dev+$(git rev-parse --short HEAD)}   # RELEASE=1 on a tagged commit: the plain version
N=aceapex-$ID-linux-x86_64; D=$O/$N; B=$O/build; mkdir -p $D $B/zstd $B/t
for f in $Z/lib/common/*.c $Z/lib/compress/*.c $Z/lib/decompress/*.c $Z/lib/decompress/*.S; do
  [ -f "$f" ] && gcc -O3 -march=x86-64-v2 -I$Z/lib -I$Z/lib/common -c -o $B/zstd/$(basename $f).o $f; done
ar rcs $B/libzstd.a $B/zstd/*.o
g++ -std=c++17 -O3 -march=x86-64-v2 -funroll-loops -static -DACEAPEX_CLI -DACEAPEX_BUILD_ID="\"$ID\"" -I$Z/lib -Isrc \
    -o $D/aceapex src/aceapex_api.cpp $B/libzstd.a -Wl,--whole-archive -lpthread -Wl,--no-whole-archive
strip $D/aceapex
file $D/aceapex | grep -q 'statically linked' || { echo "not static"; exit 1; }
T=$B/t; python3 scripts/make_fixtures.py --regen >/dev/null 2>&1 || true
n=0; for f in /tmp/conf_inputs/*; do [ -f "$f" ] || continue
  $D/aceapex c --in $f --out $T/x.aet >/dev/null 2>&1; $D/aceapex d --in $T/x.aet --out $T/x.out >/dev/null 2>&1
  cmp -s $f $T/x.out || { echo "round trip failed: $f"; exit 1; }; n=$((n+1)); done
echo "conformance inputs: $n round trips byte for byte"
if [ -n "$FA" ]; then for P in default open; do E=""; [ $P = open ] && E="ACEAPEX_BS=16384 LIT_CHUNK=65536 AX_PROFILE=open"
  t0=$(date +%s.%N); env $E $D/aceapex c --in $FA --out $T/fa.$P.aet >/dev/null 2>&1; t1=$(date +%s.%N)
  $D/aceapex d --in $T/fa.$P.aet -c 2>/dev/null | cmp -s - $FA || { echo "FASTA round trip failed ($P)"; exit 1; }; t2=$(date +%s.%N)
  echo "$P: $(stat -c%s $FA) -> $(stat -c%s $T/fa.$P.aet) B, compress $(awk -v a=$t0 -v b=$t1 'BEGIN{printf "%.2f", b-a}') s, decompress to stdout $(awk -v a=$t1 -v b=$t2 'BEGIN{printf "%.2f", b-a}') s, byte for byte"; done
  $D/aceapex faidx $T/fa.open.aet && head -n 1 $T/fa.open.aet.fai | cut -f1 | { read c; $D/aceapex faidx $T/fa.open.aet "$c:1000001-1000120" | head -n 2; }; fi
cp packaging/README_linux.md $D/README.md; sed -i "s/@ID@/$ID/g; s/@ZSTD@/$(basename $Z)/g" $D/README.md
{ $D/aceapex --version; echo "commit $(git rev-parse HEAD)"; echo "built $(date -u +%FT%TZ) on $(uname -m), gcc $(gcc -dumpfullversion), -march=x86-64-v2 -static"; } > $D/VERSION
( cd $D && sha256sum aceapex README.md VERSION > SHA256SUMS )
tar -C $O -czf $O/$N.tar.gz $N; ( cd $O && sha256sum $N.tar.gz > $N.tar.gz.sha256 )
echo "package $O/$N.tar.gz"; cat $O/$N.tar.gz.sha256; cat $D/SHA256SUMS; cat $D/VERSION
