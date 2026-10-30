#!/bin/bash
BIN=${1:-./aceapex}
FILE=${2:-enwik9}
echo "=== ACEAPEX v2 Benchmark ==="
echo "Hardware: $(grep 'model name' /proc/cpuinfo | head -1 | cut -d: -f2)"
echo "Date: $(date -u)"
echo ""
for t in 1 2 4 8; do
    echo "--- threads=$t ---"
    $BIN t --in $FILE --threads $t
done
