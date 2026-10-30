/* ax_vec.h - 16-byte stores of the GPU kernels (src/aceapex_gpu_kernels.cuh, AX_VEC 1), host/device, so the CPU
 * judge (scripts/gpu_plan_emu.cpp) runs the same code lane by lane:
 *   axv_copy16   one lane's share of a non-overlapping copy of l bytes by G lanes: bytes up to d's 16-byte
 *                boundary, then one 16-byte store per lane step assembled from aligned 32-bit source words
 *                (funnel shift; reads up to 3 bytes past the source run: the callers' buffers have slack), the tail
 *   axv_unpack16 16 bases of the 2-bit DNA pack with their case bits (positions i0..i0+15, i0 a multiple of 16)
 */
#ifndef AX_VEC_H
#define AX_VEC_H
#include <stdint.h>
#include <string.h>
#ifdef __CUDACC__
#define AXV_HD static inline __host__ __device__
#else
#define AXV_HD static inline
#endif

AXV_HD uint32_t axv_fsr(uint32_t lo, uint32_t hi, uint32_t b) {   /* __funnelshift_r for b < 32 */
#ifdef __CUDA_ARCH__
    return __funnelshift_r(lo, hi, b);
#else
    return (uint32_t)((((uint64_t)hi << 32) | lo) >> b);
#endif
}
AXV_HD void axv_store16(uint8_t* d, uint32_t x, uint32_t y, uint32_t z, uint32_t w) {   /* d 16-byte aligned */
#ifdef __CUDA_ARCH__
    *(uint4*)d = make_uint4(x, y, z, w);
#else
    const uint32_t v[4] = {x, y, z, w}; memcpy(d, v, 16);
#endif
}
AXV_HD void axv_copy16(uint8_t* d, const uint8_t* s, uint32_t l, uint32_t lg, uint32_t G) {
    uint32_t head = (uint32_t)((16 - ((uintptr_t)d & 15)) & 15); if (head > l) head = l;
    for (uint32_t i = lg; i < head; i += G) d[i] = s[i];
    const uint32_t n16 = (l - head) >> 4; uint8_t* d16 = d + head; const uint8_t* s16 = s + head;
    const uint32_t sh = (uint32_t)((uintptr_t)s16 & 3); const uint32_t* w = (const uint32_t*)(s16 - sh);
    for (uint32_t k = lg; k < n16; k += G) { const uint32_t* q = w + 4 * k;
        if (sh) { const uint32_t a0 = q[0], a1 = q[1], a2 = q[2], a3 = q[3], a4 = q[4], b = 8 * sh;
            axv_store16(d16 + 16 * k, axv_fsr(a0, a1, b), axv_fsr(a1, a2, b), axv_fsr(a2, a3, b), axv_fsr(a3, a4, b)); }
        else axv_store16(d16 + 16 * k, q[0], q[1], q[2], q[3]); }
    for (uint32_t i = head + 16 * n16 + lg; i < l; i += G) d[i] = s[i];
}
AXV_HD void axv_unpack16(const uint8_t* seq, const uint8_t* cse, uint64_t i0, uint32_t w[4]) {
    const uint32_t m = ((uint32_t)cse[i0 >> 3] << 8) | cse[(i0 >> 3) + 1];
    for (uint32_t q = 0; q < 4; q++) { const uint8_t v = seq[(i0 >> 2) + q]; uint32_t x = 0;
        for (uint32_t k = 0; k < 4; k++) { uint32_t b = (0x54474341u >> (8 * ((v >> (6 - 2 * k)) & 3))) & 0xFFu;
            if (m & (0x8000u >> (4 * q + k))) b |= 0x20;
            x |= b << (8 * k); }
        w[q] = x; }
}
#endif
