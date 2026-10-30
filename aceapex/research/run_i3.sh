#!/usr/bin/env bash
# run_i3.sh - research I3 + I1: 1, 2 and 4 HPRC assemblies (HG00438.1/.2, HG00621.1/.2 of the year-1 index, sha256 of
# the .fa.gz checked against it) concatenated into one FASTA; the open profile as the CLI writes it (16 KiB blocks,
# through the library: .wk/env/lmt) and research/pangenome_refseg.cpp without and with reference segments, with the
# k-mer count (AX_KMER=1). The FASTA is deleted after each step (disk <= 20 GB). Usage: bash research/run_i3.sh <dir with the .fa.gz>
set -euo pipefail
D=$1; R=$(cd "$(dirname "$0")/.." && pwd)
ASM=(HG00438.1 HG00438.2 HG00621.1 HG00621.2)
g++ -std=c++17 -O3 -march=native -DACEAPEX_ENV_TUNING -I$R/src -o $D/pg $R/research/pangenome_refseg.cpp -lzstd -lpthread
for N in ${NS:-1 2 4}; do
  f=$D/cat$N.fa; : > $f; for ((i=0;i<N;i++)); do zcat $D/${ASM[$i]}.fa.gz >> $f; done
  echo "== N=$N $(stat -c%s $f) B md5 $(md5sum < $f | cut -c1-32)"
  env ACEAPEX_BS=16384 LIT_CHUNK=65536 AX_PROFILE=open $R/.wk/env/lmt $f 16 | sed "s/^/open16k: /"
  for r in 0 1; do env AX_REFSEG=$r AX_KMER=1 $D/pg $D/pg$N.$r.axr $f 2>&1 | grep -v "^\[pg\] HG" | sed "s/^/refs$r: /"; rm -f $D/pg$N.$r.axr; done
  rm -f $f
done
