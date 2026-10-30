/* ax_xxh3.h - XXH3_64bits (seed 0, default secret) split for a GPU, host and device (src/xxhash.h is the
 * reference; scripts/xxh3_split_emu.cpp checks this file against it on the CPU).
 *
 * The long path (len > 240) of XXH3 walks 1024-byte blocks (16 stripes of 64 bytes): per block it ADDS a
 * stripe term to each of 8 accumulator lanes, then scrambles every lane (x ^= x >> 47; x ^= key; x *= P32_1).
 * The additions inside a block do not depend on the accumulators, so the block term S_b[lane] is computed for
 * all blocks in parallel (axh_block_lane); only the chain acc = scramble(acc + S_b) is sequential, one thread
 * per lane (axh_chain_step). Then the last partial block, the last stripe and the merge (axh_finish).
 * Inputs of at most 240 bytes take XXH3's short paths (axh_short), one thread.
 */
#ifndef AX_XXH3_H
#define AX_XXH3_H
#include <stdint.h>
#include <string.h>

#ifdef __CUDACC__
#define AXH_HD static inline __host__ __device__
#define AXH_C __constant__ const
#else
#define AXH_HD static inline
#define AXH_C static const
#endif

#define AXH_P32_1 0x9E3779B1u
#define AXH_P32_2 0x85EBCA77u
#define AXH_P32_3 0xC2B2AE3Du
#define AXH_P64_1 0x9E3779B185EBCA87ull
#define AXH_P64_2 0xC2B2AE3D27D4EB4Full
#define AXH_P64_3 0x165667B19E3779F9ull
#define AXH_P64_4 0x85EBCA77C2B2AE63ull
#define AXH_P64_5 0x27D4EB2F165667C5ull
#define AXH_MX1   0x165667919E3779F9ull
#define AXH_MX2   0x9FB21C651E98DF25ull
#define AXH_BLOCK 1024u            /* 16 stripes x 64 B with the 192-byte default secret */
#define AXH_STRIPE 64u

AXH_C uint8_t axh_secret[192] = {
    0xb8, 0xfe, 0x6c, 0x39, 0x23, 0xa4, 0x4b, 0xbe, 0x7c, 0x01, 0x81, 0x2c, 0xf7, 0x21, 0xad, 0x1c,
    0xde, 0xd4, 0x6d, 0xe9, 0x83, 0x90, 0x97, 0xdb, 0x72, 0x40, 0xa4, 0xa4, 0xb7, 0xb3, 0x67, 0x1f,
    0xcb, 0x79, 0xe6, 0x4e, 0xcc, 0xc0, 0xe5, 0x78, 0x82, 0x5a, 0xd0, 0x7d, 0xcc, 0xff, 0x72, 0x21,
    0xb8, 0x08, 0x46, 0x74, 0xf7, 0x43, 0x24, 0x8e, 0xe0, 0x35, 0x90, 0xe6, 0x81, 0x3a, 0x26, 0x4c,
    0x3c, 0x28, 0x52, 0xbb, 0x91, 0xc3, 0x00, 0xcb, 0x88, 0xd0, 0x65, 0x8b, 0x1b, 0x53, 0x2e, 0xa3,
    0x71, 0x64, 0x48, 0x97, 0xa2, 0x0d, 0xf9, 0x4e, 0x38, 0x19, 0xef, 0x46, 0xa9, 0xde, 0xac, 0xd8,
    0xa8, 0xfa, 0x76, 0x3f, 0xe3, 0x9c, 0x34, 0x3f, 0xf9, 0xdc, 0xbb, 0xc7, 0xc7, 0x0b, 0x4f, 0x1d,
    0x8a, 0x51, 0xe0, 0x4b, 0xcd, 0xb4, 0x59, 0x31, 0xc8, 0x9f, 0x7e, 0xc9, 0xd9, 0x78, 0x73, 0x64,
    0xea, 0xc5, 0xac, 0x83, 0x34, 0xd3, 0xeb, 0xc3, 0xc5, 0x81, 0xa0, 0xff, 0xfa, 0x13, 0x63, 0xeb,
    0x17, 0x0d, 0xdd, 0x51, 0xb7, 0xf0, 0xda, 0x49, 0xd3, 0x16, 0x55, 0x26, 0x29, 0xd4, 0x68, 0x9e,
    0x2b, 0x16, 0xbe, 0x58, 0x7d, 0x47, 0xa1, 0xfc, 0x8f, 0xf8, 0xb8, 0xd1, 0x7a, 0xd0, 0x31, 0xce,
    0x45, 0xcb, 0x3a, 0x8f, 0x95, 0x16, 0x04, 0x28, 0xaf, 0xd7, 0xfb, 0xca, 0xbb, 0x4b, 0x40, 0x7e,
};

