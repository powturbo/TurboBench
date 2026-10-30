#!/usr/bin/env bash
# paper6-v1 ships its own contract; the judge is that script, unchanged, from an empty environment.
set -uo pipefail
env -i PATH="$PATH" HOME="$HOME" GOLDEN="$GOLDEN" NO_DOWNLOAD=1 bash ./reproduce_paper5.sh
cp results.json "$RECORDS"
