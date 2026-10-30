/* ax_rans_warp.h - per-lane steps of the one-warp-per-chunk rANS decoder (ADR-018, spec 3.1.1).
 *
 * The CUDA kernel k_rans in aceapex_gpu.cu runs these steps with __syncwarp / __ballot_sync /
 * warp scans between them; scripts/rans_warp_emu.cpp runs the same steps for lanes 0..31 in
 * turn on the CPU and compares the result with axr_decode (src/ax_rans.h), so the device code
 * is judged on a host without a GPU. Between two steps every lane has finished the previous one.
 *
 * Per chunk:
 *   stage     lanes copy the bitmap and up to 768 LEB128 bytes into sh.sym, zero sh.f
 *   rank      lane l owns bitmap byte l (symbols 8l..8l+7): popcount, exclusive scan,
 *             then sh.c[rank] = symbol (sh.c is the rank -> symbol map during parsing)
 *   leb       rounds of 32 bytes: a byte < 0x80 ends a value; ballot + popcount gives the
 *             value's rank j, the lane assembles it backwards (at most 3 bytes) into
 *             sh.f[symbol of rank j]; the lane of rank K-1 stores the end of the table
 *   sum/cum   lane l sums f[8l..8l+7], exclusive scan -> sh.c = cumulative frequencies;
 *             the total must be 4096
 *   fill      lane l writes sh.sym[128l..128l+127] (slot -> symbol)
 *   init      states (each >= 2^16), W, and the chunk size must be header + 2W exactly
 *   step      group g: lane l decodes symbol 32g+l if it exists; returns "needs a word"
 *   refill    mask = ballot(needs); word index = base + popcount(mask below l);
 *             base += popcount(mask); an index >= W is an error
 *   end       base == W and every state back at 2^16
 * Any failure sets the lane's `bad`; the caller makes it warp-uniform before branching.
 */
#ifndef AX_RANS_WARP_H
#define AX_RANS_WARP_H
#include <stdint.h>
#include "ax_rans.h"

#ifdef __CUDACC__
#define AXW_HD static inline __host__ __device__
#else
#define AXW_HD static inline
#endif

#define AXW_MIN   (32u + 128u + 4u)   /* smallest valid chunk: bitmap, states, W */
#define AXW_STAGE (32u + 768u)        /* bitmap + at most 256 values x 3 bytes */

struct AxwShared {
    uint8_t  sym[AXR_M];   /* staging bytes while parsing, then slot -> symbol */
    uint16_t f[256];       /* frequencies */
    uint16_t c[256];       /* rank -> symbol while parsing, then cumulative frequencies */
    uint32_t end;          /* offset of the first state after the frequency table */
};

AXW_HD uint32_t axw_popc(uint32_t v) {
#ifdef __CUDA_ARCH__
    return (uint32_t)__popc(v);
#else
    return (uint32_t)__builtin_popcount(v);
#endif
}
AXW_HD uint32_t axw_rd32(const uint8_t* p) { return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24); }
AXW_HD uint32_t axw_rd16(const uint8_t* p) { return (uint32_t)p[0] | ((uint32_t)p[1] << 8); }
AXW_HD uint32_t axw_lim(uint32_t sz) { return sz < AXW_STAGE ? sz : AXW_STAGE; }

