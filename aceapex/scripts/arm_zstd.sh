#!/usr/bin/env bash
# Builds static libzstd for the two ARM targets that scripts/arm_test.sh judges, from a zstd
# source checkout. Usage: scripts/arm_zstd.sh <zstd-src-dir> [out=/opt/zstd-arm]
set -euo pipefail
SRC=${1:?zstd source dir}; OUT=${2:-/opt/zstd-arm}; mkdir -p $OUT/include
cp $SRC/lib/zstd.h $SRC/lib/zstd_errors.h $OUT/include/
for tri in aarch64-linux-gnu arm-linux-gnueabihf; do
  ( cd $SRC/lib && make -s clean >/dev/null 2>&1 || true
    CC=$tri-gcc AR=$tri-ar make -s -j"$(nproc)" libzstd.a ZSTD_LEGACY_SUPPORT=0 >/dev/null )
  mkdir -p $OUT/$tri && cp $SRC/lib/libzstd.a $OUT/$tri/ && echo "$tri: $OUT/$tri/libzstd.a"
done
