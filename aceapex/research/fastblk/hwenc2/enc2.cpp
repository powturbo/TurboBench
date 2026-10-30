// enc2.cpp - HW-ENC2 research harness: a fast FAST-profile encoder (format unchanged, decoder unchanged) against level 11
// and the HW-ENC single-slot candidate. Every archive is decoded by src/ax_fast.h and compared with the input (bit-perfect)
// outside the timer. Variant knobs: hash bits, bucket ways, key bytes, insertion stride inside matches, backward extension,
// lazy steps, skip shift.
// Build: g++ -O3 -std=c++17 -march=native -I<repo>/src enc2.cpp -o enc2
// Run:   enc2 FILE [reps=5] [variant spec ...]   spec: name:hb=17,w=4,key=8,ins=1,back=1,lazy=1,skip=7,mm=16
#define XXH_STATIC_LINKING_ONLY
#define XXH_IMPLEMENTATION
#include "xxhash.h"
#include "ax_fast_enc.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <algorithm>
#include <stdexcept>

using Bytes = std::vector<uint8_t>;

struct V { std::string name; int hb = 17, acc = 0, mx = 0, base = 1, pi = 0, w = 1, key = 8, ins = 1, back = 0, lazy = 0, skip = 7, mm = 16, kind = 2; };
// kind 0 = level 11 (library), 1 = HW-ENC single slot (as in HW_ENC_WIP), 2 = enc2

struct Enc2 {
    V v; std::vector<uint32_t> tab; std::vector<uint8_t> rot; axf::Out o;
    explicit Enc2(const V& vv) : v(vv) { tab.assign((size_t)v.w << v.hb, 0); }
    inline uint32_t H(const uint8_t* p) const {
        uint64_t x = axf::rd64(p); if (v.key < 8) x <<= (64 - 8 * v.key);
        return (uint32_t)((x * 0x9E3779B185EBCA87ull) >> (64 - v.hb));
    }
    // position p (>= 0) into its bucket: newest first (shift), entries store p + 1 (0 = empty)
    inline void insert(const uint8_t* in, uint32_t p) {
        uint32_t* b = &tab[(size_t)H(in + p) * v.w];
        for (int i = v.w - 1; i > 0; i--) b[i] = b[i - 1];
        b[0] = p + 1;
    }
    // longest legal match at pos among the bucket; returns length (0 if < mm), distance in d
    inline uint32_t find(const uint8_t* in, uint32_t n, uint32_t pos, uint32_t& d) const {
        const uint32_t* b = &tab[(size_t)H(in + pos) * v.w]; uint32_t best = 0; const uint64_t k = axf::rd64(in + pos);
        const uint32_t lim = n - pos;
        for (int i = 0; i < v.w; i++) {
            if (!b[i]) break; const uint32_t c = b[i] - 1;
            if (pos - c < AXF_MIN_DIST) continue;
            if (axf::rd64(in + c) != k) continue;
            uint32_t l = 8;
            while (l + 8 <= lim) { const uint64_t x = axf::rd64(in + c + l) ^ axf::rd64(in + pos + l); if (x) { l += axf::ctz64(x) >> 3; goto done; } l += 8; }
            while (l < lim && in[c + l] == in[pos + l]) l++;
            done:
            if (l > best) { best = l; d = pos - c; }
        }
        return best >= (uint32_t)v.mm ? best : 0;
    }
    __attribute__((noinline)) size_t block(const uint8_t* in, uint32_t n, uint8_t* out) {
        std::fill(tab.begin(), tab.end(), 0u);
        o.tok.clear(); o.ext.clear(); o.lit.clear(); o.lit.reserve(n); o.tok.reserve(n / 16 + 1);
        const uint32_t mm = v.mm;
        if (n < 80) { o.lit.insert(o.lit.end(), in, in + n); return o.write(out, n); }
        const uint32_t last = n - 16;          // probe needs 8 key bytes; keep a margin
        uint32_t pos = AXF_MIN_DIST, anchor = 0, ins = 0;   // positions < ins are in the table
        enum : uint32_t { QM = (1u << 16) - 1 }; static thread_local uint32_t q[QM + 1]; uint32_t qh = 0, qt = 0;
        if (v.pi) for (uint32_t p = 0; p < AXF_MIN_DIST; p += 1) q[qt++ & QM] = p;
        while (pos < last) {
            // delayed insertion: every position up to pos - 64 (so a bucket never offers an illegal distance first)
            const uint32_t upto = pos - AXF_MIN_DIST;
            if (v.pi) {
                while (qh != qt && q[qh & QM] <= upto) { insert(in, q[qh & QM]); qh++; }
                uint32_t d = 0, len = find(in, n, pos, d);
                q[qt++ & QM] = pos;
                if (!len) { pos += 1 + ((pos - anchor) >> v.skip); continue; }
                if (v.back) { while (pos > anchor && pos > d && in[pos - 1] == in[pos - 1 - d]) { pos--; len++; } }
                o.lit.insert(o.lit.end(), in + anchor, in + pos);
                o.put(pos - anchor, len, d, mm);
                // positions inside the match: every v.ins-th, plus the last two
                for (uint32_t p = pos + v.ins; p + 2 < pos + len; p += v.ins) q[qt++ & QM] = p;
                if (len > 2) { q[qt++ & QM] = pos + len - 2; }
                if (qt - qh > QM) qh = qt - QM;   // FIFO overflow: drop the oldest (never happens below 4096 pending)
                pos += len; anchor = pos;
                continue;
            }
            if (upto - ins > 64 && v.ins > 1) { // a jump (after a match or a skip): sparse inside it
                for (; ins + 64 < upto; ins += v.ins) insert(in, ins);
            }
            for (; ins <= upto; ins++) insert(in, ins);
            uint32_t d = 0, len = find(in, n, pos, d);
            if (!len) { pos += 1 + ((pos - anchor) >> v.skip); continue; }
            if (v.lazy) {
                for (int s = 1; s <= v.lazy && pos + s < last; s++) {
                    // the positions in front of pos + s are legal only once inserted: insert up to pos + s - 64
                    for (; ins <= pos + s - AXF_MIN_DIST; ins++) insert(in, ins);
                    uint32_t d2 = 0; const uint32_t l2 = find(in, n, pos + s, d2);
                    if (l2 > len + (uint32_t)s) { len = l2; d = d2; pos += s; s = 0; }
                }
            }
            if (v.back) { while (pos > anchor && pos > d && in[pos - 1] == in[pos - 1 - d]) { pos--; len++; } }
            o.lit.insert(o.lit.end(), in + anchor, in + pos);
            o.put(pos - anchor, len, d, mm);
            pos += len; anchor = pos;
        }
        o.lit.insert(o.lit.end(), in + anchor, in + n);
        return o.write(out, n - anchor);
    }
};


