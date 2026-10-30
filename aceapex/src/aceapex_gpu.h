/* aceapex_gpu.h - GPU decoder of ACEPX2 archives, C ABI (gate 5; docs/GPU_API.md).
 *
 * Two phases, as batched GPU codecs do:
 *   1. on the host, once per archive: aceapex_gpu_plan_create parses the header and every chunk table of a
 *      host copy of the archive, checks the framing and uploads its job tables to the device (the only
 *      allocation and the only copy this library makes on its own).
 *   2. on a stream, as often as needed: aceapex_gpu_decompress_async / _range_async launch the decode of a
 *      device copy of the same archive. They allocate nothing, copy nothing from the host and never
 *      synchronize; the caller provides the temp buffer. The result is valid when the stream reaches it.
 *
 * Fail-closed: framing errors are found by plan_create (NULL, see aceapex_gpu_last_error). Content errors
 * found on the device - a rANS piece, an open DNA pack, a zstd frame or a block whose tokens do not decode
 * to its size - are reported in *d_status (device memory, one int, written at the end of the call; 0 =
 * success, else a bitmask of ACEAPEX_GPU_STATUS_*). A decode never reads or writes outside the buffers it
 * was given. Bytes stored raw inside the archive (literal runs, raw pieces, zstd raw blocks) carry no check
 * of their own: only the archive's XXH3 of the whole original catches them - flag ACEAPEX_GPU_VERIFY_XXH3
 * computes it on the device (ACEAPEX_GPU_STATUS_HASH). A range decode cannot check it (the hash covers the
 * whole original).
 *
 * Buffers: d_in = the archive bytes (in_bytes, any alignment); d_temp = aceapex_gpu_temp_bytes() bytes
 * (range: aceapex_gpu_range_temp_bytes()), 256-byte aligned (cudaMalloc is); d_out = the original
 * (aceapex_gpu_output_bytes()) or the range (length bytes). One temp buffer per call in flight.
 * A plan is read-only after creation: several streams may use it at once, each with its own temp.
 * Archives of every profile; zstd frames (the default and rANS-token profiles) need the library built
 * with nvCOMP (-DACEAPEX_GPU_NVCOMP), the open profile (AX_PROFILE=open) needs nothing but CUDA.
 *
 * Untrusted archives: the open profile, or plan_create with ACEAPEX_GPU_VALIDATE_ZSTD. nvCOMP 5.3 does not
 * finish on some corrupt zstd frames (verify/repro/README.md: a kernel that never returns cannot be stopped,
 * so no status can report it); the flag decodes every zstd frame with libzstd on the host first and refuses
 * the archive if one fails, so nvCOMP only ever sees frames libzstd decoded to their exact size.
 */
