#!/usr/bin/env bash
# HEAD: the working tree, not a worktree. Judge = the contract plus the region test on
# a fresh interactive archive, both from an empty environment. This is what CI runs.
set -uo pipefail
env -i PATH="$PATH" HOME="$HOME" GOLDEN="$GOLDEN" NO_DOWNLOAD=1 bash ./reproduce_paper5.sh >/dev/null 2>&1
make -s >/dev/null 2>&1
if [ ! -f "$GOLDEN/genome/chr1.fa" ]; then V=skipped-no-corpus; M="chr1.fa not under GOLDEN"; HA=-
else
env -i PATH="$PATH" LIT_CHUNK=65536 FSE_CHUNK=4096 ./aceapex c --in "$GOLDEN/genome/chr1.fa" --out /tmp/_vh.aet --threads 8 >/dev/null 2>&1
if env -i PATH="$PATH" HOME="$HOME" BIN=./aceapex bash scripts/region_test.sh "$GOLDEN/genome/chr1.fa" /tmp/_vh.aet 200 >/tmp/_vh.log 2>&1; then V=pass; M=$(tail -1 /tmp/_vh.log); else V=fail; M=$(tail -1 /tmp/_vh.log); fi
HA=$(sha256sum /tmp/_vh.aet | cut -c1-16)
fi
HB=$(sha256sum ./aceapex | cut -c1-16)
bash scripts/fixture_test.sh > /tmp/_vh_fx.tsv 2>/dev/null
bash scripts/cdec_test.sh >> /tmp/_vh_fx.tsv 2>/dev/null
bash scripts/arm_test.sh >> /tmp/_vh_fx.tsv 2>/dev/null
bash scripts/stdin_test.sh >> /tmp/_vh_fx.tsv 2>/dev/null
bash scripts/portability_test.sh >> /tmp/_vh_fx.tsv 2>/dev/null
bash scripts/fast_test.sh >> /tmp/_vh_fx.tsv 2>/dev/null
python3 - "$V" "$M" "$HA" "$HB" <<'PY'
import json,sys
d=json.load(open('results.json')); c=d.setdefault('claims',[])
c.append({"claim_id":"head_archive_sha256_chr1_interactive","level":"M","expected":"-","tolerance":"-","measured":sys.argv[3],"verdict":"declared","command":"sha256 of chr1 interactive archive, first 16 hex; same-libzstd comparison only"})
c.append({"claim_id":"head_binary_sha256","level":"M","expected":"-","tolerance":"-","measured":sys.argv[4],"verdict":"declared","command":"sha256 of ./aceapex built by make, first 16 hex"})
EX=open('verify/fixtures/chr1_4MiB.sha256').read().strip()[:16]+'..'
for l in open('/tmp/_vh_fx.tsv'):
    cid,v,m=l.rstrip('\n').split('\t')
    c.append({"claim_id":cid,"level":"R","expected":EX if 'fixture_decode_zstd' in cid else 'bytes==',
      "tolerance":"0","measured":m,"verdict":v,"command":"scripts/fixture_test.sh"})
c.append({"claim_id":"head_region_200","level":"R","expected":"bad=0","tolerance":"0","measured":sys.argv[2],"verdict":sys.argv[1],"command":"scripts/region_test.sh chr1 interactive 200"})
json.dump(d,open(__import__('os').environ['RECORDS'],'w'))
PY
git checkout -q -- results.json 2>/dev/null; rm -f /tmp/_vh.aet /tmp/_vh.log /tmp/_vh_fx.tsv