// Fx<W, HB>: the HW-ENC2 encoder, compile-time ways and table bits. Probe-driven insertion through a FIFO of
// (position, hash): a probed position enters its bucket once it is >= 64 bytes behind the probe (every candidate a bucket
// offers has a legal distance); inside a match every INS-th position and the one 2 before its end are queued; a match is
// extended backward into the pending literals; misses step 1 + (distance from the last match >> SKIP).
template <int W, int HB>
struct Fx {
    int ins, skip, back; uint32_t mm; std::vector<uint32_t> tab; std::vector<uint32_t> qp, qh; std::vector<uint8_t> lit; axf::Out o;
    Fx(int ins_, int skip_, int back_, uint32_t mm_) : ins(ins_), skip(skip_), back(back_), mm(mm_), tab((size_t)W << HB), qp(1u << 16), qh(1u << 16), lit(AXF_BLOCK_MAX + 64) {}
    static inline uint32_t H(const uint8_t* p) { return (uint32_t)((axf::rd64(p) * 0x9E3779B185EBCA87ull) >> (64 - HB)); }
    inline void insert(uint32_t h, uint32_t p) {
        uint32_t* b = &tab[(size_t)h * W];
        for (int i = W - 1; i > 0; i--) b[i] = b[i - 1];
        b[0] = p + 1;
    }
    __attribute__((noinline)) size_t block(const uint8_t* in, uint32_t n, uint8_t* out) {
        std::fill(tab.begin(), tab.end(), 0u);
        o.tok.clear(); o.ext.clear(); o.tok.reserve(n / 16 + 1);
        uint8_t* L = lit.data(); size_t nl = 0;
        if (n < 80) { o.lit.assign(in, in + n); return o.write(out, n); }
        const uint32_t last = n - 16; const uint32_t QM = (1u << 16) - 1;
        uint32_t* QP = qp.data(); uint32_t* QH = qh.data(); uint32_t h0 = 0, t0 = 0;
        for (uint32_t p = 0; p < AXF_MIN_DIST; p += 4) { QP[t0 & QM] = p; QH[t0 & QM] = H(in + p); t0++; }
        uint32_t pos = AXF_MIN_DIST, anchor = 0;
        while (pos < last) {
            const uint32_t upto = pos - AXF_MIN_DIST;
            while (h0 != t0 && QP[h0 & QM] <= upto) { insert(QH[h0 & QM], QP[h0 & QM]); h0++; }
            const uint32_t h = H(in + pos); const uint32_t* b = &tab[(size_t)h * W]; const uint64_t k = axf::rd64(in + pos);
            uint32_t best = 0, d = 0; const uint32_t lim = n - pos;
            for (int i = 0; i < W; i++) {
                const uint32_t c1 = b[i]; if (!c1) break; const uint32_t c = c1 - 1;
                if (axf::rd64(in + c) != k) continue;
                uint32_t l = 8;
                for (;;) { if (l + 8 > lim) { while (l < lim && in[c + l] == in[pos + l]) l++; break; }
                           const uint64_t x = axf::rd64(in + c + l) ^ axf::rd64(in + pos + l); if (x) { l += axf::ctz64(x) >> 3; break; } l += 8; }
                if (l > best) { best = l; d = pos - c; }
            }
            QP[t0 & QM] = pos; QH[t0 & QM] = h; t0++;
            if (best < mm) { pos += 1 + ((pos - anchor) >> skip); continue; }
            uint32_t len = best;
            if (back) { while (pos > anchor && pos > d && in[pos - 1] == in[pos - 1 - d]) { pos--; len++; } }
            memcpy(L + nl, in + anchor, pos - anchor); nl += pos - anchor;
            o.put(pos - anchor, len, d, mm);
            const uint32_t end = pos + len;
            for (uint32_t p = pos + ins; p + 2 < end && p < last; p += ins) { QP[t0 & QM] = p; QH[t0 & QM] = H(in + p); t0++; }
            if (end - 2 < last) { QP[t0 & QM] = end - 2; QH[t0 & QM] = H(in + end - 2); t0++; }
            if (t0 - h0 > QM) h0 = t0 - QM;
            pos = end; anchor = end;
        }
        memcpy(L + nl, in + anchor, n - anchor); nl += n - anchor;
        // block layout as axf::Out::write, literals from our buffer
        uint8_t* p = out; axf::wr32(p, (uint32_t)o.tok.size()); axf::wr32(p + 4, (uint32_t)o.ext.size()); axf::wr32(p + 8, n - anchor); p += AXF_BHDR;
        memcpy(p, o.tok.data(), o.tok.size() * 4); p += o.tok.size() * 4; if (!o.ext.empty()) memcpy(p, o.ext.data(), o.ext.size()); p += o.ext.size();
        memcpy(p, L, nl); p += nl; memset(p, 0, AXF_PAD); p += AXF_PAD;
        return (size_t)(p - out);
    }
};
struct FxAny { virtual size_t block(const uint8_t*, uint32_t, uint8_t*) = 0; virtual ~FxAny() {} };
template <int W, int HB> struct FxI : FxAny { Fx<W, HB> f; FxI(int a, int b, int c, uint32_t m) : f(a, b, c, m) {} size_t block(const uint8_t* i, uint32_t n, uint8_t* o) override { return f.block(i, n, o); } };
static FxAny* mkfx(const V& v) {
#define C(W, HB) if (v.w == W && v.hb == HB) return new FxI<W, HB>(v.ins, v.skip, v.back, v.mm);
    C(1,16) C(1,17) C(1,18) C(2,15) C(2,16) C(2,17) C(4,14) C(4,15) C(4,16) C(8,14) C(8,15)
#undef C
    throw std::runtime_error("no Fx instance for w/hb");
}


