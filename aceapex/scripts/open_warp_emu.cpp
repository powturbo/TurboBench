// open_warp_emu.cpp - the GPU decoder of the open DNA pack (ADR-019: k_open_cse, k_open_bases,
// k_open_exc in aceapex_gpu.cu, host framing axo_parse) on the CPU. The per-thread steps of
// src/ax_open_warp.h run for the AXO_NT threads of a block in turn, with the block scan done
// by a loop at the same points as in the kernels; pieces are decoded with axo_piece_decode (the
// rANS pieces go through k_rans on the device, judged by head_rans_warp_emu). The result is
// compared with the reference axo_dna_decode (src/ax_lit_open.h):
//   1. round-trips of axo_dna_encode on generated DNA (case runs, N runs, IUPAC codes);
//   2. every mode 2 chunk of the given archives;
//   3. crafted streams (zero runs, 6-byte and 33-bit LEB128, unterminated streams, positions
//      past the chunk, gap counts off by one) and random bit flips / truncations: both
//      decoders must agree on the verdict, and on the bytes when they accept.
// Build: g++ -std=c++17 -O2 -Isrc -o open_warp_emu scripts/open_warp_emu.cpp
// Usage: open_warp_emu [archive.aet ...]   Output: claim_id <TAB> verdict <TAB> measured
#include "ax_open_warp.h"
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <vector>
#include <random>
#include <algorithm>

// one parse round = AXO_NT threads; exclusive scan of (1 << 44 | v) over the round, as in the kernels
static void emu_round(const uint8_t* b, uint32_t n, uint32_t base, bool* term, uint32_t* v, uint64_t* ex, uint64_t& total, bool* bad) {
    uint64_t acc = 0;
    for (uint32_t x = 0; x < AXO_NT; x++) {
        term[x] = axl_term(base + x, b, n); v[x] = axl_value(base + x, b, term[x], bad[x]);
        ex[x] = acc; acc += term[x] ? (AXO_KEY_J | v[x]) : 0;
    }
    total = acc;
}
static uint32_t emu_cse(const uint8_t* b, uint32_t n, uint32_t raw, uint32_t* ends, bool& badw) {
    static bool bad[AXO_NT]; static bool term[AXO_NT]; static uint32_t v[AXO_NT]; static uint64_t ex[AXO_NT];
    for (uint32_t x = 0; x < AXO_NT; x++) bad[x] = false;
    if (axl_tail_bad(b, n)) bad[0] = true;
    uint64_t carry = 0; uint32_t jb = 0;
    for (uint32_t base = 0; base < n; base += AXO_NT) {
        uint64_t total; emu_round(b, n, base, term, v, ex, total, bad);
        for (uint32_t x = 0; x < AXO_NT; x++)
            axl_cse_end(term[x], jb + (uint32_t)(ex[x] >> 44), v[x], carry + (ex[x] & (AXO_KEY_J - 1)) + v[x], raw, ends, bad[x]);
        jb += (uint32_t)(total >> 44); carry += total & (AXO_KEY_J - 1);
    }
    if (carry != raw) bad[0] = true;
    bool any = false; for (uint32_t x = 0; x < AXO_NT; x++) any |= bad[x];
    badw |= any;
    return any ? 0 : jb;                                             // the kernel stores 0 runs for a bad chunk
}
static void emu_exc(const uint8_t* b, uint32_t n, uint32_t nexc, uint32_t raw, const uint8_t* val, uint8_t* dst, bool& badw) {
    if (nexc == 0) return;
    static bool bad[AXO_NT]; static bool term[AXO_NT]; static uint32_t v[AXO_NT]; static uint64_t ex[AXO_NT];
    for (uint32_t x = 0; x < AXO_NT; x++) bad[x] = false;
    if (axl_tail_bad(b, n)) bad[0] = true;
    uint64_t carry = 0; uint32_t jb = 0;
    for (uint32_t base = 0; base < n; base += AXO_NT) {
        uint64_t total; emu_round(b, n, base, term, v, ex, total, bad);
        for (uint32_t x = 0; x < AXO_NT; x++)
            axl_exc(term[x], jb + (uint32_t)(ex[x] >> 44), v[x], carry + (ex[x] & (AXO_KEY_J - 1)) + v[x], nexc, raw, val, dst, bad[x]);
        jb += (uint32_t)(total >> 44); carry += total & (AXO_KEY_J - 1);
    }
    if (jb != nexc) bad[0] = true;
    for (uint32_t x = 0; x < AXO_NT; x++) badw |= bad[x];
}
// the device path for one mode 2 payload: host framing, pieces, seq expand, cse, exceptions
static int emu_decode(const uint8_t* s, size_t sz, uint8_t* dst, uint32_t raw) {
    AxoParts P;
    if (axo_parse(s, sz, raw, &P)) return -1;
    std::vector<uint8_t> part[4];
    for (int k = 0; k < 4; k++) {
        part[k].assign(P.n[k] + 1, 0);
        if (!P.n[k]) continue;
        if (P.mode[k] == 0) memcpy(part[k].data(), s + P.off[k], P.n[k]);
        else if (axr_decode(s + P.off[k], P.h[k], part[k].data(), P.n[k])) return -1;
    }
    bool bad = false;
    std::vector<uint32_t> ends(P.ncse + 1);
    const uint32_t R = emu_cse(part[1].data(), P.ncse, raw, ends.data(), bad);   // k_open_cse
    for (uint32_t g = 0; 16 * g < raw; g++) axl_bases16(g, part[0].data(), ends.data(), R, raw, dst);   // k_open_bases
    if (bad) return -1;
    emu_exc(part[2].data(), P.ngap, P.nexc, raw, part[3].data(), dst, bad);
    return bad ? -1 : 0;
}