AXH_C uint64_t axh_k64[24] = {   /* the secret as 64-bit words at the 8-aligned offsets 0..184 (stripe keys 0..22, scramble keys 16..23) */
    0xbe4ba423396cfeb8ull, 0x1cad21f72c81017cull, 0xdb979083e96dd4deull, 0x1f67b3b7a4a44072ull,
    0x78e5c0cc4ee679cbull, 0x2172ffcc7dd05a82ull, 0x8e2443f7744608b8ull, 0x4c263a81e69035e0ull,
    0xcb00c391bb52283cull, 0xa32e531b8b65d088ull, 0x4ef90da297486471ull, 0xd8acdea946ef1938ull,
    0x3f349ce33f76faa8ull, 0x1d4f0bc7c7bbdcf9ull, 0x3159b4cd4be0518aull, 0x647378d9c97e9fc8ull,
    0xc3ebd33483acc5eaull, 0xeb6313faffa081c5ull, 0x49daf0b751dd0d17ull, 0x9e68d429265516d3ull,
    0xfca1477d58be162bull, 0xce31d07ad1b8f88full, 0x280416958f3acb45ull, 0x7e404bbbcafbd7afull,
};
AXH_HD uint64_t axh_r64(const uint8_t* p) { uint64_t v; memcpy(&v, p, 8); return v; }   /* little-endian hosts and GPUs */
AXH_HD uint32_t axh_r32(const uint8_t* p) { uint32_t v; memcpy(&v, p, 4); return v; }
AXH_HD uint64_t axh_rotl(uint64_t x, int r) { return (x << r) | (x >> (64 - r)); }
AXH_HD uint64_t axh_swap64(uint64_t x) {
    return ((x << 56) & 0xff00000000000000ull) | ((x << 40) & 0x00ff000000000000ull) | ((x << 24) & 0x0000ff0000000000ull) |
           ((x << 8) & 0x000000ff00000000ull) | ((x >> 8) & 0x00000000ff000000ull) | ((x >> 24) & 0x0000000000ff0000ull) |
           ((x >> 40) & 0x000000000000ff00ull) | ((x >> 56) & 0x00000000000000ffull);
}
AXH_HD uint64_t axh_mul128_fold64(uint64_t a, uint64_t b) {
#ifdef __CUDA_ARCH__
    return (a * b) ^ __umul64hi(a, b);
#else
    unsigned __int128 p = (unsigned __int128)a * b; return (uint64_t)p ^ (uint64_t)(p >> 64);
#endif
}
AXH_HD uint64_t axh_avalanche(uint64_t h) { h ^= h >> 37; h *= AXH_MX1; h ^= h >> 32; return h; }
AXH_HD uint64_t axh_xxh64_avalanche(uint64_t h) { h ^= h >> 33; h *= AXH_P64_2; h ^= h >> 29; h *= AXH_P64_3; h ^= h >> 32; return h; }
AXH_HD uint64_t axh_rrmxmx(uint64_t h, uint64_t len) {
    h ^= axh_rotl(h, 49) ^ axh_rotl(h, 24); h *= AXH_MX2; h ^= (h >> 35) + len; h *= AXH_MX2; return h ^ (h >> 28);
}
AXH_HD uint64_t axh_mix16(const uint8_t* in, const uint8_t* sec) {
    return axh_mul128_fold64(axh_r64(in) ^ axh_r64(sec), axh_r64(in + 8) ^ axh_r64(sec + 8));
}
/* XXH3_64bits of len <= 240 bytes */
AXH_HD uint64_t axh_short(const uint8_t* in, uint64_t len, const uint8_t* s) {
    if (len == 0) return axh_xxh64_avalanche(axh_r64(s + 56) ^ axh_r64(s + 64));
    if (len <= 3) {
        uint32_t c = ((uint32_t)in[0] << 16) | ((uint32_t)in[len >> 1] << 24) | (uint32_t)in[len - 1] | ((uint32_t)len << 8);
        return axh_xxh64_avalanche((uint64_t)c ^ (uint64_t)(axh_r32(s) ^ axh_r32(s + 4)));
    }
    if (len <= 8) {
        uint64_t in64 = (uint64_t)axh_r32(in + len - 4) + ((uint64_t)axh_r32(in) << 32);
        return axh_rrmxmx(in64 ^ (axh_r64(s + 8) ^ axh_r64(s + 16)), len);
    }
    if (len <= 16) {
        uint64_t lo = axh_r64(in) ^ (axh_r64(s + 24) ^ axh_r64(s + 32)), hi = axh_r64(in + len - 8) ^ (axh_r64(s + 40) ^ axh_r64(s + 48));
        return axh_avalanche(len + axh_swap64(lo) + hi + axh_mul128_fold64(lo, hi));
    }
    if (len <= 128) {
        uint64_t acc = len * AXH_P64_1;
        if (len > 32) {
            if (len > 64) {
                if (len > 96) { acc += axh_mix16(in + 48, s + 96); acc += axh_mix16(in + len - 64, s + 112); }
                acc += axh_mix16(in + 32, s + 64); acc += axh_mix16(in + len - 48, s + 80);
            }
            acc += axh_mix16(in + 16, s + 32); acc += axh_mix16(in + len - 32, s + 48);
        }
        acc += axh_mix16(in, s); acc += axh_mix16(in + len - 16, s + 16);
        return axh_avalanche(acc);
    }
    uint64_t acc = len * AXH_P64_1, acc_end; unsigned rounds = (unsigned)len / 16;
    for (unsigned i = 0; i < 8; i++) acc += axh_mix16(in + 16 * i, s + 16 * i);
    acc_end = axh_mix16(in + len - 16, s + 136 - 17);
    acc = axh_avalanche(acc);
    for (unsigned i = 8; i < rounds; i++) acc_end += axh_mix16(in + 16 * i, s + 16 * (i - 8) + 3);
    return axh_avalanche(acc + acc_end);
}
/* the term one stripe adds to lane `lane` (secret key at k) */
AXH_HD uint64_t axh_stripe_lane(const uint8_t* st, const uint8_t* k, unsigned lane) {
    uint64_t dk = axh_r64(st + 8 * lane) ^ axh_r64(k + 8 * lane);
    return axh_r64(st + 8 * (lane ^ 1)) + (uint64_t)(uint32_t)dk * (dk >> 32);
}
/* S_b[lane]: the sum over the 16 stripes of block b (b < (len-1)/1024) */
AXH_HD uint64_t axh_block_lane(const uint8_t* block, const uint8_t* s, unsigned lane) {
    uint64_t t = 0; for (unsigned st = 0; st < 16; st++) t += axh_stripe_lane(block + 64 * st, s + 8 * st, lane); return t;
}
/* the same with the input as aligned 64-bit words (w = block start, 128 words) and the key table */
AXH_HD uint64_t axh_block_lane_w(const uint64_t* w, const uint64_t* k64, unsigned lane) {
    uint64_t t = 0;
    for (unsigned st = 0; st < 16; st++) { uint64_t dk = w[8 * st + lane] ^ k64[st + lane]; t += w[8 * st + (lane ^ 1)] + (uint64_t)(uint32_t)dk * (dk >> 32); }
    return t;
}
/* one chain step with the lane's scramble key (axh_k64[16 + lane]) in a register */
AXH_HD uint64_t axh_step_k(uint64_t acc, uint64_t S, uint64_t key) { acc += S; acc ^= acc >> 47; acc ^= key; return acc * AXH_P32_1; }
AXH_HD uint64_t axh_init(unsigned lane) {
    const uint64_t a[8] = {AXH_P32_3, AXH_P64_1, AXH_P64_2, AXH_P64_3, AXH_P64_4, AXH_P32_2, AXH_P64_5, AXH_P32_1}; return a[lane];
}
AXH_HD uint64_t axh_chain_step(uint64_t acc, uint64_t S, const uint8_t* s, unsigned lane) {
    acc += S; acc ^= acc >> 47; acc ^= axh_r64(s + 192 - 64 + 8 * lane); return acc * AXH_P32_1;
}
/* after the chain over the full blocks: last partial block and last stripe for one lane */
AXH_HD uint64_t axh_tail_lane(uint64_t acc, const uint8_t* in, uint64_t len, const uint8_t* s, unsigned lane) {
    const uint64_t nb = (len - 1) / AXH_BLOCK, nst = ((len - 1) - AXH_BLOCK * nb) / AXH_STRIPE;
    for (uint64_t st = 0; st < nst; st++) acc += axh_stripe_lane(in + nb * AXH_BLOCK + 64 * st, s + 8 * st, lane);
    return acc + axh_stripe_lane(in + len - 64, s + 192 - 64 - 7, lane);
}
AXH_HD uint64_t axh_merge(const uint64_t acc[8], uint64_t len, const uint8_t* s) {
    uint64_t r = len * AXH_P64_1;
    for (unsigned i = 0; i < 4; i++) r += axh_mul128_fold64(acc[2 * i] ^ axh_r64(s + 11 + 16 * i), acc[2 * i + 1] ^ axh_r64(s + 11 + 16 * i + 8));
    return axh_avalanche(r);
}
#endif