// Fy<W, HB>: immediate insertion (lz4 style, no FIFO). A candidate closer than 64 bytes is lifted to the first legal
// multiple of its distance (d' = d * ceil(64 / d)): on a run of period d the bytes there are the same, the match is verified
// like any other. Inside a match every INS-th position and the one 2 before its end are inserted.
template <int W, int HB>
struct Fy {
    int ins, skip, back, base; uint32_t mm; std::vector<uint32_t> tab; std::vector<uint8_t> lit; axf::Out o;
    Fy(int ins_, int skip_, int back_, uint32_t mm_, int base_) : ins(ins_), skip(skip_), back(back_), base(base_), mm(mm_), tab((size_t)W << HB), lit(AXF_BLOCK_MAX + 64) {}
    static inline uint32_t H(const uint8_t* p) { return (uint32_t)((axf::rd64(p) * 0x9E3779B185EBCA87ull) >> (64 - HB)); }
    inline void insert(uint32_t h, uint32_t p) { uint32_t* b = &tab[(size_t)h * W]; for (int i = W - 1; i > 0; i--) b[i] = b[i - 1]; b[0] = p + 1; }
    static inline uint32_t mlen(const uint8_t* in, uint32_t c, uint32_t pos, uint32_t lim) {
        uint32_t l = 8;
        for (;;) { if (l + 8 > lim) { while (l < lim && in[c + l] == in[pos + l]) l++; return l; }
                   const uint64_t x = axf::rd64(in + c + l) ^ axf::rd64(in + pos + l); if (x) return l + (axf::ctz64(x) >> 3); l += 8; }
    }
    __attribute__((noinline)) size_t block(const uint8_t* in, uint32_t n, uint8_t* out) {
        std::fill(tab.begin(), tab.end(), 0u);
        o.tok.clear(); o.ext.clear(); o.tok.reserve(n / 16 + 1);
        uint8_t* L = lit.data(); size_t nl = 0;
        if (n < 80) { o.lit.assign(in, in + n); return o.write(out, n); }
        const uint32_t last = n - 16;
        uint32_t pos = 0, anchor = 0;
        while (pos < last) {
            const uint32_t h = H(in + pos); uint32_t* b = &tab[(size_t)h * W]; const uint64_t k = axf::rd64(in + pos);
            uint32_t best = 0, d = 0; const uint32_t lim = n - pos;
            for (int i = 0; i < W; i++) {
                const uint32_t c1 = b[i]; if (!c1) break; uint32_t c = c1 - 1;
                if (axf::rd64(in + c) != k) continue;
                uint32_t dd = pos - c;
                if (dd < AXF_MIN_DIST) { dd *= (AXF_MIN_DIST + dd - 1) / dd; if (dd > pos) continue; c = pos - dd; if (axf::rd64(in + c) != k) continue; }
                const uint32_t l = mlen(in, c, pos, lim);
                if (l > best) { best = l; d = dd; }
            }
            for (int i = W - 1; i > 0; i--) b[i] = b[i - 1]; b[0] = pos + 1;
            if (best < mm) { pos += base + ((pos - anchor) >> skip); continue; }
            uint32_t len = best;
            if (back) { while (pos > anchor && pos > d && in[pos - 1] == in[pos - 1 - d]) { pos--; len++; } }
            memcpy(L + nl, in + anchor, pos - anchor); nl += pos - anchor;
            o.put(pos - anchor, len, d, mm);
            const uint32_t end = pos + len;
            for (uint32_t p = pos + ins; p + 2 < end && p < last; p += ins) insert(H(in + p), p);
            if (end - 2 < last) insert(H(in + end - 2), end - 2);
            pos = end; anchor = end;
        }
        memcpy(L + nl, in + anchor, n - anchor); nl += n - anchor;
        uint8_t* p = out; axf::wr32(p, (uint32_t)o.tok.size()); axf::wr32(p + 4, (uint32_t)o.ext.size()); axf::wr32(p + 8, n - anchor); p += AXF_BHDR;
        memcpy(p, o.tok.data(), o.tok.size() * 4); p += o.tok.size() * 4; if (!o.ext.empty()) memcpy(p, o.ext.data(), o.ext.size()); p += o.ext.size();
        memcpy(p, L, nl); p += nl; memset(p, 0, AXF_PAD); p += AXF_PAD;
        return (size_t)(p - out);
    }
};
template <int W, int HB> struct FyI : FxAny { Fy<W, HB> f; FyI(int a, int b, int c, uint32_t m, int bs) : f(a, b, c, m, bs) {} size_t block(const uint8_t* i, uint32_t n, uint8_t* o) override { return f.block(i, n, o); } };
static FxAny* mkfy(const V& v) {
#define C(W, HB) if (v.w == W && v.hb == HB) return new FyI<W, HB>(v.ins, v.skip, v.back, v.mm, v.base);
    C(1,16) C(1,17) C(1,18) C(2,15) C(2,16) C(2,17) C(4,14) C(4,15) C(4,16) C(8,14) C(8,15)
#undef C
    throw std::runtime_error("no Fy instance for w/hb");
}


