// =============================================================================
// lz/aceapex/cuda/aceapex_cuda.h
// ACEAPEX_CG — GPU codec variant for lzbench ("GPU profile": rANS container).
// Pure C interface; implementation in aceapex_cuda.cu (requires CUDA + nvcomp).
// =============================================================================
#pragma once
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Raw match-phase output of the ACEAPEX encoder (filled by aceapex_encode_raw
// in lz/aceapex/aceapex_api.cpp; all buffers malloc'd, caller frees).
typedef struct {
    uint8_t *lit, *off, *len, *cmd;     // concatenated per-block raw streams
    size_t   lit_sz, off_sz, len_sz, cmd_sz;
    void    *boffs;                     // BlockOffsets[num_blocks] (opaque here)
    size_t   num_blocks;
    uint32_t block_size;
    uint64_t orig_size;
} aceapex_raw_t;

// Implemented in lz/aceapex/aceapex_api.cpp (same TU as the encoder).
// block_size: 0 = encoder default; else forced (e.g. 16384 / 32768 / 65536).
int aceapex_encode_raw(const void* src, size_t src_size, int threads,
                       uint32_t block_size, aceapex_raw_t* out);
void aceapex_raw_free(aceapex_raw_t* r);

// ---- CUDA codec (implemented in aceapex_cuda.cu) ----------------------------
// Returns 1 if a CUDA device + nvcomp are usable at runtime, else 0.
int aceapex_cg_available(void);

// Compress src -> GPU-profile container ("ACEGPU4": header + BlockOffsets +
// chunk table + rANS chunks). Returns container size, or 0 on error/overflow.
// block_size: 0 = default (16384). threads: CPU LZ phase threads.
int64_t aceapex_cg_compress(const void* src, size_t src_size,
                            void* dst, size_t dst_capacity,
                            uint32_t block_size, int threads);

// Decompress container -> dst (host buffer). Full host<->host path:
// H2D(compressed) + nvcomp-ANS(device) + warp decode(device) + D2H(output).
// Returns orig_size, or 0 on error.
// Env ACEAPEX_CUDA_TIMING=1: print one-time stage breakdown to stderr.
int64_t aceapex_cg_decompress(const void* src, size_t src_size,
                              void* dst, size_t dst_capacity);

// Free cached device buffers / context (deinit).
void aceapex_cg_release(void);

#ifdef __cplusplus
}
#endif