// the default device path since 01.10 (AX_OPEN_EXC): k_open_cg (case runs, then exception positions), k_open_bases_x
// (warps of 32 threads x 16 positions, lanes 0-3 bracket the runs / exceptions of the warp's 512 positions)
// S 1: k_open_bases_s (AX_OPEN_SHB) - per 4096-position block the run-end / exception slices copied (cap 1024) and
// read through pointers shifted by the slice start
static uint64_t g_shb = 0;
static int emu_decode_v(const uint8_t* s, size_t sz, uint8_t* dst, uint32_t raw, int X = 1, int S = 0) {
    AxoParts P;
    if (axo_parse(s, sz, raw, &P)) return -1;
    std::vector<uint8_t> part[4];
    for (int k = 0; k < 4; k++) {
        part[k].assign(P.n[k] + 4, 0);
        if (!P.n[k]) continue;
        if (P.mode[k] == 0) memcpy(part[k].data(), s + P.off[k], P.n[k]);
        else if (axr_decode(s + P.off[k], P.h[k], part[k].data(), P.n[k])) return -1;
    }
    bool bad = false;
    std::vector<uint32_t> ends(P.ncse + 1), epos(P.nexc + 1, 0);
    const uint32_t R = emu_cse(part[1].data(), P.ncse, raw, ends.data(), bad);           // k_open_cse / first half of k_open_cg
    if (bad) return -1;
    if (X && P.nexc) {                                                                    // second half of k_open_cg
        static bool bb[AXO_NT]; static bool term[AXO_NT]; static uint32_t v[AXO_NT]; static uint64_t ex[AXO_NT];
        for (uint32_t x = 0; x < AXO_NT; x++) bb[x] = false;
        const uint8_t* b = part[2].data(); const uint32_t n = P.ngap;
        if (axl_tail_bad(b, n)) bb[0] = true;
        uint64_t carry = 0; uint32_t jb = 0;
        for (uint32_t base = 0; base < n; base += AXO_NT) {
            uint64_t total; emu_round(b, n, base, term, v, ex, total, bb);
            for (uint32_t x = 0; x < AXO_NT; x++)
                axl_exc_pos(term[x], jb + (uint32_t)(ex[x] >> 44), v[x], carry + (ex[x] & (AXO_KEY_J - 1)) + v[x], P.nexc, raw, epos.data(), bb[x]);
            jb += (uint32_t)(total >> 44); carry += total & (AXO_KEY_J - 1);
        }
        if (jb != P.nexc) bb[0] = true;
        for (uint32_t x = 0; x < AXO_NT; x++) bad |= bb[x];
        if (bad) return -1;
    }
    const uint32_t ne = X ? P.nexc : 0;
    if (S) {
        for (uint32_t first = 0; first < raw; first += 4096) {                            // one block of 256 threads
            const uint32_t last = std::min(first + 4095u, raw - 1);
            const uint32_t jlo = axl_run_of(ends.data(), R, first), jhi = axl_run_of(ends.data(), R, last);
            const uint32_t elo = axl_exc_in(epos.data(), 0, ne, first), ehi = axl_exc_in(epos.data(), 0, ne, last + 1);
            const uint32_t nj = std::min(jhi, R - 1) - jlo + 1, nx = ehi - elo;
            std::vector<uint32_t> se(ends.begin() + jlo, ends.begin() + jlo + (nj <= 1024 && nx <= 1024 ? nj : 0)),
                                  sp(epos.begin() + elo, epos.begin() + elo + (nj <= 1024 && nx <= 1024 ? nx : 0));
            const bool sh = nj <= 1024 && nx <= 1024; if (sh) g_shb++;
            se.push_back(0); sp.push_back(0);                                             // never empty: shifted pointers of a real array
            const uint32_t* E = sh ? se.data() - jlo : ends.data(); const uint32_t* Pp = sh ? sp.data() - elo : epos.data();
            for (uint32_t g = first / 16; g < first / 16 + 256 && 16 * g < raw; g++)
                axl_bases16_v(g, part[0].data(), E, R, raw, dst, axl_run_in(E, jlo, jhi, 16 * g), Pp, ehi, part[3].data(), axl_exc_in(Pp, elo, ehi, 16 * g));
        }
        return bad ? -1 : 0;
    }
    for (uint32_t gw = 0; 16 * gw < raw; gw += 32) {                                      // one warp
        const uint32_t first = 16 * gw, last = std::min(first + 511u, raw - 1);
        const uint32_t jlo = axl_run_of(ends.data(), R, first), jhi = axl_run_of(ends.data(), R, last);
        const uint32_t elo = X ? axl_exc_in(epos.data(), 0, ne, first) : 0, ehi = X ? axl_exc_in(epos.data(), 0, ne, last + 1) : 0;
        for (uint32_t g = gw; g < gw + 32 && 16 * g < raw; g++) {
            const uint32_t i0 = 16 * g, j = axl_run_in(ends.data(), jlo, jhi, i0);
            axl_bases16_v(g, part[0].data(), ends.data(), R, raw, dst, j, X ? epos.data() : nullptr, ehi, part[3].data(), X ? axl_exc_in(epos.data(), elo, ehi, i0) : 0);
        }
    }
    if (!X) emu_exc(part[2].data(), P.ngap, P.nexc, raw, part[3].data(), dst, bad);
    return bad ? -1 : 0;
}

