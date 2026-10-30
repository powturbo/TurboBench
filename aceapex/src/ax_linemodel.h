// ax_linemodel.h - AX_LINEMODEL (experiment, builds with ACEAPEX_ENV_TUNING only; not a format of the library):
// the FASTA line ends are taken out before LZ. The input is split into a line model - header lines ('>' ...) as text,
// sequence lines as runs (length, count): a regular FASTA has one or two runs per record, any other line length is a
// run of its own (the exceptions) - and the sequence bytes (case kept). The sequence is compressed as an ordinary
// ACEPX2 image; the container (magic AXLINE01) is
//   "AXLINE01" | u64 original size | u64 XXH3 of the original | u64 model bytes | u64 model zstd bytes | model (zstd)
//   | the ACEPX2 image of the sequence (the rest)
// Only tuning builds read it (aceapex_decompress*, aceapex_decompress_region); every other decoder refuses its magic.
// Decode: every decoded block goes through a sink that puts it in place with its line ends while it is in cache
// (tile path; otherwise the image into a sequence buffer, then the lines rebuilt in parallel);
// a region maps its FASTA bytes to one sequence range, decodes that range and rebuilds just those lines.
// Measured in results/linemodel-2026-10-02.log; why: results/reality-2026-10-02.log T1 (line ends cost T2T 4.7 %).
#ifndef AX_LINEMODEL_H
#define AX_LINEMODEL_H
#include <thread>
#include <vector>
#include <cstring>
#include <algorithm>
#include <zstd.h>
#include "xxhash.h"