// Fz<W, HB>: sparse probe, sparse insert, coprime strides. Positions == 0 mod B enter the table (as the scan passes them,
// also inside literals that are skipped and, every INS-th, inside matches); probes land every A bytes (A, B coprime), so a
// match whose source and target overlap probes for lcm(A, B) bytes is found at some probe and extended backward to its
// start. Cost per byte ~ 1/A probes + 1/B inserts. Candidates closer than 64 are lifted to a multiple of their distance.
template <int W, int HB>
struct Fz {
    int A, B, ins, skip, acc = 0; uint32_t mm; std::vector<uint32_t> tab; std::vector<uint8_t> lit; axf::Out o;
    Fz(int a, int b, int ins_, int skip_, uint32_t mm_) : A(a), B(b), ins(ins_), skip(skip_), mm(mm_), tab((size_t)W << HB), lit(AXF_BLOCK_MAX + 64) {}
    static inline uint32_t H(const uint8_t* p) { return (uint32_t)((axf::rd64(p) * 0x9E3779B185EBCA87ull) >> (64 - HB)); }
    inline void insert(uint32_t p, const uint8_t* in) { uint32_t* b = &tab[(size_t)H(in + p) * W]; for (int i = W - 1; i > 0; i--) b[i] = b[i - 1]; b[0] = p + 1; }
    static inline uint32_t mlen(const uint8_t* in, uint32_t c, uint32_t pos, uint32_t lim) {
        uint32_t l = 8;
        for (;;) { if (l + 8 > lim) { while (l < lim && in[c + l] == in[pos + l]) l++; return l; }
                   const uint64_t x = axf::rd64(in + c + l) ^ axf::rd64(in + pos + l); if (x) return l + (axf::ctz64(x) >> 3); l += 8; }
    }
    __attribute__((noinline)) size_t block(const uint8_t* in, uint32_t n, uint8_t* out) {
        std::fill(tab.begin(), tab.end(), 0u);
        o.tok.clear(); o.ext.clear(); o.tok.reserve(n / 16 + 1);
        uint8_t* L = lit.data(); size_t nl = 0;
        if (n < 80) { o.lit.assign(in, in + n); return o.write(out, n); }
        const uint32_t last = n - 16;
        uint32_t pos = 0, anchor = 0, ip = 0;   // ip: next position == 0 mod B not yet inserted
        while (pos < last) {
            if (acc && pos - anchor > (1u << acc)) {   // a long literal run: insert sparser, step B << (run >> acc), at most 8 B
                uint32_t sh = (pos - anchor) >> acc; const uint32_t st = (uint32_t)B * (sh >= 8 ? 8u : sh + 1);
                for (; ip < pos; ip += st) insert(ip, in);
            } else for (; ip < pos; ip += B) insert(ip, in);
            const uint32_t h = H(in + pos); const uint32_t* b = &tab[(size_t)h * W]; const uint64_t k = axf::rd64(in + pos);
            uint32_t best = 0, d = 0; const uint32_t lim = n - pos;
            for (int i = 0; i < W; i++) {
                const uint32_t c1 = b[i]; if (!c1) break; uint32_t c = c1 - 1;
                if (axf::rd64(in + c) != k) continue;
                uint32_t dd = pos - c;
                if (dd < AXF_MIN_DIST) { dd *= (AXF_MIN_DIST + dd - 1) / dd; if (dd > pos) continue; c = pos - dd; if (axf::rd64(in + c) != k) continue; }
                const uint32_t l = mlen(in, c, pos, lim);
                if (l > best) { best = l; d = dd; }
            }
            if (best < 8) { pos += A + (((pos - anchor) >> skip) / A) * A; continue; }
            uint32_t len = best; const uint32_t probe = pos;
            {   // backward extension, 8 bytes at a time while both sides have 8 bytes before them inside [anchor, pos)
                uint32_t room = pos - anchor; if (room > pos - d) room = pos - d;
                while (room >= 8) { const uint64_t x = axf::rd64(in + pos - 8) ^ axf::rd64(in + pos - 8 - d);
                    if (x) { const uint32_t k = (uint32_t)__builtin_clzll(x) >> 3; pos -= k; len += k; room = 0; break; } pos -= 8; len += 8; room -= 8; }
                while (room && in[pos - 1] == in[pos - 1 - d]) { pos--; len++; room--; }
            }
            if (len < mm) { pos = probe + A; continue; }
            memcpy(L + nl, in + anchor, pos - anchor); nl += pos - anchor;
            o.put(pos - anchor, len, d, mm);
            const uint32_t end = pos + len;
            // inside the match: positions == 0 mod B, every INS-th of them; then continue the dense stream from the end
            if (ip < end) { for (; ip + 8 < end && ip < last; ip += (uint32_t)(B * ins)) insert(ip, in); ip = end + ((B - end % B) % B); }
            if (end - 2 < last) insert(end - 2, in);
            pos = end; anchor = end;
        }
        memcpy(L + nl, in + anchor, n - anchor); nl += n - anchor;
        uint8_t* p = out; axf::wr32(p, (uint32_t)o.tok.size()); axf::wr32(p + 4, (uint32_t)o.ext.size()); axf::wr32(p + 8, n - anchor); p += AXF_BHDR;
        memcpy(p, o.tok.data(), o.tok.size() * 4); p += o.tok.size() * 4; if (!o.ext.empty()) memcpy(p, o.ext.data(), o.ext.size()); p += o.ext.size();
        memcpy(p, L, nl); p += nl; memset(p, 0, AXF_PAD); p += AXF_PAD;
        return (size_t)(p - out);
    }
};
template <int W, int HB> struct FzI : FxAny { Fz<W, HB> f; FzI(int a, int b, int c, int d, uint32_t m) : f(a, b, c, d, m) {} size_t block(const uint8_t* i, uint32_t n, uint8_t* o) override { return f.block(i, n, o); } };
static FxAny* mkfz(const V& v) {
#define C(W, HB) if (v.w == W && v.hb == HB) { auto* z = new FzI<W, HB>(v.base, v.pi, v.ins, v.skip, v.mm); z->f.acc = v.acc; return z; }
    C(1,15) C(1,16) C(1,17) C(1,18) C(2,15) C(2,16) C(2,17) C(4,14) C(4,15) C(4,16)
#undef C
    throw std::runtime_error("no Fz instance for w/hb");
}


