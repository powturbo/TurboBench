// xxh3_split_emu.cpp - src/ax_xxh3.h (XXH3_64bits split into parallel block terms + a sequential scramble
// chain, as the GPU library computes it) against the reference XXH3_64bits of src/xxhash.h, on the CPU:
// every length 0..4200, random lengths up to 8 MiB, random bytes and text. Prints one claim line.
// Build: g++ -std=c++17 -O2 -Isrc scripts/xxh3_split_emu.cpp
#define XXH_INLINE_ALL
#include "xxhash.h"
#include "ax_xxh3.h"
#include <cstdio>
#include <random>
#include <vector>
static uint64_t split(const uint8_t* in, uint64_t len) {
    if (len <= 240) return axh_short(in, len, axh_secret);
    const uint64_t nb = (len - 1) / AXH_BLOCK; uint64_t acc[8];
    for (unsigned l = 0; l < 8; l++) acc[l] = axh_init(l);
    const bool al = ((uintptr_t)in & 7) == 0;                     // the GPU path: aligned words, key table
    for (uint64_t b = 0; b < nb; b++) for (unsigned l = 0; l < 8; l++) {
        const uint64_t S = al ? axh_block_lane_w((const uint64_t*)(in + b * AXH_BLOCK), axh_k64, l) : axh_block_lane(in + b * AXH_BLOCK, axh_secret, l);
        acc[l] = (b & 1) ? axh_step_k(acc[l], S, axh_k64[16 + l]) : axh_chain_step(acc[l], S, axh_secret, l); }
    for (unsigned l = 0; l < 8; l++) acc[l] = axh_tail_lane(acc[l], in, len, axh_secret, l);
    return axh_merge(acc, len, axh_secret);
}
int main() {
    std::mt19937_64 rng(2026); std::vector<uint8_t> buf(8u << 20);
    for (auto& c : buf) c = (uint8_t)rng();
    std::vector<uint8_t> txt(buf.size()); for (size_t i = 0; i < txt.size(); i++) txt[i] = "ACGTacgtN the fox\n"[(i * 7 + (i >> 11)) % 18];
    int n = 0, bad = 0;
    for (int i = 0; i < 24; i++) bad += axh_k64[i] != axh_r64(axh_secret + 8 * i);   // the key table is the secret
    for (int k = 0; k < 2; k++) { const uint8_t* b = k ? txt.data() : buf.data();
        for (uint64_t len = 0; len <= 4200; len++) { n++; bad += split(b, len) != XXH3_64bits(b, len); }
        for (int r = 0; r < 60; r++) { uint64_t len = rng() % buf.size(); n++; bad += split(b, len) != XXH3_64bits(b, len); } }
    printf("head_gpu_xxh3_emu\t%s\t%d inputs (every length 0..4200 and random up to 8 MiB, random and text): split XXH3 == reference XXH3_64bits, %d differ\n", bad ? "fail" : "pass", n, bad);
    return bad ? 1 : 0;
}
