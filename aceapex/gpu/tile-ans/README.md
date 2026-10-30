# ACEAPEX GPU full-pipeline decode (TILE-ANS) — D1 device-resident

Full device-resident decode: ANS entropy (DietGPU) + LZ match kernel, whole file, bit-perfect.
Entropy tile granularity DECOUPLED from LZ block_size (ANS on fixed 64KB tiles cut across
flat streams; match kernel keeps its own block_size).

## Measured result (PAPER5, 2026-07-20, pod H100 80GB, CUDA 12.4)
- **D1-dense: 200.4 GB/s** full-pipeline (+13% over fused 177.3 GB/s)
- data: NA12878_REAL.fastq md5 9af9ffaa (honest), tuned streams.bin ratio 3.970
- bit-perfect: FNV 513444748a087502
- PARSE WALL (second bottleneck, honest): ANS+parse 2.664ms / copy 2.694ms (49.7%/50.3%)
  -> command-parse is HALF of decode time.

## Files
- e2e_dense_HOST.cu — host driver, full-pipeline TILE-ANS (main D1 binary)
- d2p_dense_v4_282.cuh — dense two-core decode kernel (the +13% version)
- d2p_kernel.cuh — kernel helpers
- e2e_pipe_tile.cu — earlier tile-pipe variant
- gpu_hash_finder.cu — GPU match finder (encode side)
- nsight_sweep.sh — Nsight occupancy/L2/DRAM sweep by block_size
- silesia_all.sh — throughput+FNV over all 12 Silesia files

## Build (H100, sm_90)
nvcc -O3 -arch=sm_90 -I/path/to/dietgpu -o e2e_dense e2e_dense_HOST.cu /path/to/dietgpu/dietgpu/ans/GpuANSDecode.cu -lgpu_ans -ldietgpu_utils
Requires DietGPU (github.com/facebookresearch/dietgpu) built for libgpu_ans.

## Reproduce
Clone this repo at the recorded commit, get NA12878 by md5 9af9ffaa, encode to streams.bin,
run e2e_dense on it, verify FNV. See PAPER5_DENSE in private context for exact params.

## Storage-Engine KPIs (reproduce with storage_kpi.sh)
Measured on NVIDIA H100 80GB, block_size=16384, G=32, NA12878 (ERR194147, md5 9af9ffaa).
All bit-perfect (FNV vs original). Run: `bash storage_kpi.sh <fastq>`

| KPI | metric | value |
|-----|--------|-------|
| 1 Parallel decode | full-file device-resident | **201 GB/s** |
| 2 O(1) random access | latency 1 block / 1000 blocks | **295 us / 336 us** (flat) |
| 3 Random vs full | single block / whole file | **271 us / 4923 us** (18x) |
| 4 Block-independence | arbitrary blocks decode standalone | yes |
| 5 Encode granularity | block_size sets #blocks | 16K->65536, 256K->4096 |

**Interpretation.** Random-access latency to any region is ~0.3 ms and does NOT grow
with region size (KPI-2) — the access profile of uncompressed RAM, but the data is
100% compressed. This is the basis for a compressed-resident storage engine: store
everything compressed, random-access cost comparable to in-memory.

### Build v7ra (random-access / seek decoder)
nvcc -O3 -arch=sm_90 -o v7ra full_gpu_decode_v7_ra.cu
CLI: `v7ra <streams.bin> [original] [G=8|16|32] [start_block] [count]`
Omitting start/count decodes all blocks; giving them decodes ONLY [start, start+count).

## Honest scope (what is proven vs compatible-but-not-built)
- PROVEN by code here: parallel GPU decode, O(1) byte-range seek, block-independence,
  encode-time granularity — all bit-perfect on H100.
- COMPATIBLE, not yet built: GPUDirect-Storage NVMe->VRAM path (self-contained blocks
  make it possible; the cuFile integration is not in this repo, needs GDS-capable NVMe).
- NOT this codec: search/filter directly over compressed bytes WITHOUT decoding a block
  (that is FM-index territory; ACEAPEX decodes the target block fast, it does not query
  the compressed form in place).
