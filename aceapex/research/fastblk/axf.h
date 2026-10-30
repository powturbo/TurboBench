#pragma once
// axf.h - our own "fast" token engine for the ACEAPEX container (#F1, step 2): written from the
// format idea, not from any other codec's code. One independent block = one token stream + one
// literal stream; no entropy stage. Decoder does no per-token branching on lengths in the common
// path: every copy is an unconditional 64-byte store sequence.
//
// Block layout (all little-endian):
//   u32 ntok      tokens
//   u32 next      bytes of the extension stream
//   u32 tail      literal bytes after the last token
//   u32 tok[ntok] ll (6 bits) | ml (6 bits) << 6 | d (20 bits) << 12
//   u8  ext[next] literal-length extensions: for ll == 63, bytes of 255 then one byte < 255
//   u8  lit[]     literals in token order, then the tail
// Token semantics: copy ll literals (ll < 63 direct; 63 -> 63 + extension), then a match of
// ml + MIN_MATCH bytes from dst - (d + MIN_DIST). Every match length is <= COPY (64); the encoder
// splits longer matches into tokens with ll = 0. MIN_DIST = 16 makes every match copy a plain
// forward 16-byte-step copy (source chunk k is complete before destination chunk k is written).
// The decoder may write up to COPY bytes past the block end: the caller gives that slack.
// Build: g++ -O3 -march=native -std=c++17 -o axf axf.cpp -lpthread
// Usage: axf file [block=262144] [level=1] [reps=9]
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <vector>
#include <thread>
#include <chrono>
#include <algorithm>
#include <immintrin.h>

#ifndef MIN_MATCH
#define MIN_MATCH 5u
#endif
#ifndef MIN_DIST_V
#define MIN_DIST_V 64u     /* 26.10: 16 -> 64 = +0.37 pp, 1T +8 % (no store-forwarding hazard inside a 32-byte copy) */
#endif
enum : uint32_t { MIN_DIST = MIN_DIST_V, COPY = 64, LL_EXT = 63, ML_EXT = 63, ML_DIRECT_MAX = COPY, D_MAX = (1u << 20) - 1, SLACK = 256 };
#ifndef WIN_V
#define WIN_V (1u << 20)
#endif
static const uint32_t WIN = WIN_V;   /* encoder: farthest match source */
// match length: code 0..62 -> MIN_MATCH + code (the encoder keeps it <= COPY); code 63 -> COPY + 1 + extension (bytes of 255, then one byte)
static inline uint32_t rd32(const uint8_t* p) { uint32_t v; memcpy(&v, p, 4); return v; }
static inline uint64_t rd64(const uint8_t* p) { uint64_t v; memcpy(&v, p, 8); return v; }
static inline void wr32(uint8_t* p, uint32_t v) { memcpy(p, &v, 4); }
static inline void cpyn(uint8_t* d, const void* s, size_t k) { if (k) memcpy(d, s, k); }   /* empty vectors give data() == nullptr */
static double now() { return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); }

