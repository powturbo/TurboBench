#!/usr/bin/env bash
# Judge for `aceapex r`: every region equals the byte slice of the original.
# Usage: scripts/region_test.sh <original> <archive.aet> [N=200]   (runs under env -i)
set -euo pipefail
ORIG=$1; AET=$2; N=${3:-200}; BIN=${BIN:-./aceapex}
W=$(mktemp -d); trap 'rm -rf "$W"' EXIT
SZ=$(stat -L -c%s "$ORIG")
python3 - "$ORIG" "$SZ" "$N" "$W" <<'PY'
import sys,random
orig,sz,n,w=sys.argv[1],int(sys.argv[2]),int(sys.argv[3]),sys.argv[4]
bs=16384
pairs=[(0,1),(0,bs),(sz-1,1),(sz-16000,16000),(bs-8,16),(bs*7-1,2),(0,1<<20),(bs*3,bs*2)]
r=random.Random(20260921)
while len(pairs)<n:
    ln=r.choice([1,17,4096,16000,16384,65536]); off=r.randrange(0,sz-ln); pairs.append((off,ln))
f=open(orig,'rb')
with open(f"{w}/pairs","w") as p:
    for i,(o,l) in enumerate(pairs):
        f.seek(o); open(f"{w}/ref{i}","wb").write(f.read(l)); p.write(f"{i} {o} {l}\n")
PY
bad=0; n=0
while read -r i o l; do
  n=$((n+1))
  if env -i PATH="$PATH" "$BIN" r --in "$AET" --out "$W/out" --region "$o" "$l" >/dev/null 2>&1 \
     && cmp -s "$W/out" "$W/ref$i"; then :; else bad=$((bad+1)); echo "MISMATCH off=$o len=$l"; fi
done < "$W/pairs"
# beyond the end must be refused, not truncated silently
if env -i PATH="$PATH" "$BIN" r --in "$AET" --out "$W/out" --region "$((SZ-10))" 100 >/dev/null 2>&1; then
  bad=$((bad+1)); echo "BEYOND-END ACCEPTED"; fi
echo "regions=$n bad=$bad"
[ "$bad" = 0 ]
