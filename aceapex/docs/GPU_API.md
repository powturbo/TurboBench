# GPU decoder: C ABI (`src/aceapex_gpu.h`)

Gate 5 of ROADMAP: a library other programs link, not only the measurement tool. Header `src/aceapex_gpu.h`
(C, `extern "C"`, the only CUDA type is `cudaStream_t`), implementation `src/aceapex_gpu_lib.cu` (device and plan)
and `src/aceapex_gpu_abi.cpp` (argument checks, last error, version: host C++), shared library `libaceapex_gpu.so.1`, host plan
`src/aceapex_gpu_plan.h` (plain C++), device kernels `src/aceapex_gpu_kernels.cuh` (shared with the tool
`aceapex_gpu.cu`). Example: `examples/gpu_decode.cu` (29 lines).

## Two phases

```c
/* 1. host, once per archive */
unsigned aceapex_gpu_version(void);                          /* ACEAPEX_GPU_API_VERSION the library was built with */
aceapex_gpu_plan* aceapex_gpu_plan_create(const void* h_archive, size_t in_bytes, uint64_t flags);   /* bits of ACEAPEX_GPU_PLAN_FLAGS */
int     aceapex_gpu_last_error(void);
size_t  aceapex_gpu_temp_bytes(const aceapex_gpu_plan*);
size_t  aceapex_gpu_range_temp_bytes(const aceapex_gpu_plan*, uint64_t max_length);
size_t  aceapex_gpu_output_bytes(const aceapex_gpu_plan*);

/* 2. device, asynchronous, as often as needed */
int aceapex_gpu_decompress_async(const aceapex_gpu_plan*, const void* d_in,
                                 void* d_out, void* d_temp, int* d_status, unsigned flags, cudaStream_t);
int aceapex_gpu_decompress_range_async(const aceapex_gpu_plan*, const void* d_in,
                                       uint64_t offset, uint64_t length,
                                       void* d_out, void* d_temp, int* d_status, cudaStream_t);
void aceapex_gpu_plan_destroy(aceapex_gpu_plan*);
```

- `plan_create` reads a host copy of the whole archive: header, block table and every chunk table (the
  open DNA pack keeps a small header in each literal chunk, so "only the head" is not enough). It checks the
  framing (the checks of the CPU decoders), builds the job list and uploads it with the block table to the
  device - its only allocation and copy. It keeps no pointer into `h_archive`. With
  `ACEAPEX_GPU_VALIDATE_ZSTD` it also decodes every zstd frame with libzstd on all host threads (next section).
- The async calls launch kernels on `stream` and return. They make no allocation, no host-to-device copy of
  host memory and no synchronization: the job descriptors are copied from the plan into `d_temp` by a kernel
  that adds the addresses of `d_in` and `d_temp` ("fixup"). The return value reports host-side errors only
  (arguments, a launch failure); the result of the decode is `*d_status` once the stream gets there.
- A plan is read-only after creation. Several streams may decode with one plan at once, each with its own
  `d_temp`. One `d_temp` per call in flight.
- `d_in`: the archive bytes on the device, any alignment. `d_temp`: `temp_bytes` (range:
  `range_temp_bytes(max_length)`), 256-byte aligned. `d_out`: `output_bytes`, or `length` for a range.

## Range decode

`decompress_range_async(offset, length)` decodes only the blocks covering `[offset, offset+length)` and, in
each stream, only the chunks those blocks use: on the host, from the plan, it picks a contiguous index range
of every job array (all of them are ordered by stream and chunk) - O(log n), no device transfer - then
launches the same kernels on those ranges, decodes the blocks into a window after the temp layout and copies
the requested bytes into `d_out`. nvCOMP has no equivalent: it decodes whole frames.

## Untrusted archives

Use the open profile (`AX_PROFILE=open` with the CLI - environment variables only in builds with `ACEAPEX_ENV_TUNING`,
[ENVIRONMENT.md](ENVIRONMENT.md) -: no zstd frame, every step on our own kernels, all bounded and judged on the
CPU), or create the plan with `ACEAPEX_GPU_VALIDATE_ZSTD`. Reason: nvCOMP 5.3.0.16 does not finish on some corrupt
zstd frames. Repro in `verify/repro/` (README.md there): T2T frame 150180 with one flipped byte inside its
compressed block - libzstd 1.5.5 reports `Data corruption detected`, the frame and block headers are valid, and
nvCOMP alone on that one frame (batch of 1, `scripts/nvcomp_frame_repro.cu`) is still running after 60 s on
Blackwell. A kernel that never returns cannot be stopped and no status can report it, so the check has to come
before nvCOMP: the flag decodes every frame with libzstd on the host and refuses the archive (NULL,
`ACEAPEX_GPU_E_ARCHIVE`) if one fails or decodes to another size; nvCOMP then only sees frames libzstd decoded.
Cost on ace-core (16 threads, libzstd 1.5.5), T2T default profile, 191 495 frames, 1.44 GB of frame output:
78 ms (1 thread: 853 ms), plan without it 19 ms. Off by default: an archive from a trusted source decodes without
the host pass. Judge: claim `head_gpu_zstd_validate`.