// Fw<HB, A, B>: Fz with the strides as constants and the output written in place: tokens, extensions and literals go
// to preallocated arrays (literals by 32-byte wild copies into a buffer with slack), no vector growth checks.
template <int HB, int A, int B, int K = 8>
struct Fw {
    int ins, skip; uint32_t mm, macc; std::vector<uint32_t> tab, tok; std::vector<uint8_t> lit, ext;
    Fw(int ins_, int skip_, uint32_t mm_) : ins(ins_), skip(skip_), mm(mm_), macc(mm_), tab(1u << HB), tok(AXF_BLOCK_MAX / 8 + 16), lit(AXF_BLOCK_MAX + 128), ext(AXF_BLOCK_MAX / 16 + 64) {}
    static inline uint64_t key(const uint8_t* p) { return K == 8 ? axf::rd64(p) : axf::rd64(p) << (64 - 8 * K); }   // the first K bytes
    static inline uint32_t H(const uint8_t* p) { return (uint32_t)((key(p) * 0x9E3779B185EBCA87ull) >> (64 - HB)); }
    static inline void wild(uint8_t* d, const uint8_t* s, size_t k) { for (size_t i = 0; i < k; i += 32) { uint8_t t[32]; memcpy(t, s + i, 32); memcpy(d + i, t, 32); } }
    __attribute__((noinline)) size_t block(const uint8_t* in, uint32_t n, uint8_t* out) {
        uint32_t* T = tab.data(); memset(T, 0, tab.size() * 4);
        uint32_t* TK = tok.data(); uint32_t nt = 0; uint8_t* E = ext.data(); uint32_t ne = 0; uint8_t* L = lit.data(); size_t nl = 0;
        auto leb = [&](uint32_t r) { while (r >= 255) { E[ne++] = 255; r -= 255; } E[ne++] = (uint8_t)r; };
        uint32_t pos = 0, anchor = 0, ip = 0;
        if (n >= 80) {
        const uint32_t last = n - 16;
        // the literal copy reads up to 31 bytes past the run: safe while the run ends <= n - 32, else exact copy
        auto litcopy = [&](uint32_t a, uint32_t b) { const size_t k = b - a; if (b + 32 <= n) wild(L + nl, in + a, k); else memcpy(L + nl, in + a, k); nl += k; };
        while (pos < last) {
            for (; ip < pos; ip += B) T[H(in + ip)] = ip + 1;
            const uint32_t h = H(in + pos); const uint32_t c1 = T[h];
            uint32_t len = 0, d = 0;
            if (c1) {
                uint32_t c = c1 - 1; const uint64_t k = key(in + pos);
                if (key(in + c) == k) {
                    d = pos - c;
                    if (d < AXF_MIN_DIST) { d *= (AXF_MIN_DIST + d - 1) / d; if (d <= pos && key(in + pos - d) == k) c = pos - d; else d = 0; }
                    if (d) {
                        const uint32_t lim = n - pos; uint32_t l = K;
                        for (;;) { if (l + 8 > lim) { while (l < lim && in[c + l] == in[pos + l]) l++; break; }
                                   const uint64_t x = axf::rd64(in + c + l) ^ axf::rd64(in + pos + l); if (x) { l += axf::ctz64(x) >> 3; break; } l += 8; }
                        len = l;
                    }
                }
            }
            if (!len) { pos += A + ((pos - anchor) >> skip); continue; }
            const uint32_t probe = pos;
            {   uint32_t room = pos - anchor; if (room > pos - d) room = pos - d;
                while (room >= 8) { const uint64_t x = axf::rd64(in + pos - 8) ^ axf::rd64(in + pos - 8 - d);
                    if (x) { const uint32_t k = (uint32_t)__builtin_clzll(x) >> 3; pos -= k; len += k; room = 0; break; } pos -= 8; len += 8; room -= 8; }
                while (room && in[pos - 1] == in[pos - 1 - d]) { pos--; len++; room--; } }
            if (len < macc) { pos = probe + A; continue; }
            litcopy(anchor, pos);
            const uint32_t ll = pos - anchor; uint32_t llf = ll; if (ll >= axf::LL_EXT) { llf = axf::LL_EXT; leb(ll - axf::LL_EXT); }
            uint32_t mlf; if (len <= AXF_COPY) mlf = len - mm; else { mlf = axf::ML_EXT; leb(len - AXF_COPY - 1); }
            TK[nt++] = llf | mlf << 6 | (d - AXF_MIN_DIST) << 12;
            const uint32_t end = pos + len;
            if (ip < end) { const uint32_t st = (uint32_t)(B * ins); for (; ip + 8 < end && ip < last; ip += st) T[H(in + ip)] = ip + 1; ip = end + ((B - end % B) % B); }
            if (end - 2 < last) T[H(in + end - 2)] = end - 2 + 1;
            pos = end; anchor = end;
        }
        }
        memcpy(L + nl, in + anchor, n - anchor); nl += n - anchor;
        uint8_t* p = out; axf::wr32(p, nt); axf::wr32(p + 4, ne); axf::wr32(p + 8, n - anchor); p += AXF_BHDR;
        memcpy(p, TK, (size_t)nt * 4); p += (size_t)nt * 4; memcpy(p, E, ne); p += ne;
        memcpy(p, L, nl); p += nl; memset(p, 0, AXF_PAD); p += AXF_PAD;
        return (size_t)(p - out);
    }
};
template <int HB, int A, int B, int K = 8> struct FwI : FxAny { Fw<HB, A, B, K> f; FwI(int a, int b, uint32_t m) : f(a, b, m) {} size_t block(const uint8_t* i, uint32_t n, uint8_t* o) override { return f.block(i, n, o); } };
static FxAny* mkfw(const V& v) {
#define CK(HB, A, B, K) if (v.hb == HB && v.base == A && v.pi == B && v.key == K) { auto* w = new FwI<HB, A, B, K>(v.ins, v.skip, v.mm); w->f.macc = v.mm + v.mx; return w; }
#define C(HB, A, B) CK(HB, A, B, 8)
    CK(17,7,2,8) CK(17,5,3,6) CK(17,9,2,6) CK(17,7,2,6) CK(17,11,2,5) CK(17,9,2,5) CK(17,5,3,5) CK(17,7,2,5) CK(17,11,2,4) CK(17,5,3,4) CK(17,4,3,6) CK(17,3,2,6) CK(16,7,2,6) CK(16,9,2,5) CK(16,5,3,6)
    C(15,4,3) C(16,4,3) C(17,4,3) C(15,3,2) C(16,3,2) C(17,3,2) C(15,5,2) C(16,5,2) C(17,5,2) C(15,5,3) C(16,5,3) C(16,4,1) C(16,8,1) C(15,7,2) C(16,7,2) C(15,5,4) C(16,5,4) C(16,7,3) C(15,7,3)
#undef C
#undef CK
    throw std::runtime_error("no Fw instance");
}

