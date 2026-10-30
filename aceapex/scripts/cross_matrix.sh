#!/usr/bin/env bash
# Platform matrix on the sources of this tree (release gate, 2.2.0): for every target the static
# libzstd (from a zstd source tree, ZSTD_SRC), the C++ CLI, the C99 decoder and the two API programs
# (scripts/api_roundtrip.cpp, scripts/api_concurrent.cpp) are built, then run natively or under
# qemu-user:
#   conf      every conformance archive (verify/fixtures/conf) through the C99 decoder and the CLI
#   slice     the chr1 4 MiB fixtures of both libzstd lines decode to the pinned sha256
#   det       re-encode of the slice with AX_ENC=chain + interactive profile == the fixture of the
#             libzstd line built here (byte-identical encoder across platforms)
#   l1        default encode of the slice (l1 for DNA), default / open profile == the x86-64 output
#   api       api_roundtrip (library round-trips) and api_concurrent (4 threads at once)
# Usage: scripts/cross_matrix.sh <workdir> [target ...]
#   targets: x86_64 x86_32 arm32 arm64 ppc64le mingw64 (default: all that have a toolchain)
# Toolchains: system gcc (x86_64), arm-linux-gnueabihf-g++, x86_64-w64-mingw32-g++-posix, and
#   I686_BIN / AARCH64_BIN / PPC64LE_BIN = bin dirs of cross toolchains (toolchains.bootlin.com,
#   glibc stable 2024.05-1); x86-32 runs natively (static i686 binary on an x86-64 kernel).
# MinGW is built only (no Windows runner here). Output: one line per target and check.
set -uo pipefail
W=$(realpath -m "${1:?workdir}"); shift; mkdir -p "$W"; R=$(pwd)
ZSRC=${ZSTD_SRC:?ZSTD_SRC = zstd source tree (e.g. zstd-1.5.5)}
ZV=$(awk '/#define ZSTD_VERSION_(MAJOR|MINOR|RELEASE) /{v=v (v?".":"") $3} END{print v}' $ZSRC/lib/zstd.h)
F=verify/fixtures; SLICE=$W/slice.fa
TARGETS=${*:-x86_64 x86_32 arm32 arm64 ppc64le mingw64}
tc(){ case $1 in
  x86_64)  echo "gcc|g++||" ;;
  x86_32)  echo "${I686_BIN:-/nonexistent}/i686-linux-gcc|${I686_BIN:-/nonexistent}/i686-linux-g++||" ;;
  arm32)   echo "arm-linux-gnueabihf-gcc|arm-linux-gnueabihf-g++|qemu-arm-static|" ;;
  arm64)   echo "${AARCH64_BIN:-/nonexistent}/aarch64-linux-gcc|${AARCH64_BIN:-/nonexistent}/aarch64-linux-g++|qemu-aarch64-static|" ;;
  ppc64le) echo "${PPC64LE_BIN:-/nonexistent}/powerpc64le-linux-gcc|${PPC64LE_BIN:-/nonexistent}/powerpc64le-linux-g++|qemu-ppc64le-static|" ;;
  mingw64) echo "x86_64-w64-mingw32-gcc-posix|x86_64-w64-mingw32-g++-posix||build-only" ;;
  esac; }
