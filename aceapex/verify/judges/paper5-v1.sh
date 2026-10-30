#!/usr/bin/env bash
# paper5-v1 predates the GOLDEN convention (3ffcb5c, 11.08): corpora are passed by path.
set -uo pipefail
env -i PATH="$PATH" HOME="$HOME" CHR1="$GOLDEN/genome/chr1.fa" ENWIK8="$GOLDEN/text/enwik8" \
  ENWIK9="$GOLDEN/text/enwik9" SILESIA="$GOLDEN/mixed/silesia.tar" \
  FASTQ="$GOLDEN/genome/ERR194147_1gb.fastq" bash ./reproduce_paper5.sh
cp results.json "$RECORDS"