// HW-ENC single slot, verbatim logic of HW_ENC_WIP/0001-fast-encoder.patch
struct Slot {
    static constexpr unsigned HB = 17; std::vector<uint32_t> table = std::vector<uint32_t>(1u << HB); axf::Out o;
    static unsigned hash(const uint8_t* p) { return unsigned((axf::rd64(p) * 0x9e3779b185ebca87ULL) >> (64 - HB)); }
    __attribute__((noinline)) size_t block(const uint8_t* in, uint32_t n, uint8_t* out) {
        std::fill(table.begin(), table.end(), UINT32_MAX); o.tok.clear(); o.ext.clear(); o.lit.clear(); o.lit.reserve(n); o.tok.reserve(n / 16 + 1);
        uint32_t pos = 64, anchor = 0;
        while (pos + 16 <= n) {
            const uint32_t eligible = pos - 64; table[hash(in + eligible)] = eligible;
            uint32_t cand = table[hash(in + pos)];
            if (cand != UINT32_MAX && cand <= pos - 64 && axf::rd64(in + cand) == axf::rd64(in + pos) && axf::rd64(in + cand + 8) == axf::rd64(in + pos + 8)) {
                uint32_t len = 16;
                while (len + 8 <= n - pos && axf::rd64(in + cand + len) == axf::rd64(in + pos + len)) len += 8;
                while (len < n - pos && in[cand + len] == in[pos + len]) ++len;
                o.lit.insert(o.lit.end(), in + anchor, in + pos); o.put(pos - anchor, len, pos - cand, 16); pos += len; anchor = pos;
            } else pos += 1 + ((pos - anchor) >> 7);
        }
        o.lit.insert(o.lit.end(), in + anchor, in + n); return o.write(out, n - anchor);
    }
};

static double g_blk;   // seconds inside the block encoders only (no buffer zeroing, no archive assembly)
static Bytes archive(const Bytes& in, const V& v) {
    g_blk = 0;
    std::vector<Bytes> blocks; Enc2 e2(v); FxAny* fx = v.kind == 3 ? mkfx(v) : v.kind == 4 ? mkfy(v) : v.kind == 5 ? mkfz(v) : v.kind == 6 ? mkfw(v) : nullptr; Slot sl; axf::Enc old; axf::Params P; axf::level_params(1, P);
    if (v.kind == 0) { axf::level_params(v.mm == 16 ? 1 : v.mm == 12 ? 2 : v.mm == 8 ? 3 : 4, P); }
    for (size_t off = 0; off < in.size(); off += AXF_BLOCK_MAX) {
        const uint32_t n = (uint32_t)std::min<size_t>(AXF_BLOCK_MAX, in.size() - off);
        Bytes b(axf::enc_bound(n, v.mm));
        const auto tb = std::chrono::steady_clock::now();
        const size_t used = v.kind == 0 ? axf::enc_block(in.data() + off, n, b.data(), P, old)
                          : v.kind == 1 ? sl.block(in.data() + off, n, b.data()) : v.kind >= 3 ? fx->block(in.data() + off, n, b.data()) : e2.block(in.data() + off, n, b.data());
        g_blk += std::chrono::duration<double>(std::chrono::steady_clock::now() - tb).count();
        b.resize(used); blocks.push_back(std::move(b));
    }
    delete fx; Bytes r(axf::archive_size(blocks)); uint64_t pay = 0; for (auto& b : blocks) pay += b.size();
    axf::write_header(r.data(), in.size(), AXF_BLOCK_MAX, (uint32_t)blocks.size(), XXH3_64bits(in.data(), in.size()), v.kind == 1 ? 16 : v.mm, pay);
    axf::write_body(r.data() + 68, blocks);
    return r;
}
static void judge(const Bytes& in, const Bytes& a, double& dec_s) {
    axf_arc_t arc; if (axf_open(a.data(), a.size(), &arc) || arc.orig != in.size()) throw std::runtime_error("header rejected");
    Bytes out(in.size());
    static const int DR = getenv("DECR") ? atoi(getenv("DECR")) : 1; dec_s = 1e9;
    for (int r = 0; r < DR; r++) {
        const auto t0 = std::chrono::steady_clock::now();
        for (uint32_t b = 0; b < arc.nb; b++) if (axf_dec_block_of(&arc, b, out.data() + (size_t)b * arc.bs) != axf_block_len(&arc, b)) throw std::runtime_error("decode failed");
        dec_s = std::min(dec_s, std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count());
    }
    if (memcmp(out.data(), in.data(), in.size())) throw std::runtime_error("BIT MISMATCH");
}
static V parse(const char* s) {
    V v; std::string t(s); size_t c = t.find(':'); v.name = t.substr(0, c);
    if (v.name == "l11") { v.kind = 0; } else if (v.name == "slot") { v.kind = 1; } else if (v.name[0] == 'x') { v.kind = 3; v.back = 1; v.ins = 16; } else if (v.name[0] == 'y') { v.kind = 4; v.back = 1; v.ins = 16; } else if (v.name[0] == 'w') { v.kind = 6; v.ins = 8; v.skip = 8; } else if (v.name[0] == 'z') { v.kind = 5; v.base = 1; v.pi = 1; v.ins = 1; }
    if (c == std::string::npos) return v;
    std::string r = t.substr(c + 1); size_t p = 0;
    while (p < r.size()) { size_t q = r.find(',', p); if (q == std::string::npos) q = r.size(); std::string kv = r.substr(p, q - p); size_t e = kv.find('=');
        std::string k = kv.substr(0, e); int x = atoi(kv.c_str() + e + 1);
        if (k == "hb") v.hb = x; else if (k == "w") v.w = x; else if (k == "pi") v.pi = x; else if (k == "base") v.base = x; else if (k == "acc") v.acc = x; else if (k == "mx") v.mx = x; else if (k == "key") v.key = x; else if (k == "ins") v.ins = x; else if (k == "back") v.back = x;
        else if (k == "lazy") v.lazy = x; else if (k == "skip") v.skip = x; else if (k == "mm") v.mm = x; else throw std::runtime_error("bad knob " + k);
        p = q + 1; }
    return v;
}