static uint64_t g_ok = 0, g_rej = 0, g_bytes = 0, g_var = 0;
static int cmp_chunk(const uint8_t* c, size_t sz, uint32_t raw) {
    std::vector<uint8_t> a(raw + 1, 0xAA), e(raw + 1, 0x55), v1(raw + 1, 0x22);
    int ra = axo_dna_decode(c, sz, a.data(), raw), re = emu_decode(c, sz, e.data(), raw);
    std::vector<uint8_t> v2(raw + 1, 0x33);
    int r1 = emu_decode_v(c, sz, v1.data(), raw), r2 = emu_decode_v(c, sz, v2.data(), raw, 1, 1); g_var++;
    if (ra != re || ra != r1 || ra != r2) return 1;
    if (ra == 0) { if (memcmp(a.data(), e.data(), raw) || memcmp(a.data(), v1.data(), raw) || memcmp(a.data(), v2.data(), raw)) return 1; g_ok++; g_bytes += raw; } else g_rej++;
    return 0;
}
static uint64_t rd64(const uint8_t* p) { uint64_t v; memcpy(&v, p, 8); return v; }

// rebuild a payload with one stream replaced (stored as a raw piece)
static std::vector<uint8_t> with_stream(const uint8_t* s, size_t sz, uint32_t raw, int k, const std::vector<uint8_t>& ns, int dnexc = 0) {
    AxoParts P; std::vector<uint8_t> out;
    if (axo_parse(s, sz, raw, &P)) return out;
    std::vector<uint8_t> part[4];
    for (int q = 0; q < 4; q++) { part[q].resize(P.n[q]); if (P.n[q]) axo_piece_decode(s + P.off[q] - 1, P.h[q] + 1, part[q].data(), P.n[q]); }
    part[k] = ns;
    uint32_t nexc = P.nexc + dnexc; if (dnexc > 0) part[3].push_back('N'); if (dnexc < 0 && !part[3].empty()) part[3].pop_back();
    uint32_t w[7] = {nexc, (uint32_t)part[1].size(), (uint32_t)part[2].size(), 0, 0, 0, 0};
    out.resize(AXO_HDR);
    for (int q = 0; q < 4; q++) { if (part[q].empty()) continue; w[3 + q] = (uint32_t)part[q].size() + 1; out.push_back(0); out.insert(out.end(), part[q].begin(), part[q].end()); }
    memcpy(out.data(), w, AXO_HDR);
    return out;
}
static std::vector<uint8_t> leb_all(const std::vector<uint32_t>& v) { std::vector<uint8_t> o; uint8_t b[5]; for (uint32_t x : v) { size_t n = axo_put_leb(b, x); o.insert(o.end(), b, b + n); } return o; }
static std::vector<uint32_t> leb_read(const std::vector<uint8_t>& b) { std::vector<uint32_t> v; size_t i = 0; uint32_t x; while (i < b.size() && !axo_leb(b.data(), b.size(), &i, &x)) v.push_back(x); return v; }
static std::vector<uint8_t> stream_of(const uint8_t* s, size_t sz, uint32_t raw, int k) {
    AxoParts P; std::vector<uint8_t> o; if (axo_parse(s, sz, raw, &P)) return o;
    o.resize(P.n[k]); if (P.n[k]) axo_piece_decode(s + P.off[k] - 1, P.h[k] + 1, o.data(), P.n[k]); return o;
}