## Fail-closed

- Framing errors (header, block table, chunk tables, open-pack headers, zstd frame and block headers -
  magic, no dictionary, Frame_Content_Size == the expected size, Block_Size <= Block_Maximum_Size, the last block
  ending the frame): `plan_create` returns NULL, `aceapex_gpu_last_error()` = `ACEAPEX_GPU_E_ARCHIVE`. Damage
  inside a compressed zstd block passes these checks; nvCOMP reports it per frame (`_ZSTD`). A corrupt frame
  nvCOMP itself does not finish on cannot be stopped from outside a kernel: open question on Blackwell
  (verify/repro/README.md).
- Content errors found on the device set bits of `*d_status`: `ACEAPEX_GPU_STATUS_PIECE` (a rANS chunk or
  piece, spec 3.1.1), `_OPEN` (an open DNA pack, spec 3.4), `_ZSTD` (a frame failed or decoded to another
  size), `_MATCH` (a block's tokens did not decode to exactly its size), `_LIMIT` (a kernel loop hit its step
  limit). The call does not crash and does not read or write outside its buffers (plan limits; CPU judge
  below, also under ASan/UBSan).
- No loop runs on data alone: every kernel loop is bounded by plan values (chunk sizes, counts) or by the
  bytes it consumes. The match kernel checks each length against the room left in the block (before 2026-10-01
  it checked `out_pos + l`, which a corrupt varint near 2^32 wrapped below the block size: the copy then ran
  gigabytes past the block) and stops after `cmd_sz + 1` steps (every token consumes a command byte), setting
  `_LIMIT`. A varint longer than 5 bytes or cut by the stream end fails the block (`_MATCH`). Descriptor sizes
  are 64-bit; a chunk or piece above 0xFFFFFF00 bytes is refused by the plan (the per-chunk steps count in
  32 bits). CPU judge (`head_gpu_flip_emu`): 1000 byte flips of the streams under the plan of the intact
  archive (as on the device), a failed zstd frame leaving random bytes: none over the step limit, no copy
  outside its block, every changed output flagged by the status or by XXH3 (0 silent; the rest decode to
  the original); the wrapped length is refused.
- Flag `ACEAPEX_GPU_VERIFY_XXH3` (full decode): the device computes XXH3_64bits of the output and compares it
  with the archive header; a difference sets `ACEAPEX_GPU_STATUS_HASH`. This is what catches bytes stored raw
  in the archive (literal runs, raw pieces, zstd raw blocks), which carry no check of their own: on the
  Blackwell run 74fc806 the zstd profile decoded 20 of 20 byte flips to wrong bytes with status 0 without it.
  XXH3's long path adds stripe terms per 1 KiB block and scrambles the 8 accumulators after each block; the
  additions do not depend on the accumulators, so the block terms are computed in parallel and only the
  scramble chain (one step per KiB, 8 lanes) runs sequentially (`src/ax_xxh3.h`, judged on the CPU against
  `XXH3_64bits`: claim `head_gpu_xxh3_emu`). A range decode cannot be checked this way: the hash covers the
  whole original.

## Profiles and nvCOMP

The open profile (`AX_PROFILE=open`: rANS tokens, open literal chunks) needs nothing but CUDA. The default
and rANS-token profiles contain zstd frames; they need the library built with `-DACEAPEX_GPU_NVCOMP` and
nvCOMP 5 (`-l:libnvcomp.so.5`), otherwise `plan_create` returns NULL with `ACEAPEX_GPU_E_NVCOMP`.
Match kernel: v7-RA with 32 lanes per block (the value the tool's probe picked on T4, L4, A100 and the
Blackwell); grid from the occupancy of the current device at `plan_create`.

## Outputs larger than the card (2.3)

`aceapex_gpu_plan_create_blocks(h, n, b0, b1, flags)` builds a plan of blocks [b0, b1) only: the streams restricted to the
chunks those blocks use, a temp sized for them (`aceapex_gpu_temp_bytes`), the output = the window of those blocks
(`aceapex_gpu_output_bytes`), and the jobs reading the archive slice [in_lo, in_hi) of `aceapex_gpu_plan_input_window` -
pass `d_in` = the archive on the device + in_lo, or upload just that slice. Blocks are independent (every match lies in
its block, `results/dep-range-2026-10-01.log`), so batches decode in any order and concatenate to the original.
`scripts/gpu_stream.cu` decodes a corpus in batches sized from the free memory, two in flight, hashing the output on the
host against the header; the CPU judge runs 5 batches of every fixture through the plan emulator (`head_gpu_plan_emu`).
The XXH3 flag and the range call are refused on a block-range plan (the hash covers the whole original).

## Windows batch for data loaders (2.3)

