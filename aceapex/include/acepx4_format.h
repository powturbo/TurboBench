#pragma once
#include <cstdint>
#include <cstring>

// ACEPX4 Format Constants
// Binary layout identical to ACEPX2 — only offset stream semantics change
// ACEPX2: off = back-reference distance in OUTPUT buffer
// ACEPX4: off = absolute index in LITERAL BUFFER of block

static const char ACEPX4_MAGIC[8] = {'A','C','E','P','X','4','\0','\0'};
static const uint32_t ACEPX4_VERSION = 4;
static const uint32_t ACEPX4_FLAG_LIT_SEPARATED = (1u << 8);

inline bool acepx4_is_lit_separated(uint32_t version_field) {
    return (version_field & ACEPX4_FLAG_LIT_SEPARATED) != 0;
}

inline uint32_t acepx4_make_version(uint32_t ver, uint32_t flags) {
    return (ver & 0xFF) | (flags & 0xFFFFFF00u);
}

// ENCODER CONTRACT (compress_block, ACEPX4 mode)
// origin_lit[local_pos] = lit-buffer index (not output position)
//
// On literal emit:
//   origin_lit[local_pos] = lit_i;  // before lit_i++
//   lit_buf[lit_i++] = byte;
//
// On match emit:
//   src_local = local_pos - c_off;
//   base = origin_lit[src_local];
//   contiguous = true;
//   for fi in 0..c_len:
//     if origin_lit[src_local+fi] != base+fi: contiguous=false; break;
//   if contiguous:
//     emit base as offset (lit-buffer index)
//   else:
//     literalize (v1) — emit as literals

// DECODER CONTRACT (decompress_streams, ACEPX4 mode)
// lit_block_base = global_lit + bo.lit_off  (immutable, per-block slice)
//
// Phase A (sequential): parse cmd/off/len → build ops[] array
//   ops[i] = {dst_off, lit_idx, len}
//
// Phase B (parallel):
//   #pragma omp parallel for
//   for each op in ops[]:
//     memcpy(dst + op.dst_off, lit_block_base + op.lit_idx, op.len)
//
// Parallel-safe because:
//   - all reads from immutable lit_block_base
//   - dst ranges non-overlapping (guaranteed by encoder)

// Helper: parallel-safe match copy from literal buffer
inline void acepx4_copy_match_litbuf(
    uint8_t* dst, size_t dst_off,
    const uint8_t* lit_base, uint32_t lit_idx,
    uint32_t len)
{
    memcpy(dst + dst_off, lit_base + lit_idx, len);
}