#ifndef ACEAPEX_GPU_H
#define ACEAPEX_GPU_H
#include <stddef.h>
#include <stdint.h>
#if defined(__has_include)
#if __has_include(<cuda_runtime_api.h>)
#include <cuda_runtime_api.h>   /* cudaStream_t */
#else
typedef struct CUstream_st* cudaStream_t;   /* the CUDA definition: the header compiles without the toolkit */
#endif
#else
#include <cuda_runtime_api.h>
#endif
#ifdef __cplusplus
extern "C" {
#endif

/* version of this API (docs/GPU_API.md "Stability"): major*10000 + minor*100 + patch; aceapex_gpu_version() returns
   the value the library was built with. Shared library: libaceapex_gpu.so.1 (SONAME; `make gpu-lib`). */
#define ACEAPEX_GPU_API_VERSION_MAJOR 2
#define ACEAPEX_GPU_API_VERSION_MINOR 3
#define ACEAPEX_GPU_API_VERSION_PATCH 0
#define ACEAPEX_GPU_API_VERSION (ACEAPEX_GPU_API_VERSION_MAJOR * 10000 + ACEAPEX_GPU_API_VERSION_MINOR * 100 + ACEAPEX_GPU_API_VERSION_PATCH)

typedef struct aceapex_gpu_plan aceapex_gpu_plan;

/* return codes of the host calls (and aceapex_gpu_last_error after a NULL plan) */
#define ACEAPEX_GPU_OK          0
#define ACEAPEX_GPU_E_ARGS     -1   /* null pointer, temp not 256-aligned, empty or out-of-range region */
#define ACEAPEX_GPU_E_ARCHIVE  -2   /* header, block table or chunk framing invalid */
#define ACEAPEX_GPU_E_NVCOMP   -3   /* the archive has zstd frames and the library was built without nvCOMP */
#define ACEAPEX_GPU_E_CUDA     -4   /* a CUDA call failed (allocation or launch) */
#define ACEAPEX_GPU_E_RANGE    -5   /* region reads need block slices laid out back to back (every encoder writes them so) */

/* *d_status after a decode: 0 or a bitmask */
#define ACEAPEX_GPU_STATUS_PIECE   1   /* a rANS token chunk or piece failed its checks (spec 3.1.1) */
#define ACEAPEX_GPU_STATUS_OPEN    2   /* an open DNA pack failed its checks (spec 3.4) */
#define ACEAPEX_GPU_STATUS_ZSTD    4   /* a zstd frame failed or decoded to another size */
#define ACEAPEX_GPU_STATUS_MATCH   8   /* a block's tokens did not decode to exactly its size */
#define ACEAPEX_GPU_STATUS_HASH   16   /* ACEAPEX_GPU_VERIFY_XXH3: XXH3_64bits of the output != the archive header */
#define ACEAPEX_GPU_STATUS_LIMIT  32   /* a kernel loop hit its step limit (a broken invariant: stopped, output invalid) */
#define ACEAPEX_GPU_STATUS_RANGE  64   /* 2.3 windows batch: a window reaches past the end of the original (not written) */

/* flags of aceapex_gpu_plan_create (uint64_t); a bit outside ACEAPEX_GPU_PLAN_FLAGS: NULL, ACEAPEX_GPU_E_ARGS */
#define ACEAPEX_GPU_VALIDATE_ZSTD  UINT64_C(1)   /* decode every zstd frame with libzstd on the host (all hardware threads);
                                                    one that fails or has another size: NULL, ACEAPEX_GPU_E_ARCHIVE */
#define ACEAPEX_GPU_PLAN_FLAGS     (ACEAPEX_GPU_VALIDATE_ZSTD)

/* flags of aceapex_gpu_decompress_async; a bit outside ACEAPEX_GPU_DECODE_FLAGS: ACEAPEX_GPU_E_ARGS */
#define ACEAPEX_GPU_VERIFY_XXH3    1u  /* hash the whole output on the device and compare with the header (full decode only) */
#define ACEAPEX_GPU_DECODE_FLAGS   (ACEAPEX_GPU_VERIFY_XXH3)

unsigned aceapex_gpu_version(void);                          /* ACEAPEX_GPU_API_VERSION of the built library */
aceapex_gpu_plan* aceapex_gpu_plan_create(const void* h_archive, size_t in_bytes, uint64_t flags);
/* 2.3: a plan of blocks [b0, b1) only - for outputs larger than the device, decoded batch by batch (scripts/gpu_stream.cu).
   Its decode writes the window of those blocks (aceapex_gpu_output_bytes = the window) into d_out, needs a temp sized
   for those blocks' chunks only (aceapex_gpu_temp_bytes), and reads the archive bytes [in_lo, in_hi) given by
   aceapex_gpu_plan_input_window: pass d_in = (the whole archive on the device) + in_lo, or upload just that slice.
   ACEAPEX_GPU_VERIFY_XXH3 and the range call are refused on such a plan (E_ARGS): the hash covers the whole original. */
aceapex_gpu_plan* aceapex_gpu_plan_create_blocks(const void* h_archive, size_t in_bytes, uint32_t b0, uint32_t b1, uint64_t flags);
int     aceapex_gpu_plan_input_window(const aceapex_gpu_plan* plan, uint64_t* in_lo, uint64_t* in_hi);   /* whole plan: 0, in_bytes */
int     aceapex_gpu_last_error(void);                        /* of the last plan_create on this thread */
size_t  aceapex_gpu_temp_bytes(const aceapex_gpu_plan* plan);
size_t  aceapex_gpu_range_temp_bytes(const aceapex_gpu_plan* plan, uint64_t max_length);
size_t  aceapex_gpu_output_bytes(const aceapex_gpu_plan* plan);

int aceapex_gpu_decompress_async(const aceapex_gpu_plan* plan, const void* d_in,
                                 void* d_out, void* d_temp, int* d_status, unsigned flags, cudaStream_t stream);

/* bytes [offset, offset+length) of the original into d_out (length bytes): only the blocks covering the
   range and, in each stream, only the chunks those blocks use are decoded */
int aceapex_gpu_decompress_range_async(const aceapex_gpu_plan* plan, const void* d_in,
                                       uint64_t offset, uint64_t length,
                                       void* d_out, void* d_temp, int* d_status, cudaStream_t stream);

/* 2.3, windows batch (training data loaders): n windows of window_len bytes of the original at the offsets in
   d_offsets (device memory, n x uint64) into d_out (n x window_len bytes, window i at i x window_len) - one call, the
   selection on the device: the blocks the windows cover are marked, the chunks of those blocks picked, only those jobs
   and blocks decoded (one slot per block), the windows gathered. No host work beyond the launches, no host copy, no
   synchronization; offsets may change between calls without a new plan. d_temp: aceapex_gpu_windows_temp_bytes(plan,
   n, window_len) bytes (>= the plan's temp). Archives of the open profile (no zstd frame); else ACEAPEX_GPU_E_ARGS.
   A window past the end is not written and sets ACEAPEX_GPU_STATUS_RANGE. */
size_t aceapex_gpu_windows_temp_bytes(const aceapex_gpu_plan* plan, uint64_t max_windows, uint32_t window_len);
int aceapex_gpu_decompress_windows_async(const aceapex_gpu_plan* plan, const void* d_in, const uint64_t* d_offsets,
                                         uint32_t n, uint32_t window_len, void* d_out, void* d_temp, int* d_status, cudaStream_t stream);

void aceapex_gpu_plan_destroy(aceapex_gpu_plan* plan);

#ifdef __cplusplus
}
#endif
#endif