// ---------------------------------------------------------------- encoder (greedy, hash head + chain)
struct Enc { std::vector<uint32_t> head, chain; };
#include <atomic>
static std::atomic<uint64_t> g_tok{0}, g_lit{0};
// ---------------------------------------------------------------- level 4+: optimal parse (F1-DM)
// Price of a parse = literal bytes + TOKB bytes per token + LAMBDA per token (the decode cost of a token expressed
// in bytes: LAMBDA = 0 minimises size, larger LAMBDA trades size for fewer tokens). Forward DP over the block;
// at each position the longest match (hash chain, depth DEPTH4) and every shorter length of it are candidates.
#ifndef DEPTH4
#define DEPTH4 64
#endif
static double g_lambda = 0.0;   // set by the bench (env AXF_LAMBDA)
static size_t enc_block_opt(const uint8_t* in, uint32_t n, uint8_t* out, Enc& e);
static size_t enc_block(const uint8_t* in, uint32_t n, uint8_t* out, int level, Enc& e) {
    if (level >= 4) return enc_block_opt(in, n, out, e);
    const uint32_t HB = 15, HN = 1u << HB; e.head.assign(HN, 0xffffffffu); e.chain.assign(n + 1, 0xffffffffu);
    const int depth = level <= 1 ? 1 : level == 2 ? 8 : 64;
    std::vector<uint32_t> tok; std::vector<uint8_t> ext, lit; tok.reserve(n / 8 + 1); lit.reserve(n);
    auto H = [&](uint32_t p) { return (rd32(in + p) * 0x9E3779B1u) >> (32 - HB); };
    auto leb = [&](uint32_t r) { while (r >= 255) { ext.push_back(255); r -= 255; } ext.push_back((uint8_t)r); };
    auto put = [&](uint32_t ll, uint32_t ml, uint32_t d) {
        uint32_t llf = ll; if (ll >= LL_EXT) { llf = LL_EXT; leb(ll - LL_EXT); }
        uint32_t mlf; if (ml <= ML_DIRECT_MAX) mlf = ml - MIN_MATCH; else { mlf = ML_EXT; leb(ml - COPY - 1); }
        tok.push_back(llf | mlf << 6 | (d - MIN_DIST) << 12); };
    uint32_t pos = 0, anchor = 0;
    while (pos + 12 < n) {
        uint32_t h = H(pos), cand = e.head[h], best = 0, bd = 0; int dep = depth;
        while (cand != 0xffffffffu && dep-- > 0) {
            uint32_t d = pos - cand;
            if (d > D_MAX + MIN_DIST || d > WIN) break;
            if (d >= MIN_DIST && rd32(in + cand) == rd32(in + pos)) {
                uint32_t l = 4, lim = n - pos - 8;
                while (l + 8 <= lim) { uint64_t x = rd64(in + pos + l) ^ rd64(in + cand + l); if (x) { l += __builtin_ctzll(x) >> 3; break; } l += 8; }
                if (l > lim) l = lim;
                if (l > best) { best = l; bd = d; if (l >= 512) break; }
            }
            cand = e.chain[cand];
        }
        if (best >= MIN_MATCH && level >= 2 && pos + 13 < n) {               // lazy: a longer match one byte later wins
            uint32_t h2 = H(pos + 1), c2 = e.head[h2], b2 = 0; int dp = depth;
            while (c2 != 0xffffffffu && dp-- > 0) { uint32_t d2 = pos + 1 - c2; if (d2 > D_MAX + MIN_DIST || d2 > WIN) break;
                if (d2 >= MIN_DIST && rd32(in + c2) == rd32(in + pos + 1)) { uint32_t l = 4, lim = n - pos - 9;
                    while (l + 8 <= lim) { uint64_t x = rd64(in + pos + 1 + l) ^ rd64(in + c2 + l); if (x) { l += __builtin_ctzll(x) >> 3; break; } l += 8; }
                    if (l > lim) l = lim; if (l > b2) b2 = l; }
                c2 = e.chain[c2]; }
            if (b2 > best + 1) { e.chain[pos] = e.head[h]; e.head[h] = pos; pos++; continue; }
        }
        if (best >= MIN_MATCH) {
            lit.insert(lit.end(), in + anchor, in + pos); put(pos - anchor, best, bd);
            // enter the matched positions into the tables (sparse for speed at level 1)
            uint32_t step = level <= 1 ? 4 : 1;
            for (uint32_t p = pos; p < pos + best && p + 12 < n; p += step) { uint32_t hh = H(p); e.chain[p] = e.head[hh]; e.head[hh] = p; }
            pos += best; anchor = pos;
        } else { e.chain[pos] = e.head[h]; e.head[h] = pos; pos += 1 + ((pos - anchor) >> 6); }   // skip faster in incompressible runs
    }
    lit.insert(lit.end(), in + anchor, in + n);
    const uint32_t tail = n - anchor;
    g_tok += tok.size(); g_lit += lit.size();
    uint8_t* p = out; wr32(p, (uint32_t)tok.size()); wr32(p + 4, (uint32_t)ext.size()); wr32(p + 8, tail); p += 12;
    cpyn(p, tok.data(), tok.size() * 4); p += tok.size() * 4; cpyn(p, ext.data(), ext.size()); p += ext.size();
    cpyn(p, lit.data(), lit.size()); p += lit.size();
    return (size_t)(p - out);
}
static size_t enc_bound(size_t n) { return 12 + n + n / MIN_MATCH * 5 + 64; }
static size_t enc_block_opt(const uint8_t* in, uint32_t n, uint8_t* out, Enc& e) {
    const uint32_t HB = 16, HN = 1u << HB; e.head.assign(HN, 0xffffffffu); e.chain.assign(n + 1, 0xffffffffu);
    auto H = [&](uint32_t p) { return (rd32(in + p) * 0x9E3779B1u) >> (32 - HB); };
    std::vector<uint32_t> bl(n + 1, 0), bd(n + 1, 0);
    const uint32_t lastp = n > 12 ? n - 12 : 0;
    for (uint32_t pos = 0; pos < lastp; pos++) {                 // longest match at every position
        uint32_t h = H(pos), cand = e.head[h], best = 0, bdist = 0; int dep = DEPTH4;
        while (cand != 0xffffffffu && dep-- > 0) {
            uint32_t d = pos - cand; if (d > D_MAX + MIN_DIST || d > WIN) break;
            if (d >= MIN_DIST && rd32(in + cand) == rd32(in + pos)) {
                uint32_t l = 4, lim = n - pos - 8;
                while (l + 8 <= lim) { uint64_t x = rd64(in + pos + l) ^ rd64(in + cand + l); if (x) { l += __builtin_ctzll(x) >> 3; break; } l += 8; }
                if (l > lim) l = lim; if (l > best) { best = l; bdist = d; if (l >= 4096) break; }
            }
            cand = e.chain[cand];
        }
        e.chain[pos] = e.head[h]; e.head[h] = pos;
        if (best >= MIN_MATCH) { bl[pos] = best; bd[pos] = bdist; }
    }
    const double TOKB = 4.0, LAM = g_lambda;
    std::vector<double> price(n + 1, 1e300); std::vector<uint32_t> from(n + 1, 0), mlen(n + 1, 0), mdist(n + 1, 0);
    price[0] = 0;
    for (uint32_t i = 0; i < n; i++) {
        const double pi = price[i];
        if (pi + 1.0 < price[i + 1]) { price[i + 1] = pi + 1.0; from[i + 1] = i; mlen[i + 1] = 0; }
        const uint32_t L = bl[i]; if (!L) continue;
        const double c = pi + TOKB + LAM;
        const uint32_t top = L < 64 ? L : 64;
        for (uint32_t l = MIN_MATCH; l <= top; l++) { uint32_t j = i + l; if (c < price[j]) { price[j] = c; from[j] = i; mlen[j] = l; mdist[j] = bd[i]; } }
        if (L > 64) { uint32_t j = i + L; if (c < price[j]) { price[j] = c; from[j] = i; mlen[j] = L; mdist[j] = bd[i]; } }
    }
    // backtrack into (start, len, dist) matches, then emit tokens in the order of the block
    std::vector<uint32_t> ms, ml, md;
    for (uint32_t j = n; j > 0; ) { uint32_t i = from[j]; if (mlen[j]) { ms.push_back(i); ml.push_back(mlen[j]); md.push_back(mdist[j]); } j = i; }
    std::vector<uint32_t> tok; std::vector<uint8_t> ext, lit; tok.reserve(ms.size()); lit.reserve(n);
    auto leb = [&](uint32_t r) { while (r >= 255) { ext.push_back(255); r -= 255; } ext.push_back((uint8_t)r); };
    uint32_t anchor = 0;
    for (size_t k = ms.size(); k-- > 0; ) {
        uint32_t st = ms[k], l = ml[k], d = md[k], ll = st - anchor;
        lit.insert(lit.end(), in + anchor, in + st);
        uint32_t llf = ll; if (ll >= LL_EXT) { llf = LL_EXT; leb(ll - LL_EXT); }
        uint32_t mlf; if (l <= ML_DIRECT_MAX) mlf = l - MIN_MATCH; else { mlf = ML_EXT; leb(l - COPY - 1); }
        tok.push_back(llf | mlf << 6 | (d - MIN_DIST) << 12); anchor = st + l;
    }
    lit.insert(lit.end(), in + anchor, in + n);
    g_tok += tok.size(); g_lit += lit.size();
    uint8_t* p = out; wr32(p, (uint32_t)tok.size()); wr32(p + 4, (uint32_t)ext.size()); wr32(p + 8, n - anchor); p += 12;
    cpyn(p, tok.data(), tok.size() * 4); p += tok.size() * 4; cpyn(p, ext.data(), ext.size()); p += ext.size();
    cpyn(p, lit.data(), lit.size()); p += lit.size();
    return (size_t)(p - out);
}