int main(int argc, char** argv) {
    uint64_t bad = 0, rt = 0, fx = 0, mut = 0, crafted = 0;
    std::mt19937_64 R(2019);
    const uint32_t sizes[] = {1, 3, 31, 33, 100, 4095, 4096, 4097, 20000, 65536};
    for (uint32_t n : sizes) for (int dist = 0; dist < 4; dist++) {
        std::vector<uint8_t> in(n);
        for (uint32_t i = 0; i < n;) {                                  // runs of case, N, IUPAC
            uint32_t k = 1 + (uint32_t)(R() % (dist == 0 ? 3 : dist == 1 ? 40 : 2000));
            uint64_t kind = R() % 100;
            for (uint32_t q = 0; q < k && i < n; q++, i++) {
                uint8_t b = "ACGT"[R() & 3];
                if (dist == 3) b = "ACGTNRYacgtn"[R() % 12];
                else if (kind < 20) b |= 0x20; else if (kind < 26) b = 'N'; else if (kind < 28) b = "RYKMSWn"[R() % 7];
                in[i] = b;
            }
        }
        std::vector<uint8_t> enc(axo_dna_bound(n));
        size_t sz = axo_dna_encode(in.data(), n, enc.data());
        std::vector<uint8_t> e(n + 1);
        if (!sz || emu_decode(enc.data(), sz, e.data(), n) || memcmp(e.data(), in.data(), n)) bad++;
        rt++;
        bad += cmp_chunk(enc.data(), sz, n);
        // crafted streams
        std::vector<uint32_t> runs = leb_read(stream_of(enc.data(), sz, n, 1));
        std::vector<uint32_t> gaps = leb_read(stream_of(enc.data(), sz, n, 2));
        std::vector<std::vector<uint8_t>> cse_v, gap_v;
        { auto r2 = runs; r2.push_back(0); cse_v.push_back(leb_all(r2)); }                          // zero run inside
        { auto r2 = runs; r2.back() += 1; cse_v.push_back(leb_all(r2)); }                           // sum > raw
        if (runs.back() > 1) { auto r2 = runs; r2.back() -= 1; cse_v.push_back(leb_all(r2)); }      // sum < raw
        { auto b = leb_all(runs); b.back() |= 0x80; cse_v.push_back(b); }                           // unterminated
        { auto b = leb_all(runs); b.insert(b.begin(), {0x80, 0x80, 0x80, 0x80, 0x80}); b[5] &= 0x7F; cse_v.push_back(b); }   // 6-byte LEB
        { std::vector<uint8_t> b = {0x80, 0x80, 0x80, 0x80, 0x10}; auto t = leb_all(runs); b.insert(b.end(), t.begin() + 1, t.end()); cse_v.push_back(b); } // 33 bits
        { std::vector<uint8_t> b = {0x80, 0x00}; auto t = leb_all(runs); b.insert(b.end(), t.begin() + 1, t.end()); cse_v.push_back(b); }  // padded LEB of the first run: valid when it was 0
        for (auto& c : cse_v) { auto m = with_stream(enc.data(), sz, n, 1, c); if (!m.empty()) { bad += cmp_chunk(m.data(), m.size(), n); crafted++; } }
        if (!gaps.empty()) {
            { auto g = gaps; g.push_back(1); gap_v.push_back(leb_all(g)); }                          // one gap too many
            { auto g = gaps; g.back() += n; gap_v.push_back(leb_all(g)); }                           // position past raw
            if (gaps.size() > 1) { auto g = gaps; g[1] = 0; gap_v.push_back(leb_all(g)); }            // zero gap
            { auto b = leb_all(gaps); b.back() |= 0x80; gap_v.push_back(b); }                        // unterminated
            for (auto& g : gap_v) { auto m = with_stream(enc.data(), sz, n, 2, g); if (!m.empty()) { bad += cmp_chunk(m.data(), m.size(), n); crafted++; } }
            { auto m = with_stream(enc.data(), sz, n, 2, leb_all(gaps), -1); if (!m.empty()) { bad += cmp_chunk(m.data(), m.size(), n); crafted++; } }  // nexc - 1
            { auto g = gaps; g.push_back(1); auto m = with_stream(enc.data(), sz, n, 2, leb_all(g), +1); if (!m.empty()) { bad += cmp_chunk(m.data(), m.size(), n); crafted++; } } // valid extra exception
        }
        for (int k = 0; k < 60; k++) {
            std::vector<uint8_t> m(enc.begin(), enc.begin() + sz); size_t msz = sz;
            if (k % 5 == 4) msz = (size_t)(R() % sz);
            else { size_t p = (size_t)(R() % sz); m[p] ^= (uint8_t)(1u << (R() % 8)); }
            bad += cmp_chunk(m.data(), msz, n); mut++;
        }
    }
    for (int ai = 1; ai < argc; ai++) {                                   // mode 2 chunks of real archives
        FILE* f = fopen(argv[ai], "rb"); if (!f) { bad++; continue; }
        std::vector<uint8_t> a; { uint8_t buf[1 << 16]; size_t r; while ((r = fread(buf, 1, sizeof buf, f)) > 0) a.insert(a.end(), buf, buf + r); } fclose(f);
        if (a.size() < 68 || memcmp(a.data(), "ACEPX2\0\0", 8) || axo_rd32(&a[8]) != 2) continue;   // header version 2 only (the fast profile has no entropy chunks)
        uint32_t nb = axo_rd32(&a[24]); uint64_t zl = rd64(&a[36]);
        if (68 + 64ull * nb + zl > a.size() || zl < 16) continue;
        const uint8_t* z = a.data() + 68 + 64ull * nb; uint64_t h = rd64(z);
        if (!(h >> 61 & 1) || !(h >> 60 & 1)) continue;
        uint64_t S = h & ((1ull << 60) - 1), CH = rd64(z + 8), NW = (S + CH - 1) / CH; size_t pos = 16 + 8 * NW;
        for (uint64_t t = 0; t < NW; t++) {
            uint64_t raw = (t + 1) * CH <= S ? CH : S - t * CH, csz = rd64(z + 16 + 8 * t);
            if (pos + csz > zl) { bad++; break; }
            const uint8_t* c = z + pos; pos += csz;
            if (csz && c[0] == 2) {
                bad += cmp_chunk(c + 1, csz - 1, (uint32_t)raw); fx++;
                for (int k = 0; k < 6; k++) { std::vector<uint8_t> m(c + 1, c + csz); m[(size_t)(R() % m.size())] ^= (uint8_t)(1u << (R() % 8)); bad += cmp_chunk(m.data(), m.size(), (uint32_t)raw); mut++; }
            }
        }
    }
    printf("head_open_warp_emu\t%s\tGPU open-pack steps (ax_open_warp.h, block rounds of 256) == axo_dna_decode, old path (cse, bases, exc), the default k_open_cg + k_open_bases_x and k_open_bases_s (AX_OPEN_SHB, %llu decodes each, %llu blocks from shared): %llu round-trips, %llu archive chunks, %llu crafted streams, %llu mutations (%llu accepted, %llu rejected by both), %llu bytes, %llu mismatches\n",
           (bad == 0 && rt > 0 && fx > 0 && crafted > 0 && g_shb > 0) ? "pass" : "fail", (unsigned long long)g_var, (unsigned long long)g_shb,
           (unsigned long long)rt, (unsigned long long)fx, (unsigned long long)crafted, (unsigned long long)mut,
           (unsigned long long)g_ok, (unsigned long long)g_rej, (unsigned long long)g_bytes, (unsigned long long)bad);
    return 0;
}
