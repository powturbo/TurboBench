// rans_warp_emu.cpp - the GPU rANS chunk decoder (k_rans in aceapex_gpu.cu) on the CPU.
// It runs the per-lane steps of src/ax_rans_warp.h for lanes 0..31 in turn, with the warp
// collectives (exclusive scan, ballot, any) done by loops at the same points as in the
// kernel, and compares with the reference axr_decode (src/ax_rans.h):
//   1. round-trips of axr_encode over sizes 1..70000 and several symbol distributions;
//   2. every rANS chunk of the given archives (the conformance fixtures);
//   3. bit flips and truncations of encoded chunks: both decoders must agree on the
//      verdict, and on the bytes when they accept.
// Build: g++ -std=c++17 -O2 -Isrc -o rans_warp_emu scripts/rans_warp_emu.cpp
// Usage: rans_warp_emu [archive.aet ...]   Output: claim_id <TAB> verdict <TAB> measured
#include "ax_rans_warp.h"
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <vector>
#include <random>
#include <algorithm>

static uint64_t g_win_out = 0;                                       // V 1: a word outside the 64-word window (must stay 0)
static int axw_decode_emu(const uint8_t* src, uint32_t sz, uint8_t* dst, uint32_t n, int V = 0) {
    static AxwShared sh;
    bool b[32] = {false};
    auto any = [&] { bool a = false; for (int l = 0; l < 32; l++) a |= b[l]; return a; };
    if (sz < AXW_MIN) return -1;
    for (uint32_t l = 0; l < 32; l++) axw_stage(l, src, sz, sh);
    uint32_t r0[32], K = 0;                                         // exclusive scan of counts
    for (uint32_t l = 0; l < 32; l++) { r0[l] = K; K += axw_rank_count(l, sh); }
    for (uint32_t l = 0; l < 32; l++) axw_rank_write(l, sh, r0[l]);
    const uint32_t lim = axw_lim(sz); uint32_t jb = 0;
    for (uint32_t r = 0; jb < K && 32 + 32 * r < lim; r++) {
        bool t[32]; uint32_t m = 0;
        for (uint32_t l = 0; l < 32; l++) { t[l] = axw_term(l, sh, r, lim); if (t[l]) m |= 1u << l; }   // ballot
        for (uint32_t l = 0; l < 32; l++) axw_leb(l, sh, r, t[l], jb + axw_popc(m & ((1u << l) - 1u)), K, b[l]);
        jb += axw_popc(m);
    }
    b[0] |= jb < K;
    uint32_t cb[32], tot = 0;
    for (uint32_t l = 0; l < 32; l++) { cb[l] = tot; tot += axw_sum(l, sh); }
    b[0] |= tot != AXR_M;
    if (any()) return -1;
    for (uint32_t l = 0; l < 32; l++) axw_cum(l, sh, cb[l]);
    for (uint32_t l = 0; l < 32; l++) axw_fill(l, sh);
    uint32_t x[32], W = 0; const uint8_t* words = nullptr;
    for (uint32_t l = 0; l < 32; l++) b[l] |= axw_init(l, src, sz, sh.end, x[l], W, words);
    if (any()) return -1;
    uint32_t base = 0, wb = 0, w0[32], w1[32];                     // V 1: the register window of k_rans<1> (lane l: words wb+l, wb+32+l)
    for (uint32_t l = 0; l < 32; l++) { w0[l] = axw_wload(words, W, l); w1[l] = axw_wload(words, W, 32 + l); }
    for (uint32_t g = 0; g < (n + 31) / 32; g++) {
        bool need[32]; uint32_t m = 0;
        for (uint32_t l = 0; l < 32; l++) { need[l] = axw_step(l, sh, g, n, x[l], dst); if (need[l]) m |= 1u << l; }
        if (V == 0) for (uint32_t l = 0; l < 32; l++) axw_refill(l, need[l], m, base, W, words, x[l], b[l]);
        else for (uint32_t l = 0; l < 32; l++) {                    // shuffles: lane l reads lane (o & 31)'s w0 / w1
            const uint32_t idx = axw_widx(l, m, base), o = idx - wb;
            if (need[l] && o >= 64) g_win_out++;
            axw_refill_v(need[l], idx, W, o < 32 ? w0[o & 31] : w1[o & 31], x[l], b[l]); }
        base += axw_popc(m);
        if (V && base >= wb + 32) { wb += 32; for (uint32_t l = 0; l < 32; l++) { w0[l] = w1[l]; w1[l] = axw_wload(words, W, wb + 32 + l); } }
    }
    for (uint32_t l = 0; l < 32; l++) b[l] |= base != W || x[l] != AXR_L;
    return any() ? -1 : 0;
}

static uint64_t rd64(const uint8_t* p) { uint64_t v; memcpy(&v, p, 8); return v; }
static uint32_t rd32(const uint8_t* p) { uint32_t v; memcpy(&v, p, 4); return v; }