// --selftest spec...: boundary sizes x patterns (zeros, periods 1..65 around the 64-byte minimum distance, random, copied
// noise, block transitions) through every given variant; each archive is decoded and compared, and encoded twice (deterministic)
static int selftest(int argc, char** argv) {
    std::vector<V> vs; for (int i = 2; i < argc; i++) vs.push_back(parse(argv[i]));
    uint64_t rng = 20261028; auto rnd = [&]() { rng ^= rng << 13; rng ^= rng >> 7; rng ^= rng << 17; return (uint8_t)rng; };
    size_t cases = 0;
    const unsigned sizes[] = {1, 15, 16, 17, 63, 64, 65, 79, 80, 81, 127, 128, 160, 255, 256, 4096, 65536, 1048575, 1048576, 1048577, 2621440};
    for (unsigned n : sizes) for (unsigned mode = 0; mode < 9; mode++) {
        Bytes in(n);
        for (unsigned i = 0; i < n; i++) in[i] = mode == 0 ? 0 : mode == 1 ? (uint8_t)(i % 63) : mode == 2 ? (uint8_t)(i % 64) : mode == 3 ? (uint8_t)(i % 65)
                                     : mode == 4 ? rnd() : mode == 6 ? (uint8_t)(i % 7) : mode == 7 ? (uint8_t)(i % 2) : (uint8_t)(rnd() & 3);
        if (mode == 5) { for (unsigned i = 0; i < n; i++) in[i] = rnd(); for (unsigned i = 128; i < n; i++) if (i % 97) in[i] = in[i - 128]; }
        for (const V& v : vs) { Bytes a = archive(in, v); double ds; judge(in, a, ds); if (a != archive(in, v)) throw std::runtime_error("nondeterministic"); cases++; }
    }
    for (int k = 0; k < 300; k++) {   // short random alphabets, random sizes up to 3 blocks
        const size_t n = 1 + (((size_t)rnd() << 16 | (size_t)rnd() << 8 | rnd()) % (3u << 20)); Bytes in(n); const unsigned al = 1 + rnd() % 6;
        for (auto& c : in) c = (uint8_t)(rnd() % al);
        for (const V& v : vs) { Bytes a = archive(in, v); double ds; judge(in, a, ds); cases++; }
    }
    printf("selftest PASS cases=%zu variants=%zu (decoded by src/ax_fast.h, compared byte for byte, determinism checked)\n", cases, vs.size());
    return 0;
}

// --stats FILE spec...: structure of the token stream each variant produces (what the decoder executes): tokens per MiB,
// literal share, tokens with a literal extension (ll >= 63), escaped matches (mc == 63, > 64 bytes: the slow copy path)
// by count and by bytes, match length p50 / p90 / p99, distance share < 256 / < 4096. One encode per variant, untimed.
static int stats(int argc, char** argv) {
    FILE* f = fopen(argv[2], "rb"); if (!f) { perror(argv[2]); return 1; }
    fseek(f, 0, SEEK_END); Bytes in((size_t)ftell(f)); fseek(f, 0, SEEK_SET); if (fread(in.data(), 1, in.size(), f) != in.size()) return 1; fclose(f);
    printf("variant\tratio_%%\ttok_per_MiB\tlit_%%\tll63_%%tok\tmc63_%%tok\tmc63_%%bytes\tml_p50\tml_p90\tml_p99\td<256_%%\td<4096_%%\n");
    for (int i = 3; i < argc; i++) {
        const V v = parse(argv[i]); Bytes a = archive(in, v); double ds; judge(in, a, ds);
        axf_arc_t arc; axf_open(a.data(), a.size(), &arc);
        const uint8_t* data = a.data() + 68 + 8 * (arc.nb + 1); uint32_t mm; memcpy(&mm, a.data() + 40, 4);
        uint64_t nt = 0, nll = 0, nmc = 0, bmc = 0, lit = 0, d256 = 0, d4k = 0; std::vector<uint32_t> ml;
        for (uint32_t b = 0; b < arc.nb; b++) {
            const uint8_t* p = data + axf_boff(&arc, b); uint32_t ntok, next, tail; memcpy(&ntok, p, 4); memcpy(&next, p + 4, 4); memcpy(&tail, p + 8, 4);
            const uint8_t* T = p + AXF_BHDR; const uint8_t* E = T + 4ull * ntok; size_t ep = 0;
            auto leb = [&]() { uint32_t r = 0, x; do { x = E[ep++]; r += x; } while (x == 255); return r; };
            for (uint32_t k = 0; k < ntok; k++) {
                uint32_t t; memcpy(&t, T + 4ull * k, 4); uint32_t ll = t & 63, mc = (t >> 6) & 63, d = (t >> 12) + AXF_MIN_DIST;
                if (ll == 63) { ll += leb(); nll++; }
                uint32_t l; if (mc == 63) { l = AXF_COPY + 1 + leb(); nmc++; bmc += l; } else l = mc + mm;
                lit += ll; ml.push_back(l); d256 += d < 256; d4k += d < 4096;
            }
            lit += tail; nt += ntok;
        }
        std::sort(ml.begin(), ml.end()); auto q = [&](double x) { return ml.empty() ? 0u : ml[(size_t)(x * (ml.size() - 1))]; };
        printf("%s\t%.4f\t%.0f\t%.2f\t%.3f\t%.3f\t%.2f\t%u\t%u\t%u\t%.2f\t%.2f\n", argv[i], 100.0 * a.size() / in.size(), nt * 1048576.0 / in.size(),
               100.0 * lit / in.size(), 100.0 * nll / (nt ? nt : 1), 100.0 * nmc / (nt ? nt : 1), 100.0 * bmc / in.size(), q(0.5), q(0.9), q(0.99),
               100.0 * d256 / (nt ? nt : 1), 100.0 * d4k / (nt ? nt : 1));
        fflush(stdout);
    }
    return 0;
}

