#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
FIXTURE="$ROOT/scripts/fixtures/micro_p4_sample.md"
TBOUT="$(mktemp -d)"
trap 'rm -rf "$TBOUT"' EXIT

if ! python3 -c "import matplotlib" 2>/dev/null; then
  python3 -m pip install -q -r scripts/requirements.txt
fi

python3 scripts/bench_views.py "$FIXTURE" --stem micro --out-dir "$TBOUT"

test -f "$TBOUT/micro.md"
test -f "$TBOUT/micro_DMBs.md"
test -f "$TBOUT/charts/micro_speed_vs_ratio.png"
test -f "$TBOUT/charts/micro_decomp_speed.png"

# One chart embed per page
count="$(grep -c 'micro_decomp_speed.png' "$TBOUT/micro.md" || true)"
if [[ "$count" -ne 1 ]]; then
  echo "expected 1 decomp chart in micro.md, got $count" >&2
  exit 1
fi

echo "test_bench_views.sh OK → $TBOUT"