// ---------------------------------------------------------------- decoder
static inline void copy64(uint8_t* d, const uint8_t* s) {
    _mm_storeu_si128((__m128i*)(d + 0),  _mm_loadu_si128((const __m128i*)(s + 0)));
    _mm_storeu_si128((__m128i*)(d + 16), _mm_loadu_si128((const __m128i*)(s + 16)));
    _mm_storeu_si128((__m128i*)(d + 32), _mm_loadu_si128((const __m128i*)(s + 32)));
    _mm_storeu_si128((__m128i*)(d + 48), _mm_loadu_si128((const __m128i*)(s + 48)));
}
#ifndef CW
#define CW 32            /* unconditional copy width; a second CW-byte copy only when the length exceeds it */
#endif
static inline void copyw(uint8_t* d, const uint8_t* s) {
#if CW == 16
    _mm_storeu_si128((__m128i*)d, _mm_loadu_si128((const __m128i*)s));
#elif CW == 32 && MIN_DIST_V >= 32 && defined(__AVX2__)
    _mm256_storeu_si256((__m256i*)d, _mm256_loadu_si256((const __m256i*)s));   /* one load before the store: exact when dist >= 32 */
#elif CW == 32
    _mm_storeu_si128((__m128i*)(d + 0),  _mm_loadu_si128((const __m128i*)(s + 0)));
    _mm_storeu_si128((__m128i*)(d + 16), _mm_loadu_si128((const __m128i*)(s + 16)));
#else
    copy64(d, s);
#endif
}
// copy of l <= COPY bytes: CW unconditionally, the rest only when needed (dist >= 16 keeps the order exact)
static inline void copy_le64(uint8_t* d, const uint8_t* s, uint32_t l) {
    copyw(d, s);
#if CW < 64
    if (__builtin_expect(l > CW, 0)) { for (uint32_t k = CW; k < l; k += CW) copyw(d + k, s + k); }
#endif
}
// returns bytes written (n) or 0 on a malformed block; dst has at least n + COPY bytes of room
static uint32_t dec_block(const uint8_t* src, size_t ssz, uint8_t* dst, uint32_t n) {
    if (ssz < 12) return 0;
    const uint32_t ntok = rd32(src), next = rd32(src + 4), tail = rd32(src + 8);
    if ((uint64_t)12 + 4ull * ntok + next > ssz) return 0;
    const uint32_t* tok = (const uint32_t*)(src + 12); const uint8_t* ext = src + 12 + 4 * ntok;
    const uint8_t* lit = ext + next; const uint8_t* lend = src + ssz; const uint8_t* eend = lit;
    uint8_t* d = dst; const uint8_t* dend = dst + n;
    for (uint32_t i = 0; i < ntok; i++) {
        const uint32_t t = tok[i]; uint32_t ll = t & 63, mc = (t >> 6) & 63; const uint32_t dist = (t >> 12) + MIN_DIST;
        if (__builtin_expect(ll == LL_EXT, 0)) {                       // rare: long literal run
            uint32_t r = 0; for (;;) { if (ext >= eend) return 0; uint8_t b = *ext++; r += b; if (b < 255) break; }
            ll += r; if (lit + ll > lend || d + ll > dend) return 0;
            memcpy(d, lit, ll);
        } else copy_le64(d, lit, ll);                                   // the read stays inside the block's padding
        d += ll; lit += ll;
        if (__builtin_expect((uint32_t)(d - dst) < dist, 0)) return 0;   // source before the block
        if (__builtin_expect(mc == ML_EXT, 0)) {                       // rare: long match, 64-byte steps (MIN_DIST makes them exact)
            uint32_t r = 0; for (;;) { if (ext >= eend) return 0; uint8_t b = *ext++; r += b; if (b < 255) break; }
            const uint32_t ml = COPY + 1 + r; if (d + ml > dend) return 0;
            for (uint32_t k = 0; k < ml; k += COPY) copy64(d + k, d + k - dist); d += ml;
        } else { copy_le64(d, d - dist, mc + MIN_MATCH); d += mc + MIN_MATCH; }
        if (__builtin_expect(d > dend || lit > lend, 0)) return 0;      // overshoot per token < SLACK: caught here, writes stayed in the slack
    }
    if (lit + tail != lend || d + tail != dend) return 0;
    memcpy(d, lit, tail);
    return n;
}


