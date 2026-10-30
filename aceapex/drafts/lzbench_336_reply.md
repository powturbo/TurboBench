# Reply draft for lzbench #336 (not posted)

Thanks, that was a real problem. aceapex 2.2.2 no longer reads the environment: all 16 variables
(ACEAPEX_BS, ACEAPEX_DUMP, AX_ATT, AX_ENC, AX_HLOG, AX_LIT, AX_MINL, AX_NOFLAT, AX_PROFILE, AX_SKIP, AX_TOK,
FSE_CHUNK, LIT_CHUNK, LIT_LANES, LIT_LANES_DEC, LIT_LEVEL) go through one ax_getenv(), which returns NULL unless the
build defines ACEAPEX_ENV_TUNING. Only the aceapex CLI and our own test tools define it; the vendored library in
lzbench does not.

- Your five settings on silesia/xml (level 1, -I1): 724378 bytes in all five now (2.2.1 gave 724378 / 853990 /
  812809 / 734621 / 829935).
- LIT_LANES_DEC no longer starts decode threads: under strace, lzbench -eaceapex,1,3 -i1,1 with LIT_LANES_DEC=8 and
  LIT_LANES=8 makes one clone, the same one lzbench makes with -ememcpy alone (2.2.1: 7 more).
- Default archive bytes are unchanged (18 of 18 archives identical to 2.2.1: silesia/xml, chr1, enwik8, levels 1-3,
  1 and 8 threads).
- FASTEST now lists aceapex,3 instead of aceapex,1.

The update is one commit on the PR branch: vendored files byte-identical to aceapex v2.2.2 (src/ at 911bbdf).
