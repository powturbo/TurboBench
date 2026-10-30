# T-H4 "hang" (Colab Blackwell, 02.10, commit 855d5a9): copy 918 of 10 000

`scripts/gpu_h100_tests.cu stress` corrupts the open-profile archive of a 16 MiB chr1 slice
(`tail -c +100000001 chr1.fa | head -c 16777216`, `ACEAPEX_BS=16384 LIT_CHUNK=65536 AX_PROFILE=open`, 4 469 784 B,
md5 84eba8783bbb5f91508308429c4c9d1c; the open profile is byte-deterministic) with `std::mt19937_64 g(4242)`; copy
`it` uses kind `it % 7`. Copy 918 is kind 1 (1-8 random bytes): eight bytes changed, `th4_copy00918.diff`
(offset, old, new). One of them alone does it: offset 3 375 164, 0x00 -> 0xB2 - the high byte of `ncse` in the open
DNA pack header of literal chunk 193: ncse = 2 986 345 694 for raw = 65 536.

The plan accepted it (axo_parse checked ncse != 0 only): 15 GB of temp, a rANS piece of 2.99 G symbols -
93 M warp groups in k_rans with no early exit, 11.7 M block trips in k_open_cg. A bounded but minutes-long kernel:
the T-H4 watchdog (10 s) called it a hang. `th4_copy00918.aet` is the whole corrupt copy.

Fix (02.10): `ncse <= 5 (raw + 1)`, `ngap <= 5 nexc` (a run or a gap is one LEB128 of <= 5 bytes) in axo_parse (GPU
plan: refused, ACEAPEX_GPU_E_ARCHIVE), axo_dna_decode (CPU, C99, Python copies) and k_open_cg; k_rans stops within
256 groups once the piece's words run out. Check: `scripts/gpu_stress_emu.cpp` replays the 10 000 copies through the
plan and the CPU executor of the device jobs (claim head_gpu_stress_emu).
