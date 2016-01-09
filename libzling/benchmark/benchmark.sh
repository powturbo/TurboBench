#!/bin/bash
# benchmark script
# author: Zhang Li <zhangli10@baidu.com>

set -e

if [ $# -ne 1 ]; then
    echo "USAGE: sh benchmark.sh testdata" >>/dev/stderr
    exit -1
fi
wd="$(cd $(dirname "$0") && pwd)"
testdata="$1"

_ftimer="/tmp/libzling_benchmark_time"
_fencode="/tmp/libzling_benchmark_test_encode"
_fdecode="/tmp/libzling_benchmark_test_decode"

timer() {
    if sh -c "time $*" 2>&1 | cat >"$_ftimer"; then
        echo -n "$(grep -o -P "(?<=real\s)[\w\d.]+$" <"$_ftimer")"
        return 0
    fi
    return 1
}
benchmark() {
    local name="$1"
    local enccmd="$2"
    local deccmd="$3"
    printf "%-13s enc=%s, dec=%s, size=%s, cmp=%s\n" \
        "[$name]" \
        "$(timer "$enccmd <'$testdata' >'$_fencode'")" \
        "$(timer "$deccmd <'$_fencode' >'$_fdecode'")" \
        "$(wc -c <"$_fencode")" \
        "$(cmp -s "$testdata" "$_fdecode" && echo -n "PASS" || echo -n "FAIL")"
}

benchmark "libzling e0" "$wd/../build/zling_demo e0" "$wd/../build/zling_demo d"
benchmark "libzling e1" "$wd/../build/zling_demo e1" "$wd/../build/zling_demo d"
benchmark "libzling e2" "$wd/../build/zling_demo e2" "$wd/../build/zling_demo d"
benchmark "libzling e3" "$wd/../build/zling_demo e3" "$wd/../build/zling_demo d"
benchmark "libzling e4" "$wd/../build/zling_demo e4" "$wd/../build/zling_demo d"
benchmark "gzip" "gzip -c" "gzip -d"
benchmark "bzip2" "bzip2 -c" "bzip2 -d"
benchmark "xz" "xz -c" "xz -d"