// --write FILE OUTDIR spec...: one archive per variant, OUTDIR/<name>.aet (a normal FAST archive: aceapex d / t read it)
static int writeall(int argc, char** argv) {
    FILE* f = fopen(argv[2], "rb"); if (!f) { perror(argv[2]); return 1; }
    fseek(f, 0, SEEK_END); Bytes in((size_t)ftell(f)); fseek(f, 0, SEEK_SET); if (fread(in.data(), 1, in.size(), f) != in.size()) return 1; fclose(f);
    for (int i = 4; i < argc; i++) {
        const V v = parse(argv[i]); Bytes a = archive(in, v); double ds; judge(in, a, ds);
        const std::string o = std::string(argv[3]) + "/" + v.name + ".aet"; FILE* g = fopen(o.c_str(), "wb");
        if (!g || fwrite(a.data(), 1, a.size(), g) != a.size() || fclose(g)) { perror(o.c_str()); return 1; }
        printf("%s\t%zu\n", o.c_str(), a.size());
    }
    return 0;
}

// --dec ARCHIVE R: decode a FAST archive R times on one thread (no encode in the process: for perf stat); prints median MB/s
static int deconly(int argc, char** argv) {
    FILE* f = fopen(argv[2], "rb"); if (!f) { perror(argv[2]); return 1; }
    fseek(f, 0, SEEK_END); Bytes a((size_t)ftell(f)); fseek(f, 0, SEEK_SET); if (fread(a.data(), 1, a.size(), f) != a.size()) return 1; fclose(f);
    axf_arc_t arc; if (axf_open(a.data(), a.size(), &arc)) { fprintf(stderr, "not a FAST archive\n"); return 1; }
    const int R = argc > 3 ? atoi(argv[3]) : 20; Bytes out(arc.orig); std::vector<double> t;
    for (int r = 0; r < R; r++) {
        const auto t0 = std::chrono::steady_clock::now();
        for (uint32_t b = 0; b < arc.nb; b++) if (axf_dec_block_of(&arc, b, out.data() + (size_t)b * arc.bs) != axf_block_len(&arc, b)) { fprintf(stderr, "decode failed\n"); return 1; }
        t.push_back(std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count());
    }
    if (XXH3_64bits(out.data(), out.size()) != axf_rd64(a.data() + 28)) { fprintf(stderr, "XXH3 mismatch\n"); return 1; }
    std::sort(t.begin(), t.end()); printf("%s\t%.0f MB/s median of %d, XXH3 ok\n", argv[2], arc.orig / t[R / 2] / 1e6, R);
    return 0;
}
int main(int argc, char** argv) {
    if (argc >= 3 && !strcmp(argv[1], "--dec")) return deconly(argc, argv);
    if (argc >= 4 && !strcmp(argv[1], "--write")) return writeall(argc, argv);
    if (argc >= 3 && !strcmp(argv[1], "--stats")) return stats(argc, argv);
    if (argc >= 2 && !strcmp(argv[1], "--selftest")) return selftest(argc, argv);
    if (argc < 3) { fprintf(stderr, "usage: enc2 FILE reps spec...\n"); return 1; }
    FILE* f = fopen(argv[1], "rb"); if (!f) { perror(argv[1]); return 1; }
    fseek(f, 0, SEEK_END); Bytes in((size_t)ftell(f)); fseek(f, 0, SEEK_SET); if (fread(in.data(), 1, in.size(), f) != in.size()) return 1; fclose(f);
    const int R = atoi(argv[2]);
    printf("variant\tratio_%%\tenc_MB_s_median\tenc_MB_s_best\tblock_only_MB_s_median\tdec_MB_s_median\ttokens_per_MiB\n");
    const int NV = argc - 3; std::vector<V> vs; for (int i = 0; i < NV; i++) vs.push_back(parse(argv[3 + i]));
    std::vector<std::vector<double>> te(NV), td(NV), tb(NV); std::vector<size_t> sz(NV); std::vector<uint64_t> nt(NV);
    for (int r = 0; r < R; r++) for (int i = 0; i < NV; i++) {
        const auto t0 = std::chrono::steady_clock::now(); Bytes a = archive(in, vs[i]);
        te[i].push_back(std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count()); tb[i].push_back(g_blk);
        double ds; judge(in, a, ds); td[i].push_back(ds); sz[i] = a.size();
        axf_arc_t arc; axf_open(a.data(), a.size(), &arc); uint64_t ntok = 0;
        for (uint32_t b = 0; b < arc.nb; b++) { uint32_t t; memcpy(&t, a.data() + 68 + 8 * (arc.nb + 1) + axf_boff(&arc, b), 4); ntok += t; }
        nt[i] = ntok;
    }
    auto med = [](std::vector<double> v) { std::sort(v.begin(), v.end()); return v[v.size() / 2]; };
    for (int i = 0; i < NV; i++)
        printf("%s\t%.4f\t%.1f\t%.1f\t%.1f\t%.0f\t%.0f\n", argv[3 + i], 100.0 * sz[i] / in.size(), in.size() / med(te[i]) / 1e6,
               in.size() / *std::min_element(te[i].begin(), te[i].end()) / 1e6, in.size() / med(tb[i]) / 1e6, in.size() / med(td[i]) / 1e6, nt[i] * 1048576.0 / in.size());
    return 0;
}
