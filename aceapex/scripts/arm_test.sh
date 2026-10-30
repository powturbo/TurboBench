#!/usr/bin/env bash
# ARM judge (A4): cross-build the C99 decoder and the C++ CLI for aarch64 and armv7 (32-bit),
# run them under qemu-user on every fixture, and encode the 4 MiB slice on each target to
# compare its bytes with the x86 fixture of the same libzstd line. Needs
# gcc/g++-{aarch64-linux-gnu,arm-linux-gnueabihf}, qemu-{aarch64,arm}-static and a static
# libzstd per target (ZSTD_ARM=<dir> with <dir>/<triplet>/libzstd.a and <dir>/include/zstd.h;
# scripts/arm_zstd.sh builds them). Without any of this every claim is 'declared', not 'fail'.
# Output lines: claim_id <TAB> verdict <TAB> measured. qemu-user does not trap unaligned
# access, so alignment correctness rests on the UBSan run, not on this script.
set -uo pipefail; shopt -s nullglob
F=verify/fixtures; T=$(mktemp -d); Z=${ZSTD_ARM:-/opt/zstd-arm}
for tgt in aarch64-linux-gnu:qemu-aarch64-static:arm64 arm-linux-gnueabihf:qemu-arm-static:armv7; do
  IFS=: read -r tri q short <<<"$tgt"
  if ! command -v $tri-gcc >/dev/null || ! command -v $tri-g++ >/dev/null || ! command -v $q >/dev/null \
     || [ ! -f $Z/$tri/libzstd.a ] || [ ! -f $Z/include/zstd.h ]; then
    printf 'head_%s_cdecoder_conf\tdeclared\tno cross toolchain, qemu or static libzstd for %s on this host\n' "$short" "$tri"
    printf 'head_%s_cli_roundtrip\tdeclared\tsame\n' "$short"; continue; fi
  $tri-gcc -std=c99 -O2 -static -DACEAPEX_ENV_TUNING -Ic -I$Z/include -o $T/axdec_$short c/axdec.c c/aceapex_decode.c $Z/$tri/libzstd.a 2>$T/b1 || { printf 'head_%s_cdecoder_conf\tfail\tbuild: %s\n' "$short" "$(head -c 150 $T/b1|tr '\n' ' ')"; continue; }
  $tri-g++ -std=c++17 -O2 -static -DACEAPEX_CLI -Isrc -I$Z/include -o $T/ace_$short src/aceapex_api.cpp -lpthread $Z/$tri/libzstd.a 2>$T/b2 || { printf 'head_%s_cli_roundtrip\tfail\tbuild: %s\n' "$short" "$(head -c 150 $T/b2|tr '\n' ' ')"; }
  ok=0; n=0; bad=""
  while IFS=$'\t' read -r name sz sha denv; do
    n=$((n+1)); E=""; [ "$denv" != "-" ] && E="$denv"
    env $E $q $T/axdec_$short $F/conf/$name.aet $T/o >/dev/null 2>&1
    [ "$(sha256sum $T/o 2>/dev/null | cut -c1-64)" = "$sha" ] && ok=$((ok+1)) || bad="$bad $name"
  done < $F/conf/manifest.tsv
  for a in $F/chr1_4MiB.zstd-*.aet; do n=$((n+1)); $q $T/axdec_$short $a $T/o >/dev/null 2>&1
    [ "$(sha256sum $T/o 2>/dev/null | cut -c1-64)" = "$(cat $F/chr1_4MiB.sha256)" ] && ok=$((ok+1)) || bad="$bad $(basename $a)"; done
  n=$((n+1)); $q $T/axdec_$short $F/empty.aet $T/o >/dev/null 2>&1 && [ "$(stat -c%s $T/o)" = 0 ] && ok=$((ok+1)) || bad="$bad empty"
  [ $ok = $n ] && r=pass || r=fail
  printf 'head_%s_cdecoder_conf\t%s\t%d/%d fixtures bit-perfect under %s%s\n' "$short" "$r" $ok $n "$q" "${bad:+; failed:$bad}"
  [ -x $T/ace_$short ] || continue
  # CLI: decode the two cross-version fixtures, then encode the slice and compare with x86 bytes
  $q $T/ace_$short d --in $F/chr1_4MiB.zstd-1.4.8.aet --out $T/s --threads 2 >/dev/null 2>&1
  ZV=$(awk '/#define ZSTD_VERSION_(MAJOR|MINOR|RELEASE) /{v=v (v?".":"") $3} END{print v}' $Z/include/zstd.h)
  if [ "$(sha256sum $T/s | cut -c1-64)" = "$(cat $F/chr1_4MiB.sha256)" ]; then
    env -i PATH="$PATH" ACEAPEX_BS=16384 LIT_CHUNK=65536 FSE_CHUNK=4096 $q $T/ace_$short c --in $T/s --out $T/s.aet --threads 2 >/dev/null 2>&1
    $q $T/ace_$short d --in $T/s.aet --out $T/s2 --threads 2 >/dev/null 2>&1
    if cmp -s $T/s $T/s2; then r=pass; else r=fail; fi
    same="no x86 fixture for libzstd $ZV"; for fx in $F/chr1_4MiB.zstd-*.aet; do cmp -s $T/s.aet $fx && same="bytes == $(basename $fx)"; done
    printf 'head_%s_cli_roundtrip\t%s\tdecode 1.4.8 fixture, encode (libzstd %s) + decode round-trip; %s\n' "$short" "$r" "$ZV" "$same"
  else printf 'head_%s_cli_roundtrip\tfail\tCLI could not decode the 1.4.8 fixture under %s\n' "$short" "$q"; fi
done
rm -rf $T
