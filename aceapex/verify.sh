#!/usr/bin/env bash
# verify.sh — every published claim judged from the exact sources it was made with.
# Each tag is checked out into a detached worktree and judged by verify/judges/<tag>.sh
# from main: tags stay immutable, judges can improve. One results/<tag>.json per tag,
# carrying provenance and one record per claim (claim_id, level, expected, tolerance,
# measured, verdict, command). Usage: ./verify.sh [tag ...]   (default: verify/manifest.tsv)
set -uo pipefail
ROOT=$(cd "$(dirname "$0")" && pwd); cd "$ROOT"
OUT=${OUT:-$ROOT/results}; WT=${WT:-/tmp/aceapex-verify}; mkdir -p "$OUT" "$WT"
TAGS=${*:-$(awk '!/^#/ && NF{print $1}' verify/manifest.tsv)}
FAILS=0; printf '%-14s %5s %5s %8s %9s %8s %6s\n' tag pass fail skipped declared unjudged known
for T in $TAGS; do
  J="verify/judges/$T.sh"; D="$WT/$T"; R="$D/_records.json"
  if [ ! -f "$J" ]; then printf '%-14s no judge\n' "$T"; continue; fi
  if [ "$T" != HEAD ] && ! git rev-parse -q --verify "refs/tags/$T^{commit}" >/dev/null; then printf '%-14s no tag\n' "$T"; continue; fi
  if [ "$T" = HEAD ]; then D="$ROOT"; R="$WT/HEAD_records.json"; else
    git worktree remove --force "$D" >/dev/null 2>&1 || true
    git worktree add --detach -q "$D" "$T"; fi
  ( cd "$D" && ROOT="$ROOT" RECORDS="$R" GOLDEN="${GOLDEN:-$HOME/golden}" bash "$ROOT/$J" ) >"$WT/$T.log" 2>&1 || echo "judge exit $? (see $WT/$T.log)"
  [ -s "$R" ] || echo '{"records":[]}' >"$R"
  python3 - "$T" "$(git rev-parse "$T^{commit}")" "$R" "$OUT/$T.json" <<'PY'
import sys,json,subprocess,datetime,platform
tag,sha,rec,out=sys.argv[1:]
body=json.load(open(rec))
def walk(o):
    if isinstance(o,dict):
        if "verdict" in o: yield o
        for v in o.values(): yield from walk(v)
    elif isinstance(o,list):
        for v in o: yield from walk(v)
recs=list(walk(body))
zv=(lambda h:".".join(next((l.split()[2] for l in h if l.startswith("#define ZSTD_VERSION_"+k)),"?") for k in ("MAJOR","MINOR","RELEASE")))(next((open(d+"/zstd.h").read().splitlines() for d in ("/usr/include","/usr/local/include") if __import__("os").path.exists(d+"/zstd.h")),[]))
# known deviations: a fail listed in verify/known/<tag>.tsv with a condition that holds here
# (e.g. libzstd!=1.4.8) becomes known-deviation, kept visible in its own column, never hidden.
import os
kf="verify/known/%s.tsv"%tag
if os.path.exists(kf):
    for line in open(kf):
        if line.startswith("#") or not line.strip(): continue
        cid,cond,reason=line.rstrip("\n").split("\t")[:3]
        k,op,val=cond.replace("!=","\t!=\t").replace("==","\t==\t").split("\t")
        cur={"libzstd":zv}.get(k,"")
        if (op=="!=" and cur!=val) or (op=="==" and cur==val):
            for r in recs:
                if r.get("claim_id")==cid and r.get("verdict")=="fail": r["verdict"]="known-deviation"; r["deviation"]="%s: %s"%(cond,reason)
vs=[r["verdict"] for r in recs]; c=lambda v: vs.count(v)
prov={"tag":tag,"commit":sha,"date":datetime.datetime.utcnow().strftime("%Y-%m-%dT%H:%MZ"),
      "host":platform.node(),"machine":platform.machine(),"cores":__import__("os").cpu_count(),
      "libzstd":zv,
      "compiler":subprocess.run(["g++","--version"],capture_output=True,text=True).stdout.split("\n")[0],
      "pass":c("pass"),"fail":c("fail"),"skipped":sum(c(v) for v in ("skipped-no-gpu","skipped-no-corpus","skipped-no-tool")),
      "declared":c("declared"),"unjudged":c("unjudged"),"known":c("known-deviation"),"build_failed":c("build-failed")}
json.dump({"provenance":prov,"result":body},open(out,"w"),indent=1)
print("%-14s %5d %5d %8d %9d %8d %6d"%(tag,prov["pass"],prov["fail"],prov["skipped"],prov["declared"],prov["unjudged"],prov["known"]))
sys.exit(1 if prov["fail"] or prov["build_failed"] else 0)
PY
  [ $? = 0 ] || FAILS=$((FAILS+1))
done
echo "results in $OUT; worktrees in $WT"; [ "$FAILS" = 0 ]
