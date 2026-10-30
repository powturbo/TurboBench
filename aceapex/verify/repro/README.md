# Corrupt inputs that stopped a GPU run

`t2t_frame150180.flip.zst` - zstd frame 150180 of 191 495 of the T2T archive in the default profile
(`FSE_CHUNK=4096 ACEAPEX_BS=16384 LIT_CHUNK=65536`, libzstd 1.5.5, archive 822 393 156 B; T2T md5
cd1e52ce400c027ed0b7ab4b9d613f5a), with flip 1 of `scripts/gpu_api_test.cu` (seed 20261001, 60 ranges before
the flips): archive byte 638 003 993 = frame byte 291 (0x3f -> 0xc6), in the sequences section of its only
compressed block. The frame is the case mask of a DNA-pack literal chunk: decoded size 8192 B.
`t2t_frame150180.orig.zst` is the intact frame (1136 B each).

- libzstd 1.5.5 on the CPU: `Data corruption detected` (the intact frame decodes to 8192 B).
- Frame header and block headers pass the plan's check (`agp::zstd_frame_check`): the damage is inside the
  compressed block, so only a decoder sees it.
- Blackwell f87bf19: `gpu_api_test t2t.zstd` stopped by its watchdog (300 s) in phase `flip decode`. Of the
  6 flips of that run this is the only one libzstd rejects (flips 2-6 land in literal frames that decode to
  other bytes).
- Blackwell 813bdf0, nvCOMP 5.3.0.16, `scripts/nvcomp_frame_repro.cu` (nvCOMP alone, batch of 1): the intact
  frame decodes; the flipped one is still running after 60 s (TIMEOUT). nvCOMP hangs on this frame by itself.
- Guard: `ACEAPEX_GPU_VALIDATE_ZSTD` (plan_create) decodes every frame with libzstd first and refuses the
  archive; docs/GPU_API.md "Untrusted archives". Judge: `head_gpu_zstd_validate` refuses this frame.

Run: `nvcomp_frame_repro verify/repro/t2t_frame150180.orig.zst 8192 verify/repro/t2t_frame150180.flip.zst 8192`
