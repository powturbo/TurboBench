#!/bin/bash
BIN="$(dirname $0)/aceapex"

check_file() {
    if [ ! -f "$1" ]; then echo "Error: file not found: $1"; exit 1; fi
    if [ ! -s "$1" ]; then echo "Error: file is empty: $1"; exit 1; fi
}

case "$1" in
  compress)
    check_file "$2"
    $BIN c --in "$2" --out "${2}.aet" --threads ${3:-8} ;;
  decompress)
    check_file "$2"
    $BIN d --in "$2" --out "${2%.aet}" ;;
  test)
    check_file "$2"
    $BIN t --in "$2" --threads ${3:-8} ;;
  *)
    echo "ACEAPEX — lossless compression"
    echo ""
    echo "Usage:"
    echo "  aceapex.sh compress   <file> [threads]"
    echo "  aceapex.sh decompress <file.aet>"
    echo "  aceapex.sh test       <file> [threads]"
    ;;
esac