AXW_HD void axw_stage(uint32_t lane, const uint8_t* src, uint32_t sz, AxwShared& sh) {
    for (uint32_t k = 0; k < 8; k++) sh.f[lane * 8 + k] = 0;
    const uint32_t lim = axw_lim(sz);
    for (uint32_t t = lane; t < lim; t += 32) sh.sym[t] = src[t];
}
AXW_HD uint32_t axw_rank_count(uint32_t lane, const AxwShared& sh) { return axw_popc(sh.sym[lane]); }
AXW_HD void axw_rank_write(uint32_t lane, AxwShared& sh, uint32_t r) {
    const uint32_t bm = sh.sym[lane];
    for (uint32_t b = 0; b < 8; b++) if (bm >> b & 1u) sh.c[r++] = (uint16_t)(lane * 8 + b);
}
/* round r covers staged bytes 32+32r .. 32+32r+31; lane's byte ends a value? */
AXW_HD bool axw_term(uint32_t lane, const AxwShared& sh, uint32_t r, uint32_t lim) {
    const uint32_t t = 32 + 32 * r + lane;
    return t < lim && sh.sym[t] < 0x80;
}
AXW_HD void axw_leb(uint32_t lane, AxwShared& sh, uint32_t r, bool term, uint32_t j, uint32_t K, bool& bad) {
    if (!term || j >= K) return;
    const uint32_t t = 32 + 32 * r + lane;
    uint32_t v = sh.sym[t], k = 1;
    while (k < 3 && t - k >= 32 && sh.sym[t - k] >= 0x80) { v = (v << 7) | (sh.sym[t - k] & 0x7Fu); k++; }
    if (t - k >= 32 && sh.sym[t - k] >= 0x80) bad = true;      /* a 4th byte: more than 21 bits */
    if (v == 0 || v > AXR_M) bad = true;
    sh.f[sh.c[j]] = (uint16_t)(v > AXR_M ? 0 : v);
    if (j == K - 1) sh.end = t + 1;
}
AXW_HD uint32_t axw_sum(uint32_t lane, const AxwShared& sh) {
    uint32_t s = 0; for (uint32_t k = 0; k < 8; k++) s += sh.f[lane * 8 + k]; return s;
}
AXW_HD void axw_cum(uint32_t lane, AxwShared& sh, uint32_t base) {
    for (uint32_t k = 0; k < 8; k++) { sh.c[lane * 8 + k] = (uint16_t)base; base += sh.f[lane * 8 + k]; }
}
/* needs sum(f) == 4096: then c[s] + f[s] > slot for the last s with c[s] <= slot */
AXW_HD void axw_fill(uint32_t lane, AxwShared& sh) {
    const uint32_t s0 = lane * (AXR_M / 32);
    uint32_t lo = 0, hi = 255;
    while (lo < hi) { uint32_t mid = (lo + hi + 1) >> 1; if (sh.c[mid] <= s0) lo = mid; else hi = mid - 1; }
    uint32_t s = lo;
    for (uint32_t k = 0; k < AXR_M / 32; k++) {
        const uint32_t slot = s0 + k;
        while (s < 255 && slot >= (uint32_t)sh.c[s] + sh.f[s]) s++;   /* bounded even if the sum check were bypassed */
        sh.sym[slot] = (uint8_t)s;
    }
}
AXW_HD bool axw_init(uint32_t lane, const uint8_t* src, uint32_t sz, uint32_t end, uint32_t& x, uint32_t& W, const uint8_t*& words) {
    if ((uint64_t)end + 132 > sz) { x = AXR_L; W = 0; words = src; return true; }
    x = axw_rd32(src + end + 4 * lane);
    W = axw_rd32(src + end + 128);
    words = src + end + 132;
    return x < AXR_L || (uint64_t)sz - end - 132 != 2ull * W;
}
AXW_HD bool axw_step(uint32_t lane, const AxwShared& sh, uint32_t g, uint32_t n, uint32_t& x, uint8_t* dst) {
    const uint32_t i = g * 32 + lane;
    if (i >= n) return false;
    const uint32_t slot = x & (AXR_M - 1), s = sh.sym[slot];
    dst[i] = (uint8_t)s;
    x = (uint32_t)sh.f[s] * (x >> AXR_PBITS) + slot - sh.c[s];
    return x < AXR_L;
}
AXW_HD void axw_refill(uint32_t lane, bool need, uint32_t mask, uint32_t base, uint32_t W, const uint8_t* words, uint32_t& x, bool& bad) {
    if (!need) return;
    const uint32_t idx = base + axw_popc(mask & ((1u << lane) - 1u));
    if (idx >= W) { bad = true; return; }
    x = (x << 16) | axw_rd16(words + 2 * (size_t)idx);
}
/* Windowed refill (AX_OPEN_SEQ 1, same values as axw_refill): the refill word is on the dependent chain of every
 * step, and a global load there costs hundreds of cycles; instead lane l keeps words wb+l (w0) and wb+32+l (w1) in
 * registers, a step takes its word from that 64-word window by shuffle (idx - wb < 64: base < wb + 32 at the step
 * start and at most 31 lanes below), and when base passes wb + 32 the window moves by 32 with one coalesced load
 * that the next 32 words do not wait for. Words past W read as 0 and are never used (idx >= W is bad, as before). */
AXW_HD uint32_t axw_wload(const uint8_t* words, uint32_t W, uint32_t idx) { return idx < W ? axw_rd16(words + 2 * (size_t)idx) : 0; }
/* the word lane `lane` needs this step: its index and position in the window */
AXW_HD uint32_t axw_widx(uint32_t lane, uint32_t mask, uint32_t base) { return base + axw_popc(mask & ((1u << lane) - 1u)); }
AXW_HD void axw_refill_v(bool need, uint32_t idx, uint32_t W, uint32_t v, uint32_t& x, bool& bad) {
    if (!need) return;
    if (idx >= W) { bad = true; return; }
    x = (x << 16) | v;
}
#endif