say(){ printf '%s\t%s\t%s\t%s\n' "$1" "$2" "$3" "$4"; }
sha(){ sha256sum "$1" 2>/dev/null | cut -c1-64; }
# the decoded 4 MiB chr1 slice (host decoder of this tree) and the x86-64 reference encodes
make -s aceapex >/dev/null 2>&1
for a in $F/chr1_4MiB.zstd-*.aet; do ./aceapex d --in $a --out $SLICE --threads 2 >/dev/null 2>&1 && break; done
[ "$(sha $SLICE)" = "$(cat $F/chr1_4MiB.sha256)" ] || { echo "cannot decode the slice on the host"; exit 1; }
for T in $TARGETS; do
  IFS='|' read -r CC CXX RUN MODE <<<"$(tc $T)"
  set -- $CXX; if ! command -v $1 >/dev/null 2>&1; then say $T all declared "no toolchain ($1)"; continue; fi
  [ -n "$RUN" ] && ! command -v $RUN >/dev/null 2>&1 && { say $T all declared "no $RUN"; continue; }
  D=$W/$T; mkdir -p $D
  if [ ! -f $D/zstd/lib/libzstd.a ]; then
    cp -r $ZSRC $D/zstd 2>/dev/null
    make -s -C $D/zstd/lib CC="$CC" AR="${CC%gcc*}ar" libzstd.a >$D/zstd.log 2>&1 || make -s -C $D/zstd/lib CC="$CC" libzstd.a >>$D/zstd.log 2>&1
  fi
  Z=$D/zstd/lib; ST=-static; EXE=""; [ $T = mingw64 ] && EXE=.exe
  B=0
  if [ $T = mingw64 ]; then : >$D/b_cli.log   # the CLI maps files (POSIX mmap); on Windows the library is what lzbench builds
  else $CXX -std=c++17 -O2 $ST -DACEAPEX_CLI -Isrc -I$Z -o $D/aceapex$EXE src/aceapex_api.cpp $Z/libzstd.a -lpthread 2>$D/b_cli.log || B=1; fi
  $CC -std=c99 -O2 $ST -Ic -I$Z -o $D/axdec$EXE c/axdec.c c/aceapex_decode.c $Z/libzstd.a 2>$D/b_axdec.log || B=1
  $CXX -std=c++17 -O2 $ST -Isrc -I$Z -o $D/api_rt$EXE scripts/api_roundtrip.cpp src/aceapex_api.cpp $Z/libzstd.a -lpthread 2>$D/b_rt.log || B=1
  $CXX -std=c++17 -O2 $ST -Isrc -I$Z -o $D/api_cc$EXE scripts/api_concurrent.cpp src/aceapex_api.cpp $Z/libzstd.a -lpthread 2>$D/b_cc.log || B=1
  if [ $B = 1 ]; then say $T build fail "$(cat $D/b_*.log | grep -m1 -i error | cut -c1-160)"; continue; fi
  say $T build pass "$([ $T = mingw64 ] && echo "library (no CLI: POSIX mmap)" || echo CLI), C99 decoder, api_roundtrip, api_concurrent; libzstd $ZV ($CXX)"
  [ "$MODE" = build-only ] && { say $T run declared "no runner for this target on the host"; continue; }
  X(){ env -i PATH="$PATH" "$@"; }; Q(){ if [ -n "$RUN" ]; then X $RUN "$@"; else X "$@"; fi; }
  # conf: C99 decoder and CLI on every conformance archive
  ok=0; n=0; bad=""
  while IFS=$'\t' read -r name sz s denv; do
    [ "${name:0:1}" = "#" ] && continue; n=$((n+2)); E=""; [ "$denv" != "-" ] && E="$denv"
    env -i PATH="$PATH" $E ${RUN:+$RUN} $D/axdec $F/conf/$name.aet $D/o >/dev/null 2>&1; [ "$(sha $D/o)" = "$s" ] && ok=$((ok+1)) || bad="$bad c99:$name"
    env -i PATH="$PATH" $E ${RUN:+$RUN} $D/aceapex d --in $F/conf/$name.aet --out $D/o --threads 2 >/dev/null 2>&1; [ "$(sha $D/o)" = "$s" ] && ok=$((ok+1)) || bad="$bad cli:$name"
  done < $F/conf/manifest.tsv
  [ $ok = $n ] && r=pass || r=fail; say $T conf $r "$ok/$n decodes bit-perfect${bad:+; failed:$bad}"
  # slice: both libzstd lines
  ok=0; n=0
  for a in $F/chr1_4MiB.zstd-*.aet; do n=$((n+2))
    Q $D/axdec $a $D/o >/dev/null 2>&1; [ "$(sha $D/o)" = "$(cat $F/chr1_4MiB.sha256)" ] && ok=$((ok+1))
    Q $D/aceapex d --in $a --out $D/o --threads 2 >/dev/null 2>&1; [ "$(sha $D/o)" = "$(cat $F/chr1_4MiB.sha256)" ] && ok=$((ok+1)); done
  [ $ok = $n ] && r=pass || r=fail; say $T slice $r "$ok/$n (C99 + CLI, fixtures of every libzstd line)"
  # det: chain encoder, interactive profile == fixture of this libzstd line
  if [ -f $F/chr1_4MiB.zstd-$ZV.aet ]; then
    X AX_ENC=chain ACEAPEX_BS=16384 LIT_CHUNK=65536 FSE_CHUNK=4096 ${RUN:+$RUN} $D/aceapex c --in $SLICE --out $D/re.aet --threads 2 >/dev/null 2>&1
    cmp -s $D/re.aet $F/chr1_4MiB.zstd-$ZV.aet && r=pass || r=fail; say $T det $r "chain re-encode == chr1_4MiB.zstd-$ZV.aet"
  else say $T det declared "no fixture for libzstd $ZV"; fi
  # l1: default encoder bytes == x86-64 of this tree, default and open profile (reference built above as x86_64)
  if [ $T != x86_64 ] && [ -x $W/x86_64/aceapex ]; then
    ok=0; for E in "" "AX_PROFILE=open" "ACEAPEX_BS=16384 LIT_CHUNK=65536 FSE_CHUNK=4096"; do
      X $E ${RUN:+$RUN} $D/aceapex c --in $SLICE --out $D/l.aet --threads 2 >/dev/null 2>&1
      X $E $W/x86_64/aceapex c --in $SLICE --out $D/lx.aet --threads 2 >/dev/null 2>&1
      cmp -s $D/l.aet $D/lx.aet && ok=$((ok+1)); done
    [ $ok = 3 ] && r=pass || r=fail; say $T l1 $r "$ok/3 profiles: default (l1) encode == x86-64 bytes"
  fi
  # api
  o=$(Q $D/api_rt 2>/dev/null | tail -1); say $T api_roundtrip "$(echo "$o" | cut -f2)" "$(echo "$o" | cut -f3 | cut -c1-120)"
  o=$(Q $D/api_cc 2>/dev/null | tail -1); say $T api_concurrent "$(echo "$o" | cut -f2)" "$(echo "$o" | cut -f3 | cut -c1-120)"
done
