#!/usr/bin/env bash
# stdin_test.sh - claim head_cli_stdin: the CLI works as a pipe in both directions (`c --in - --out -`,
# `d --in - --out -`), as SCB measures compressors. Checks: (1) 5 MB of DNA (chr19 under GOLDEN, else chr1, else the
# 4 MiB chr1 fixture slice) through `cat | aceapex c --in - --out - | aceapex d --in - --out -` is bit-perfect, at 1 and
# 4 threads; (2) the piped archive has the bytes of the archive written from the file (archive bytes do not change);
# (3) every conformance fixture and every chr1_4MiB fixture decodes bit-perfect from a pipe (`cat a.aet | d --in -`);
# (4) an empty pipe is an error for both c and d (rc != 0, nothing written), never an empty archive with rc 0.
# Output: claim_id <TAB> verdict <TAB> measured. Usage: bash scripts/stdin_test.sh [aceapex binary]
set -uo pipefail; shopt -s nullglob
B=${1:-./aceapex}; F=verify/fixtures; G=${GOLDEN:-$HOME/golden}; T=$(mktemp -d); trap 'rm -rf $T' EXIT
bad=""; n=0
if [ -f $G/genome/chr19.fa ]; then head -c 5000000 $G/genome/chr19.fa > $T/s; src="chr19 first 5 MB"
elif [ -f $G/genome/chr1.fa ]; then head -c 5000000 $G/genome/chr1.fa > $T/s; src="chr1 first 5 MB"
else $B d --in $F/chr1_4MiB.zstd-1.4.8.aet --out $T/s >/dev/null 2>&1; src="chr1 4 MiB fixture slice"; fi
for t in 1 4; do n=$((n+1))
  cat $T/s | env -i PATH="$PATH" $B c --in - --out - --threads $t 2>/dev/null > $T/p$t.aet
  cat $T/p$t.aet | env -i PATH="$PATH" $B d --in - --out - --threads $t 2>/dev/null | cmp -s - $T/s || bad="$bad pipe_t$t"
  n=$((n+1)); env -i PATH="$PATH" $B c --in $T/s --out $T/f$t.aet --threads $t >/dev/null 2>&1
  cmp -s $T/p$t.aet $T/f$t.aet || bad="$bad bytes_t$t"
done
while IFS=$'\t' read -r name sz sha denv; do n=$((n+1)); E=""; [ "$denv" != "-" ] && E="$denv"
  got=$(cat $F/conf/$name.aet | env -i PATH="$PATH" $E $B d --in - --out - 2>/dev/null | sha256sum | cut -c1-64)
  [ "$got" = "$sha" ] || bad="$bad $name"
done < $F/conf/manifest.tsv
for a in $F/chr1_4MiB.zstd-*.aet; do n=$((n+1))
  got=$(cat $a | env -i PATH="$PATH" $B d --in - --out - 2>/dev/null | sha256sum | cut -c1-64)
  [ "$got" = "$(cat $F/chr1_4MiB.sha256)" ] || bad="$bad $(basename $a)"; done
n=$((n+1)); : | $B c --in - --out $T/e.aet >/dev/null 2>&1; rc=$?; { [ $rc != 0 ] && [ ! -e $T/e.aet ]; } || bad="$bad empty_c(rc=$rc)"
n=$((n+1)); : | $B c --in - --out - 2>/dev/null > $T/e2.aet; rc=$?; { [ $rc != 0 ] && [ ! -s $T/e2.aet ]; } || bad="$bad empty_c_stdout(rc=$rc)"
n=$((n+1)); : | $B d --in - --out - >/dev/null 2>&1; rc=$?; [ $rc != 0 ] || bad="$bad empty_d(rc=$rc)"
[ -z "$bad" ] && printf 'head_cli_stdin\tpass\t%d/%d checks: c|d pipe round-trip bit-perfect on %s at 1 and 4 threads, piped archive == file archive, all fixtures decode bit-perfect from a pipe, empty pipe -> rc != 0 for c and d\n' $n $n "$src" \
  || printf 'head_cli_stdin\tfail\tfailed:%s (%s)\n' "$bad" "$src"