// both decoders on one chunk: 0 = agree, 1 = disagree
static uint64_t g_ok = 0, g_rej = 0, g_bytes = 0, g_scalar = 0, g_512 = 0;
static int cmp_chunk(const uint8_t* c, size_t csz, size_t n) {
    std::vector<uint8_t> a(n + 1, 0xAA), e(n + 1, 0x55);
    int ra = axr_decode(c, csz, a.data(), n);
    int re = axw_decode_emu(c, (uint32_t)csz, e.data(), (uint32_t)n);
    std::vector<uint8_t> e1(n + 1, 0x33); int r1 = axw_decode_emu(c, (uint32_t)csz, e1.data(), (uint32_t)n, 1);   // windowed refill
    std::vector<uint8_t> s0(n + 1, 0x44); int rs = axr_decode_scalar(c, csz, s0.data(), n); g_scalar++;   // axr_decode is the AVX2 one where the CPU has it
    std::vector<uint8_t> s5(n + 1, 0x55); int r5 = rs;
#ifdef AXR_SIMD
    if (__builtin_cpu_supports("avx512f") && __builtin_cpu_supports("avx512bw") && __builtin_cpu_supports("avx512vl") && __builtin_cpu_supports("avx512vbmi2")) {
        r5 = axr_decode_avx512(c, csz, s5.data(), n); g_512++; if (r5 == 0 && rs == 0 && memcmp(s5.data(), s0.data(), n)) return 1; }
#endif
    if (ra != re || ra != r1 || ra != rs || ra != r5) return 1;
    if (ra == 0) { if (memcmp(a.data(), e.data(), n) || memcmp(a.data(), e1.data(), n) || memcmp(a.data(), s0.data(), n)) return 1; g_ok++; g_bytes += n; } else g_rej++;
    return 0;
}

// Crafted table edits the random flips rarely reach. kind 0: a symbol with frequency 0
// (bitmap bit + LEB byte 0x00, must be rejected); 1: the first value padded to 4 LEB bytes
// (more than 21 bits, rejected); 2: padded to 3 bytes (valid LEB128, accepted by both).
static std::vector<uint8_t> craft(const uint8_t* c, size_t csz, int kind) {
    std::vector<uint8_t> m(c, c + csz);
    const size_t p = 32;
    if (kind == 0) {
        uint32_t s = 0; while (s < 256 && (c[s >> 3] >> (s & 7) & 1)) s++;
        if (s == 256) return m;
        size_t q = 32;                                              // insert before the values of higher symbols
        for (uint32_t t = 0; t < s; t++) if (c[t >> 3] >> (t & 7) & 1) { while (m[q] & 0x80) q++; q++; }
        m[s >> 3] |= (uint8_t)(1u << (s & 7)); m.insert(m.begin() + q, 0x00);
        return m;
    }
    uint32_t v = 0; int sh = 0; size_t q = p;
    for (;;) { uint8_t b = m[q++]; v |= (uint32_t)(b & 0x7F) << sh; if (!(b & 0x80)) break; sh += 7; }
    std::vector<uint8_t> leb; size_t want = kind == 1 ? 4 : 3;
    for (size_t k = 0; k < want; k++) { uint8_t b = (uint8_t)(v & 0x7F); v >>= 7; leb.push_back(k + 1 < want ? (uint8_t)(b | 0x80) : b); }
    m.erase(m.begin() + p, m.begin() + q); m.insert(m.begin() + p, leb.begin(), leb.end());
    return m;
}

