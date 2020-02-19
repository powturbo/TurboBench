#!/bin/sh
progs="arith_static rANS_static4x8 rANS_static4x16"
progs_old=`sed -n 's/^PROGS_OLD=//p' Makefile`
progs_sse=`sed -n 's/^PROGS_SSE=//p' Makefile`
progs_avx2=`sed -n 's/^PROGS_AVX2=//p' Makefile`
#progs_knl=`sed -n 's/^PROGS_KNL=//p' Makefile`
progs_auto="r4x16 r8x16 r16x16 r32x16 r4x16b r8x16b r16x16b r32x16b"

fn=${FN:-/var/tmp/q40}
order=${ORDER:-0}

progs=`echo $progs $progs_old $progs_auto $progs_sse $progs_avx2 $progs_knl | tr ' ' '\012' | sort`

for i in $progs
do
    printf %-20s $i
    (for x in 1 2 3;do ./$i -t -o$order $fn 2>&1;done) | sort -n | tail -1
done