`aceapex_gpu_decompress_windows_async(plan, d_in, d_offsets, n, window_len, d_out, d_temp, d_status, stream)` decodes n
windows of `window_len` bytes at offsets held in device memory into one buffer (window i at i x window_len) - the input of
a model step. The selection runs on the device: the blocks the windows cover are marked, the chunks of those blocks picked,
only those rANS / open / stored jobs and those blocks decoded (one slot per block), the windows gathered; the host only
launches. Temp: `aceapex_gpu_windows_temp_bytes(plan, n, window_len)`. Open-profile archives (no zstd frame). A window past
the end sets `ACEAPEX_GPU_STATUS_RANGE` and is not written. CPU judge: `scripts/gpu_windows_emu.cpp` (claim
head_gpu_windows_emu); on the card: T-H5 in `scripts/gpu_run.sh`.

## Stability

From 2.3 (`ACEAPEX_GPU_API_VERSION` 20300) the C ABI is frozen: the signatures, structures' absence (the plan is
opaque), return codes, status bits and flag values in `src/aceapex_gpu.h` do not change; later versions only add
(new functions, new flag bits, new status bits, new return codes). Flags are checked against the masks
`ACEAPEX_GPU_PLAN_FLAGS` / `ACEAPEX_GPU_DECODE_FLAGS`: an unknown bit is `ACEAPEX_GPU_E_ARGS`, so a caller built
against a newer header learns at once that this library lacks the feature instead of getting a silent default.
`aceapex_gpu_version()` returns the version the library was built with (compare with the header's at run time).
The shared library is `libaceapex_gpu.so.1` (SONAME): the `.so.1` stays as long as the ABI only grows; a break
would be `.so.2` and a new major version. Symbols not declared in the header (`aceapex_gpu_debug_phase_hook`, the
test hook of `scripts/gpu_api_test.cu`) are not part of the ABI. Judge: claim `head_gpu_abi` (the host part,
`src/aceapex_gpu_abi.cpp`, without CUDA: 63 unknown flag bits refused before the plan builder runs, version
macros and `aceapex_gpu_version()` agree).

## Build

```sh
make gpu-lib NVCOMP=<nvcomp dir>      # libaceapex_gpu.so.1 (SONAME) + libaceapex_gpu.so; without NVCOMP: open profile only
nvcc -std=c++17 -O3 -arch=sm_XX -Isrc examples/gpu_decode.cu -L. -laceapex_gpu -o gpu_decode   # + -Xlinker -rpath-link=<nvcomp>/lib64
# or the sources into a program:
nvcc -std=c++17 -O3 -arch=sm_XX -Isrc [-DACEAPEX_GPU_NVCOMP -I<nvcomp>/include] -c src/aceapex_gpu_lib.cu src/aceapex_gpu_abi.cpp
nvcc -std=c++17 -O3 -arch=sm_XX -Isrc examples/gpu_decode.cu src/aceapex_gpu_lib.cu src/aceapex_gpu_abi.cpp -o gpu_decode
```

## Checks

- Without a GPU (judge, claim `head_gpu_plan_emu`): `scripts/gpu_plan_emu.cpp` builds the plan and executes
  it job by job on the CPU in the order the library launches the device (raw copies, zstd frames, rANS pieces,
  both DNA unpacks, the match kernel with one lane) into a temp buffer with the plan's layout: 33 archives
  (conformance fixtures, chr1 slice, four profiles encoded on the spot) bit-perfect; 1 320 random ranges on a
  zeroed temp with only the selected jobs (a job the selection misses shows up as zeros); 1 980 one-byte
  mutations refused by the planner or run inside their buffers (also under ASan/UBSan).
- With a GPU (`scripts/colab_gpu_open.sh`, program `scripts/gpu_api_test.cu`): full decode bit-perfect and
  timed against the measurement tool on the same archive, the same with the XXH3 check (its cost), random windows 1 B .. 1 MiB against the original,
  single-byte flips of the device copy (caught / silent / harmless counts, no CUDA error), the example on the
  open archive built without nvCOMP.

## Numbers

Only measured figures go here, with the log they come from. The measurement tool on the RTX PRO 6000
Blackwell (`results/colab-2026-09-30-rtx-pro-6000-blackwell-27b61b1.log`, on-device, median of 3): chr1
open profile 2.826 ms against 3.501 ms for the zstd profile through nvCOMP (x1.24), T2T 27.135 against
35.542 ms (x1.31). The library, same GPU, chr1 (`results/colab-2026-09-30-rtx-pro-6000-blackwell-74fc806-gpu-capi.log`,
on-device, median of 3): open profile 2.474 ms (the tool 2.829 ms on the same archive), zstd profile 3.358 ms
(3.518 ms); 200 of 200 random windows equal to the original, 16 KiB window 0.691 ms (open) / 1.126 ms (zstd).
Cost of the XXH3 check, first version (`results/colab-2026-10-01-rtx-pro-6000-blackwell-f2e16b7-xxh3.log`): +9.970 ms on
chr1 zstd (3.297 -> 13.267 ms), +9.850 ms on chr1 open; byte flips caught 20 of 20 on both profiles with the check
(zstd: 0 of 20 without it). Second version (key table, 64-bit loads, chain prefetch 32 blocks ahead): pending.