// ---------------------------------------------------------------- safe decoder (#D1-2)
// The same format with no padding on either side: src is exactly ssz bytes, dst exactly n bytes. Any input either
// decodes to n bytes or returns 0, and never reads outside [src, src + ssz) or writes outside [dst, dst + n).
// Fast path (the unconditional CW-wide copies) only while a whole token fits in the margins: at least 64 literal
// bytes left to read and 64 + (62 + MIN_MATCH rounded up to CW) output bytes left to write; otherwise the exact path (memcpy of the true lengths,
// match copies in steps of dist, which keeps an overlapping copy exact).
static inline uint32_t rd_ext(const uint8_t*& ext, const uint8_t* eend, bool& bad) {
    uint32_t r = 0; for (;;) { if (ext >= eend) { bad = true; return 0; } uint8_t b = *ext++; r += b; if (b < 255) return r; if (r > (1u << 30)) { bad = true; return 0; } }
}
static uint32_t dec_block_safe(const uint8_t* src, size_t ssz, uint8_t* dst, uint32_t n) {
    if (ssz < 12) return 0;
    const uint32_t ntok = rd32(src), next = rd32(src + 4), tail = rd32(src + 8);
    if ((uint64_t)12 + 4ull * ntok + next > ssz) return 0;
    const uint8_t* tp = src + 12; const uint8_t* ext = tp + 4ull * ntok;
    const uint8_t* lit = ext + next; const uint8_t* const lend = src + ssz; const uint8_t* const eend = lit;
    uint8_t* d = dst; uint8_t* const dend = dst + n; bool bad = false;
    const ptrdiff_t M = 64 + (ptrdiff_t)((62 + MIN_MATCH + CW - 1) / CW * CW);   // literal copy + widest direct match copy
    for (uint32_t i = 0; i < ntok; i++) {
        const uint32_t t = rd32(tp + 4ull * i); uint32_t ll = t & 63, mc = (t >> 6) & 63; const uint32_t dist = (t >> 12) + MIN_DIST;
        if (__builtin_expect(ll != LL_EXT && mc != ML_EXT && lend - lit >= 64 && dend - d >= M, 1)) {
            copy_le64(d, lit, ll); d += ll; lit += ll;                   // reads lit..lit+63, writes d..d+63: inside both
            if (__builtin_expect((size_t)(d - dst) < dist, 0)) return 0;
            const uint32_t ml = mc + MIN_MATCH;
            copy_le64(d, d - dist, ml); d += ml;                         // writes < M - 62 bytes from d: inside dst
            continue;
        }
        if (ll == LL_EXT) { ll += rd_ext(ext, eend, bad); if (bad) return 0; }
        if ((size_t)(lend - lit) < ll || (size_t)(dend - d) < ll) return 0;
        memcpy(d, lit, ll); d += ll; lit += ll;
        if ((size_t)(d - dst) < dist) return 0;
        uint32_t ml = mc + MIN_MATCH;
        if (mc == ML_EXT) { ml = COPY + 1 + rd_ext(ext, eend, bad); if (bad) return 0; }
        if ((size_t)(dend - d) < ml) return 0;
        for (uint32_t k = 0; k < ml; ) { uint32_t c = std::min(ml - k, dist); memcpy(d + k, d + k - dist, c); k += c; }
        d += ml;
    }
    if ((size_t)(lend - lit) != tail || (size_t)(dend - d) != tail) return 0;
    memcpy(d, lit, tail);
    return n;
}