int main(int argc, char** argv) {
    uint64_t bad = 0, rt = 0, fx = 0, mut = 0, crafted = 0;
    std::mt19937_64 R(2026);
    std::vector<uint8_t> in, enc; std::vector<uint16_t> scratch;
    const size_t sizes[] = {1, 2, 31, 32, 33, 63, 64, 65, 100, 1000, 4095, 4096, 4097, 20000, 65535, 65536, 70000};
    for (size_t n : sizes) for (int dist = 0; dist < 5; dist++) {
        in.resize(n);
        for (size_t i = 0; i < n; i++) {
            uint64_t v = R();
            in[i] = dist == 0 ? 7                                   // one symbol
                  : dist == 1 ? (uint8_t)(v & 1)                    // two symbols
                  : dist == 2 ? (uint8_t)v                          // uniform bytes
                  : dist == 3 ? (uint8_t)((v % 100) < 97 ? v % 4 : v >> 8)   // skewed, rare symbols
                  : (uint8_t)(__builtin_ctzll(v | (1ull << 40)) * 3);        // geometric, gaps in the alphabet
        }
        enc.resize(axr_bound(n)); scratch.resize(n + 2 * AXR_LANES);
        size_t csz = axr_encode(in.data(), n, enc.data(), scratch.data());
        if (!csz) { bad++; continue; }
        std::vector<uint8_t> e(n);
        if (axw_decode_emu(enc.data(), (uint32_t)csz, e.data(), (uint32_t)n) || memcmp(e.data(), in.data(), n)) bad++;
        if (axw_decode_emu(enc.data(), (uint32_t)csz, e.data(), (uint32_t)n, 1) || memcmp(e.data(), in.data(), n)) bad++;
        rt++;
        for (int kind = 0; kind < 3; kind++) {                      // crafted: verdict must also be the expected one
            std::vector<uint8_t> m = craft(enc.data(), csz, kind);
            if (m.size() == csz) continue;                          // kind 0 on a full alphabet, or already 3 bytes
            std::vector<uint8_t> e2(n);
            int want = kind == 2 ? 0 : -1, ref = axr_decode(m.data(), m.size(), e2.data(), n);
            bad += (ref != want) + cmp_chunk(m.data(), m.size(), n); crafted++;
        }
        for (int k = 0; k < 40; k++) {                              // mutations: flips and truncations
            std::vector<uint8_t> m(enc.begin(), enc.begin() + csz); size_t msz = csz;
            if (k % 4 == 3) msz = (size_t)(R() % csz);
            else { size_t p = (size_t)(R() % csz); m[p] ^= (uint8_t)(1u << (R() % 8)); }
            bad += cmp_chunk(m.data(), msz, n); mut++;
        }
    }
    for (int ai = 1; ai < argc; ai++) {                              // rANS chunks of real archives
        FILE* f = fopen(argv[ai], "rb"); if (!f) { bad++; continue; }
        std::vector<uint8_t> a; { uint8_t buf[1 << 16]; size_t r; while ((r = fread(buf, 1, sizeof buf, f)) > 0) a.insert(a.end(), buf, buf + r); } fclose(f);
        if (a.size() < 68 || memcmp(a.data(), "ACEPX2\0\0", 8) || rd32(&a[8]) != 2) continue;   // header version 2 only (the fast profile has no entropy chunks)
        uint32_t nb = rd32(&a[24]); uint64_t zsz[4] = {rd64(&a[36]), rd64(&a[44]), rd64(&a[52]), rd64(&a[60])};
        size_t p = 68 + 64ull * nb; const uint8_t* zs[4];
        for (int i = 0; i < 4; i++) { zs[i] = a.data() + p; p += zsz[i]; }
        if (p > a.size()) { bad++; continue; }
        for (int st = 1; st < 4; st++) {
            if (zsz[st] < 8) continue;
            const uint8_t* z = zs[st]; uint64_t w = rd64(z), osz = w & ((1ull << 48) - 1), ch = ((w >> 48) & 0x7fff) * 4096;
            if (!ch) continue;
            uint64_t nc = (osz + ch - 1) / ch; size_t pos = 8 + 8 * nc;
            for (uint64_t i = 0; i < nc && pos <= zsz[st]; i++) {
                uint64_t cs = rd64(z + 8 + 8 * i); size_t raw = (size_t)std::min<uint64_t>(ch, osz - i * ch);
                size_t csz = (cs >> 63) ? raw : (size_t)(cs & ((1ull << 48) - 1));
                if (pos + csz > zsz[st]) { bad++; break; }
                if (!(cs >> 63) && ((cs >> 62) & 1)) {
                    bad += cmp_chunk(z + pos, csz, raw); fx++;
                    for (int k = 0; k < 8; k++) {
                        std::vector<uint8_t> m(z + pos, z + pos + csz); m[(size_t)(R() % csz)] ^= (uint8_t)(1u << (R() % 8));
                        bad += cmp_chunk(m.data(), csz, raw); mut++;
                    }
                }
                pos += csz;
            }
        }
    }
    printf("head_rans_warp_emu\t%s\tGPU rANS warp steps (ax_rans_warp.h) == axr_decode, refill from global and from the register window (AX_OPEN_SEQ, %llu words outside it), CPU AVX2 decoder == scalar (%s, %llu chunks), AVX-512 decoder == scalar (%llu chunks): %llu round-trips, %llu archive chunks, %llu crafted tables, %llu mutations (%llu accepted, %llu rejected by both), %llu bytes, %llu mismatches\n",
           (bad == 0 && rt > 0 && fx > 0 && crafted > 0 && g_win_out == 0) ? "pass" : "fail", (unsigned long long)g_win_out,
#ifdef AXR_SIMD
           __builtin_cpu_supports("avx2") ? "AVX2 on" : "no AVX2 on this CPU",
#else
           "no AVX2 path in this build",
#endif
           (unsigned long long)g_scalar, (unsigned long long)g_512,
           (unsigned long long)rt, (unsigned long long)fx, (unsigned long long)crafted, (unsigned long long)mut,
           (unsigned long long)g_ok, (unsigned long long)g_rej, (unsigned long long)g_bytes, (unsigned long long)bad);
    return 0;
}
