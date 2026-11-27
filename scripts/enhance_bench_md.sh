#!/usr/bin/env bash
# Post-process TurboBench Markdown (-p4) into sortable views + PNG charts.
# See scripts/README.md.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

TURBOBENCH="${TURBOBENCH:-./turbobench}"
STEM=""
OUT_DIR=""
INPUT=""
SKIP_PIP=0

usage() {
  sed -n '2,20p' "$0" | sed 's/^# \{0,1\}//'
  cat <<'EOF'

Options:
  --stem NAME       Output base name (default: input stem, or "micro" for *MICRO* .tbb)
  --out-dir DIR     Where to write micro.md, micro_*.md, charts/ (default: turbobench_)
  --turbobench PATH Path to turbobench binary (default: ./turbobench)
  --skip-pip        Do not run pip install (deps already present)
  -h, --help        This text

Examples:
  ./scripts/enhance_bench_md.sh data@MICRO.tbb --stem micro --out-dir turbobench_
  ./scripts/enhance_bench_md.sh data.md --stem micro --out-dir turbobench_
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --stem) STEM="$2"; shift 2 ;;
    --out-dir) OUT_DIR="$2"; shift 2 ;;
    --turbobench) TURBOBENCH="$2"; shift 2 ;;
    --skip-pip) SKIP_PIP=1; shift ;;
    -h|--help) usage; exit 0 ;;
    -*) echo "Unknown option: $1" >&2; usage >&2; exit 2 ;;
    *)
      if [[ -n "$INPUT" ]]; then
        echo "Extra argument: $1" >&2
        exit 2
      fi
      INPUT="$1"
      shift
      ;;
  esac
done

if [[ -z "$INPUT" ]]; then
  usage >&2
  exit 2
fi

if [[ "$INPUT" == *.tbb ]]; then
  if [[ ! -x "$TURBOBENCH" && ! -f "$TURBOBENCH" ]]; then
    echo "turbobench not found: $TURBOBENCH (build with make)" >&2
    exit 1
  fi
  "$TURBOBENCH" "$INPUT" -p4
  # printfile() strips .tbb and @SUFFIX → data@MICRO.tbb → data.md
  base="${INPUT%.tbb}"
  base="${base%%@*}"
  RAW="${base}.md"
else
  RAW="$INPUT"
fi

if [[ ! -f "$RAW" ]]; then
  echo "Markdown not found: $RAW" >&2
  exit 1
fi

if [[ -z "$STEM" ]]; then
  if [[ "$INPUT" == *MICRO* ]] || [[ "$RAW" == data.md ]]; then
    STEM=micro
  else
    STEM="$(basename "${RAW%.md}")"
  fi
fi

OUT_DIR="${OUT_DIR:-turbobench_}"
mkdir -p "$OUT_DIR"

if [[ "$SKIP_PIP" -eq 0 ]]; then
  if python3 -c "import matplotlib" 2>/dev/null; then
    :
  elif [[ -d "$ROOT/scripts/.venv" ]]; then
    # shellcheck source=/dev/null
    source "$ROOT/scripts/.venv/bin/activate"
  else
    echo "Installing Python deps (matplotlib, adjustText)…" >&2
    python3 -m pip install --user -q -r "$ROOT/scripts/requirements.txt" 2>/dev/null \
      || python3 -m pip install -q -r "$ROOT/scripts/requirements.txt"
  fi
fi

python3 "$ROOT/scripts/bench_views.py" "$RAW" --stem "$STEM" --out-dir "$OUT_DIR"

echo "Done. Open ${OUT_DIR}/${STEM}.md"