namespace axlm {
static const char MAGIC[8] = {'A', 'X', 'L', 'I', 'N', 'E', '0', '1'};
static thread_local int t_inner = 0;                      // the sequence image itself goes through aceapex_compress
static bool wanted() { const char* e = ax_getenv("AX_LINEMODEL"); return e && atoi(e) && !t_inner; }
static bool is(const void* src, size_t n) { return src && n >= 8 && !memcmp(src, MAGIC, 8); }
static void put_v(std::vector<uint8_t>& o, uint64_t x) { while (x >= 0x80) { o.push_back((uint8_t)(x | 0x80)); x >>= 7; } o.push_back((uint8_t)x); }
static bool get_v(const uint8_t*& p, const uint8_t* e, uint64_t& v) { v = 0; int s = 0;
    while (p < e && s <= 63) { const uint8_t c = *p++; v |= (uint64_t)(c & 0x7F) << s; if (!(c & 0x80)) return true; s += 7; } return false; }
// model: tag 0 header (varint length, text), 1 run (varint line length, varint count), 2 = no final '\n', 3 = end
static void split(const uint8_t* f, size_t n, std::vector<uint8_t>& lm, std::vector<uint8_t>& seq) {
    seq.reserve(n + 64); size_t i = 0; uint64_t rl = 0, rc = 0;
    auto flush = [&] { if (rc) { lm.push_back(1); put_v(lm, rl); put_v(lm, rc); rc = 0; } };
    while (i < n) {
        const uint8_t* nl = (const uint8_t*)memchr(f + i, '\n', n - i); const size_t e = nl ? (size_t)(nl - f) : n;
        if (f[i] == '>') { flush(); lm.push_back(0); put_v(lm, e - i); lm.insert(lm.end(), f + i, f + e); }
        else { const uint64_t L = e - i; if (rc && L == rl) rc++; else { flush(); rl = L; rc = 1; } seq.insert(seq.end(), f + i, f + e); }
        if (e == n) { flush(); lm.push_back(2); break; }
        i = e + 1;
    }
    flush(); lm.push_back(3);
}
struct Seg { uint64_t out, seq, len, count; const uint8_t* hdr; };   // count 0 = a header line of len bytes (+ '\n')
static bool parse(const uint8_t* p, const uint8_t* e, std::vector<Seg>& S, uint64_t& out_n, uint64_t& seq_n) {
    uint64_t o = 0, q = 0; bool unterminated = false;
    while (p < e) { const uint8_t t = *p++; uint64_t a, b;
        if (t == 0) { if (!get_v(p, e, a) || a > (uint64_t)(e - p)) return false; S.push_back({o, q, a, 0, p}); p += a; o += a + 1; }
        else if (t == 1) { if (!get_v(p, e, a) || !get_v(p, e, b) || !b) return false; S.push_back({o, q, a, b, nullptr}); o += (a + 1) * b; q += a * b; }
        else if (t == 2) unterminated = true;
        else if (t == 3) { out_n = o - (unterminated && o ? 1 : 0); seq_n = q; return true; }
        else return false; }
    return false;
}
// output bytes [lo, hi) of the original into dst; seq(x) = the sequence byte x, read from sq + (x - sq0)
static void expand(const std::vector<Seg>& S, uint64_t lo, uint64_t hi, uint8_t* dst, const uint8_t* sq, uint64_t sq0) {
    size_t k = (size_t)(std::upper_bound(S.begin(), S.end(), lo, [](uint64_t v, const Seg& g) { return v < g.out; }) - S.begin()); if (k) k--;
    uint64_t o = lo;
    for (; k < S.size() && o < hi; k++) { const Seg& g = S[k];
        if (!g.count) { const uint64_t end = g.out + g.len + 1;
            for (; o < hi && o < end; o++) *dst++ = (o - g.out < g.len) ? g.hdr[o - g.out] : '\n'; continue; }
        const uint64_t W = g.len + 1, end = g.out + W * g.count;
        while (o < hi && o < end) { const uint64_t li = (o - g.out) / W, c = (o - g.out) % W;
            if (c < g.len) { const uint64_t m = std::min<uint64_t>(g.len - c, hi - o); memcpy(dst, sq + (g.seq + li * g.len + c - sq0), m); dst += m; o += m; }
            else { *dst++ = '\n'; o++; } } }
}
// the sequence range [s0, s1) the output bytes [lo, hi) need
static void seq_range(const std::vector<Seg>& S, uint64_t lo, uint64_t hi, uint64_t& s0, uint64_t& s1) {
    auto at = [&](uint64_t o, bool upper) -> uint64_t {
        size_t k = (size_t)(std::upper_bound(S.begin(), S.end(), o, [](uint64_t v, const Seg& g) { return v < g.out; }) - S.begin()); if (k) k--;
        const Seg& g = S[k]; if (!g.count) return g.seq;
        const uint64_t W = g.len + 1, d = std::min(o - g.out, W * g.count), li = d / W, c = std::min(d % W, g.len);
        (void)upper; return g.seq + li * g.len + c; };
    s0 = at(lo, false); s1 = at(hi, true);
}

static int64_t compress(const uint8_t* src, size_t n, uint8_t* dst, size_t cap, int level, int threads) {
    std::vector<uint8_t> lm, seq; split(src, n, lm, seq);
    std::vector<uint8_t> lz(ZSTD_compressBound(lm.size())); const size_t lzn = ZSTD_compress(lz.data(), lz.size(), lm.data(), lm.size(), 19);
    if (ZSTD_isError(lzn)) return ACEAPEX_ERR_MEMORY;
    const size_t hd = 8 + 8 * 4 + lzn; if (cap < hd) return ACEAPEX_ERR_BUFFER;
    std::vector<uint8_t> img(aceapex_compress_bound(seq.size()));
    t_inner = 1; const int64_t zn = aceapex_compress(seq.data(), seq.size(), img.data(), img.size(), level, threads); t_inner = 0;
    if (zn < 0) return zn; if (hd + (size_t)zn > cap) return ACEAPEX_ERR_BUFFER;
    const uint64_t h[4] = {(uint64_t)n, XXH3_64bits(src, n), (uint64_t)lm.size(), (uint64_t)lzn};
    memcpy(dst, MAGIC, 8); memcpy(dst + 8, h, 32); memcpy(dst + 40, lz.data(), lzn); memcpy(dst + hd, img.data(), (size_t)zn);
    return (int64_t)(hd + (size_t)zn);
}
struct View { uint64_t orig, hash; std::vector<uint8_t> lm; std::vector<Seg> S; uint64_t out_n, seq_n; const uint8_t* img; size_t img_n; };
static int open(const uint8_t* src, size_t n, View& v) {
    if (n < 40) return ACEAPEX_ERR_DATA; uint64_t h[4]; memcpy(h, src + 8, 32); v.orig = h[0]; v.hash = h[1];
    if (h[3] > n - 40 || h[2] > ((uint64_t)1 << 40)) return ACEAPEX_ERR_DATA;
    v.lm.resize((size_t)h[2]); if (ZSTD_decompress(v.lm.data(), v.lm.size(), src + 40, (size_t)h[3]) != h[2]) return ACEAPEX_ERR_DATA;
    if (!parse(v.lm.data(), v.lm.data() + v.lm.size(), v.S, v.out_n, v.seq_n) || v.out_n != v.orig) return ACEAPEX_ERR_DATA;
    v.img = src + 40 + h[3]; v.img_n = n - 40 - (size_t)h[3]; return 0;
}
// the sequence bytes [s0, s0 + n) (a decoded block) to their places in the output, each full line's '\n' after it
struct Sink { const std::vector<Seg>* R; uint8_t* dst; uint64_t orig; };
static void sink(void* ctx, size_t s0, const uint8_t* p, size_t n) {
    const Sink& K = *(const Sink*)ctx; const std::vector<Seg>& R = *K.R; uint64_t x = s0; const uint64_t s1 = s0 + n;
    size_t k = (size_t)(std::upper_bound(R.begin(), R.end(), x, [](uint64_t v, const Seg& g) { return v < g.seq; }) - R.begin()); if (k) k--;
    for (; k < R.size() && x < s1; k++) { const Seg& g = R[k]; const uint64_t end = g.seq + g.len * g.count; if (x >= end) continue;
        while (x < s1 && x < end) { const uint64_t li = (x - g.seq) / g.len, c = (x - g.seq) % g.len, m = std::min(g.len - c, s1 - x);
            const uint64_t o = g.out + li * (g.len + 1) + c; memcpy(K.dst + o, p + (x - s0), m); x += m;
            if (c + m == g.len && o + m < K.orig) K.dst[o + m] = '\n'; } }
}
static int64_t decompress(const uint8_t* src, size_t n, uint8_t* dst, size_t cap, int threads) {
    View v; int e = open(src, n, v); if (e) return e; if (v.orig > cap) return ACEAPEX_ERR_BUFFER;
    // headers and empty lines first (no block holds them)
    for (const Seg& g : v.S) { if (!g.count) { const uint64_t m = std::min<uint64_t>(g.len + 1, v.orig - g.out); if (m) { memcpy(dst + g.out, g.hdr, std::min<uint64_t>(g.len, m)); if (m > g.len) dst[g.out + g.len] = '\n'; } }
        else if (!g.len) for (uint64_t c = 0; c < g.count && g.out + c < v.orig; c++) dst[g.out + c] = '\n'; }
    std::vector<Seg> R; for (const Seg& g : v.S) if (g.count && g.len) R.push_back(g);
    // fused: every decoded block goes through the sink (tile path: chunked literals, AX_LIT_TILE on); else two passes
    uint64_t w = 0; uint32_t nb = 0; memcpy(&nb, v.img + 24, 4); const size_t lit0 = 68 + (size_t)64 * nb;
    if (v.img_n >= lit0 + 8) memcpy(&w, v.img + lit0, 8);
    static const bool tile = [] { const char* e = ax_getenv("AX_LIT_TILE"); return e ? atoi(e) != 0 : true; }();
    if (tile && ((w >> 62) & 1) && ((w >> 61) & 1) && v.orig >= v.seq_n) {
        Sink K{&R, dst, v.orig}; g_ax_out_sink = sink; g_ax_out_ctx = &K;
        t_inner = 1; const int64_t r = aceapex_decompress_mt(v.img, v.img_n, dst, cap, threads); t_inner = 0;
        g_ax_out_sink = nullptr; g_ax_out_ctx = nullptr;
        if (r != (int64_t)v.seq_n) return r < 0 ? r : ACEAPEX_ERR_DATA;
    } else {
        uint8_t* seq = (uint8_t*)malloc(v.seq_n + 64); if (!seq) return ACEAPEX_ERR_MEMORY;   // not a vector: no zero fill
        t_inner = 1; const int64_t r = aceapex_decompress_mt(v.img, v.img_n, seq, v.seq_n + 64, threads); t_inner = 0;
        if (r != (int64_t)v.seq_n) { free(seq); return r < 0 ? r : ACEAPEX_ERR_DATA; }
        int T = threads > 0 ? threads : (int)std::thread::hardware_concurrency(); if (T < 1) T = 1; if (v.orig < ((uint64_t)8 << 20)) T = 1;
        std::vector<std::thread> th;
        for (int t = 0; t < T; t++) { const uint64_t lo = v.orig * t / T, hi = v.orig * (t + 1) / T;
            if (t == T - 1) expand(v.S, lo, hi, dst + lo, seq, 0); else th.emplace_back([&, lo, hi] { expand(v.S, lo, hi, dst + lo, seq, 0); }); }
        for (auto& x : th) x.join();
        free(seq);
    }
    static const bool verify = [] { const char* e = ax_getenv("AX_LINEMODEL_VERIFY"); return e && atoi(e); }();   // as the plain decoder: no hash unless asked
    if (verify && XXH3_64bits(dst, v.orig) != v.hash) return ACEAPEX_ERR_DATA;
    return (int64_t)v.orig;
}
static int64_t region(const uint8_t* src, size_t n, uint8_t* dst, size_t cap, uint64_t off, uint64_t len) {
    View v; int e = open(src, n, v); if (e) return e;
    if (off > v.orig || len > v.orig - off) return ACEAPEX_ERR_DATA; if (len > cap) return ACEAPEX_ERR_BUFFER; if (!len) return 0;
    uint64_t s0, s1; seq_range(v.S, off, off + len, s0, s1);
    std::vector<uint8_t> seq((size_t)(s1 - s0) + 64);
    if (s1 > s0) { const int64_t r = aceapex_decompress_region(v.img, v.img_n, seq.data(), seq.size(), s0, s1 - s0); if (r != (int64_t)(s1 - s0)) return r < 0 ? r : ACEAPEX_ERR_DATA; }
    expand(v.S, off, off + len, dst, seq.data(), s0);
    return (int64_t)len;
}
}  // namespace axlm
#endif
