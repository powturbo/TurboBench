#!/usr/bin/env bash
# cost.sh SAMPLE spec... : deterministic cost of the block encoder only (callgrind, toggle on *::block), per input byte:
# Ir (instructions), Bcm (mispredicted conditional branches), D1mr+D1mw (L1 data misses), DLmr+DLmw (LL misses)
set -uo pipefail
S=$1; shift; N=$(stat -c %s "$S")
printf 'variant\tratio\tIr/B\tBcm/B\tD1m/B\tLLm/B\tmodel_cyc/B\n'
for v in "$@"; do
  r=$(valgrind --tool=callgrind --branch-sim=yes --cache-sim=yes --toggle-collect='Slot::block*' --toggle-collect='Fy<*>::block*' --toggle-collect='Fz<*>::block*' --toggle-collect='Fx<*>::block*' --toggle-collect='Fw<*>::block*' --callgrind-out-file=/tmp/cg.$$ ./enc2g "$S" 1 "$v" 2>/dev/null | awk -F'\t' 'NR==2{print $2}')
  callgrind_annotate --show=Ir,Bcm,D1mr,D1mw,DLmr,DLmw /tmp/cg.$$ 2>/dev/null | awk -v n=$N -v v="$v" -v r="$r" '/PROGRAM TOTALS/{gsub(",","");gsub(/\([^)]*\)/,"");split($0,a," ");ir=a[1];bcm=a[2];d1=a[3]+a[4];ll=a[5]+a[6];
     printf "%s\t%s\t%.2f\t%.4f\t%.3f\t%.4f\t%.2f\n", v, r, ir/n, bcm/n, d1/n, ll/n, (ir/4 + 15*bcm + 12*d1 + 60*ll)/n}'
done
rm -f /tmp/cg.$$
