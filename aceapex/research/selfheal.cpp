// selfheal.cpp - research I2 (CPU; no format change): can a decoder repair a bit flip in an ACEPX2 archive by itself?
// The archive is decoded unit by unit - header, block table, chunk tables, every literal chunk and token chunk, every
// block (its tokens must consume exactly its slices and fill exactly its size). A unit that fails names the archive
// bytes it depends on; every bit there is flipped in turn: a probe re-decodes only what the bit touches (a chunk body:
// that chunk and the blocks that read it; a table, a header or a chunk-table entry: the whole archive), and a probe that
// passes every check goes to the oracle - XXH3 of the whole output against the header. A unit that fails nothing but
// the oracle (a flip in bytes stored raw) has no locator: not found.
// Outcome per corrupt copy: harmless (the oracle already agrees), found (the repaired archive == the original bytes),
// false (the oracle agrees with another archive), not found; probes and CPU time.
// Step 2 (02.10): with a sidecar of block hashes (selfheal --sidecar <archive> <out.bh>: XXH3_64 of every block of the
// intact archive's output, 8 B per block - not in the format) a block must also match its hash: a flip in bytes stored
// raw names its block, and a probe is judged on the blocks it touches, not on the whole output.
// Usage: selfheal <archive.aet> <copies> <flips per copy> [seed] [sidecar.bh]  |  selfheal --sidecar <archive.aet> <out.bh>
// Build: g++ -std=c++17 -O3 -march=native -Isrc research/selfheal.cpp -lzstd -lpthread
#include "../src/aceapex_api.cpp"
#include "ax_open_warp.h"
#include <random>
#include <thread>
#include <mutex>

namespace sh {
static const std::vector<uint64_t>* g_bh = nullptr;            // block hashes (research sidecar): XXH3_64 of every decoded block
static double now() { return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
static uint64_t rd64(const uint8_t* p) { uint64_t v; memcpy(&v, p, 8); return v; }
struct Chunk { uint64_t off, raw, lo, hi, ent; };            // stream offset, raw size, file range of the body, file offset of its table entry
struct Tok { uint64_t S = 0, CH = 0, hlo = 0, hhi = 0; std::vector<Chunk> ch; std::vector<uint64_t> cs; };
struct Unit { int kind; uint64_t lo, hi; };                   // file bytes [lo, hi); kind: 0 layout (full re-decode), 1 literal chunk, 2 token chunk, 3 block
struct Dec {
    const uint8_t* a = nullptr; size_t n = 0; bool parsed = false;
    uint64_t orig = 0, bs = 0, nb = 0, xxh = 0, z[4] = {0, 0, 0, 0}, zo[4] = {0, 0, 0, 0};
    std::vector<uint8_t> tab;                                  // block table bytes
    std::vector<Chunk> lch; uint64_t lsz = 0, lcsz = 0; bool ltag = false; uint64_t lhlo = 0, lhhi = 0;
    Tok tk[4];
    std::vector<uint8_t> lit, tok[4], out; std::vector<char> lok, tok_ok[4], bok;
    uint64_t bo(uint64_t b, int f) const { return rd64(&tab[64 * b + 8 * f]); }
    bool parse() {
        parsed = false; if (n < 68 || memcmp(a, "ACEPX2\0\0", 8)) return false;
        uint32_t v32; memcpy(&v32, a + 8, 4); if (v32 != 2) return false;
        orig = rd64(a + 12); uint32_t b32, n32; memcpy(&b32, a + 20, 4); memcpy(&n32, a + 24, 4); bs = b32; nb = n32; xxh = rd64(a + 28);
        for (int s = 0; s < 4; s++) z[s] = rd64(a + 36 + 8 * s);
        if (!bs || !nb || nb * bs < orig || (nb - 1) * bs >= orig || 68 + 64 * nb > n) return false;
        tab.assign(a + 68, a + 68 + 64 * nb); uint64_t p = 68 + 64 * nb;
        for (int s = 0; s < 4; s++) { zo[s] = p; if (z[s] > n - p) return false; p += z[s]; }
        if (p != n) return false;
        // literal stream: tagged chunked layout
        { const uint8_t* q = a + zo[0]; if (z[0] < 16) return false; const uint64_t h = rd64(q); if (!((h >> 62) & 1) || !((h >> 61) & 1) || (h >> 63)) return false;
          ltag = (h >> 60) & 1; lsz = h & ~(7ull << 60); lcsz = rd64(q + 8); if (!lcsz) return false; const uint64_t NW = (lsz + lcsz - 1) / lcsz;
          if (NW > (z[0] - 16) / 8) return false; lhlo = zo[0]; lhhi = zo[0] + 16 + 8 * NW; uint64_t f = lhhi; lch.resize(NW);
          for (uint64_t t = 0; t < NW; t++) { const uint64_t c = rd64(q + 16 + 8 * t); if (c > zo[0] + z[0] - f) return false;
              lch[t] = {t * lcsz, std::min(lcsz, lsz - t * lcsz), f, f + c, zo[0] + 16 + 8 * t}; f += c; }
          if (f != zo[0] + z[0]) return false; }
        for (int s = 1; s < 4; s++) { Tok& t = tk[s]; t = Tok(); if (z[s] < 8) continue; const uint8_t* q = a + zo[s]; const uint64_t w = rd64(q);
            if (w >> 63) return false; t.S = w & ((1ull << 48) - 1); t.CH = ((w >> 48) & 0x7fff) * 4096; if (!t.CH) return false;
            const uint64_t nc = (t.S + t.CH - 1) / t.CH; if (nc > (z[s] - 8) / 8) return false; t.hlo = zo[s]; t.hhi = zo[s] + 8 + 8 * nc; uint64_t f = t.hhi;
            t.ch.resize(nc); t.cs.resize(nc);
            for (uint64_t i = 0; i < nc; i++) { const uint64_t e = rd64(q + 8 + 8 * i); if (ax_ce_bad(e)) return false; const uint64_t raw = std::min(t.CH, t.S - i * t.CH);
                const uint64_t c = ax_ce_raw(e) ? raw : ax_ce_size(e); if (c > zo[s] + z[s] - f) return false; t.cs[i] = e; t.ch[i] = {i * t.CH, raw, f, f + c, zo[s] + 8 + 8 * i}; f += c; }
            if (f != zo[s] + z[s]) return false; }
        // block slices back to back and inside the streams
        const uint64_t ss[4] = {lsz, tk[1].S, tk[2].S, tk[3].S};
        for (uint64_t b = 0; b < nb; b++) for (int s = 0; s < 4; s++) { const uint64_t o = bo(b, s), m = bo(b, 4 + s);
            if (o > ss[s] || m > ss[s] - o) return false; if (b && o != bo(b - 1, s) + bo(b - 1, 4 + s)) return false; }
        lit.resize(lsz + 64); for (int s = 1; s < 4; s++) tok[s].resize(tk[s].S + 64); out.resize(orig + 64);
        lok.assign(lch.size(), 0); for (int s = 1; s < 4; s++) tok_ok[s].assign(tk[s].ch.size(), 0); bok.assign(nb, 0);
        parsed = true; return true;
    }
    bool dec_lit(size_t t) { const Chunk& c = lch[t]; const AxLitChunk d{(size_t)c.off, (size_t)c.raw, a + c.lo, (size_t)(c.hi - c.lo), ltag}; return lok[t] = ax_lit_chunk_decode(d, lit.data() + c.off); }
    bool dec_tok(int s, size_t i) { const Chunk& c = tk[s].ch[i]; return tok_ok[s][i] = ax_tok_chunk(tk[s].cs[i], tok[s].data() + c.off, (size_t)c.raw, a + c.lo, (size_t)(c.hi - c.lo)); }
    // a block decoded with its checks: every token inside its slices, the slices consumed exactly, exactly its size written
    bool dec_block(uint64_t b) {
        const uint64_t n0 = std::min(bs, orig - b * bs); uint8_t* dst = out.data() + b * bs;
        const uint8_t *L = lit.data() + bo(b, 0), *O = tok[1].data() + bo(b, 1), *N = tok[2].data() + bo(b, 2), *C = tok[3].data() + bo(b, 3);
        const uint64_t ls = bo(b, 4), os = bo(b, 5), ns = bo(b, 6), cs = bo(b, 7);
        size_t lp = 0, op = 0, np = 0, cp = 0, o = 0; uint32_t rep[4] = {1, 2, 4, 8};
        auto var = [&](const uint8_t* buf, size_t& q, size_t lim, bool& bad) { uint32_t v = 0; for (int k = 0; k < 5; k++) { if (q >= lim) { bad = true; return 0u; } const uint8_t c = buf[q++]; v |= (uint32_t)(c & 0x7F) << (7 * k); if (!(c & 0x80)) return v; } bad = true; return 0u; };
        bool bad = false;
        while (cp < cs && !bad) { const uint8_t c = C[cp++];
            if (c == 0xFF) { rep[0] = 1; rep[1] = 2; rep[2] = 4; rep[3] = 8; continue; }
            if (c < 0x80) { const uint32_t l = c + 1u; if (lp + l > ls || o + l > n0) { bad = true; break; } memcpy(dst + o, L + lp, l); lp += l; o += l; continue; }
            uint32_t l, d;
            if ((c & 0xC0) == 0x80) { uint32_t ri = (c >> 4) & 3, lv = c & 0x0F; if (lv == 0x0F) lv += var(N, np, ns, bad); l = lv + 6; d = rep[ri]; if (ri) { for (int q = (int)ri; q > 0; q--) rep[q] = rep[q - 1]; rep[0] = d; } }
            else { const uint32_t lv = c == 0xFE ? var(N, np, ns, bad) : (uint32_t)(c & 0x3F); l = lv + 6; d = var(O, op, os, bad); rep[3] = rep[2]; rep[2] = rep[1]; rep[1] = rep[0]; rep[0] = d; }
            if (bad || !d || d > o || o + l > n0) { bad = true; break; }
            for (uint32_t k = 0; k < l; k++) dst[o + k] = dst[o + k - d]; o += l; }
        bool okb = !bad && o == n0 && lp == ls && op == os && np == ns && cp == cs;
        if (okb && g_bh && (b >= g_bh->size() || XXH3_64bits(dst, n0) != (*g_bh)[b])) okb = false;   // sidecar: the block's own hash
        return bok[b] = okb;
    }
    // everything; the failing units
    std::vector<Unit> full(int T) {
        std::vector<Unit> u; if (!parse()) { u.push_back({0, 0, (uint64_t)std::min<size_t>(n, 68 + 64 * std::max<uint64_t>(nb, 1))}); return u; }
        std::atomic<size_t> nx{0}; std::vector<std::thread> th;
        for (int t = 0; t < T; t++) th.emplace_back([&] { for (size_t i; (i = nx++) < lch.size(); ) dec_lit(i); }); for (auto& x : th) x.join(); th.clear();
        for (int s = 1; s < 4; s++) for (size_t i = 0; i < tk[s].ch.size(); i++) dec_tok(s, i);
        nx = 0; for (int t = 0; t < T; t++) th.emplace_back([&] { for (size_t b; (b = nx++) < nb; ) dec_block(b); }); for (auto& x : th) x.join();
        for (size_t t = 0; t < lch.size(); t++) if (!lok[t]) u.push_back({1, lch[t].lo, lch[t].hi});
        for (int s = 1; s < 4; s++) for (size_t i = 0; i < tk[s].ch.size(); i++) if (!tok_ok[s][i]) u.push_back({2, tk[s].ch[i].lo, tk[s].ch[i].hi});
        if (u.empty()) for (uint64_t b = 0; b < nb; b++) if (!bok[b]) { u.push_back({3, 68 + 64 * b, 68 + 64 * b + 64}); break; }
        return u;
    }
    bool oracle() { return XXH3_64bits(out.data(), orig) == xxh; }
    // the blocks that read stream bytes [lo, hi) of stream s
    void blocks_of(int s, uint64_t lo, uint64_t hi, std::vector<uint64_t>& v) const { v.clear();
        for (uint64_t b = 0; b < nb; b++) { const uint64_t o = bo(b, s), m = bo(b, 4 + s); if (m && o < hi && o + m > lo) v.push_back(b); } }
};
// SH_GPU=1: a literal chunk probe through the GPU library's path instead of the CPU decoder - the plan's framing check
// (axo_parse, as agp::build), the rANS pieces as k_rans jobs (status = axr_decode, judged == the warp steps by
// rans_warp_emu), the case runs and gaps as k_open_cg (ax_open_warp.h steps, as scripts/gpu_plan_emu.cpp Exec::open),
// the bases as k_open_bases (axl_bases16_v); probes are counted in launches of SH_BATCH (4096): the device decodes a
// batch of probe chunks per launch, one job set each, and stops at the first batch with the fix.
static bool gpu_lit_probe(const uint8_t* c, size_t csz, uint32_t raw, uint8_t* dst) {
    if (csz < 2) return false;
    if (c[0] == 3) { if (c[1] > 1) return false; if (c[1] == 0) { if (csz - 2 != raw) return false; memcpy(dst, c + 2, raw); return true; } return axr_decode(c + 2, csz - 2, dst, raw) == 0; }
    if (c[0] != 2) return false;
    AxoParts Q; if (axo_parse(c + 1, csz - 1, raw, &Q)) return false;           // the plan refuses it
    static thread_local std::vector<uint8_t> pc[4]; static thread_local std::vector<uint32_t> ends, ep;
    for (int k = 0; k < 4; k++) { pc[k].resize((size_t)Q.n[k] + 64); if (!Q.n[k]) continue;
        const uint8_t* p = c + 1 + Q.off[k];
        if (Q.mode[k] == 0) memcpy(pc[k].data(), p, Q.n[k]); else if (axr_decode(p, Q.h[k], pc[k].data(), Q.n[k])) return false; }   // k_rans status
    bool bad = false; uint64_t sum = 0; uint32_t j = 0; ends.assign((size_t)Q.ncse + 1, 0);
    for (uint32_t t = 0; t < Q.ncse; t++) { const bool term = axl_term(t, pc[1].data(), Q.ncse); const uint32_t v = axl_value(t, pc[1].data(), term, bad);
        if (term) { sum += v; axl_cse_end(term, j, v, sum, raw, ends.data(), bad); j++; } }
    if (axl_tail_bad(pc[1].data(), Q.ncse) || sum != raw || bad) return false;     // k_open_cg: STATUS_OPEN
    const uint32_t R = j; ep.assign((size_t)Q.nexc + 1, 0);
    if (Q.nexc) { sum = 0; j = 0; for (uint32_t t = 0; t < Q.ngap; t++) { const bool term = axl_term(t, pc[2].data(), Q.ngap); const uint32_t v = axl_value(t, pc[2].data(), term, bad);
            if (term) { sum += v; axl_exc_pos(term, j, v, sum, Q.nexc, raw, ep.data(), bad); j++; } }
        if (axl_tail_bad(pc[2].data(), Q.ngap) || j != Q.nexc || bad) return false; }
    for (uint32_t g = 0; 16 * g < raw; g++) axl_bases16_v(g, pc[0].data(), ends.data(), R, raw, dst, axl_run_of(ends.data(), R, 16 * g), ep.data(), Q.nexc, pc[3].data(), axl_exc_in(ep.data(), 0, Q.nexc, 16 * g));
    return true;
}
static const bool g_gpu = getenv("SH_GPU") && atoi(getenv("SH_GPU"));
static const uint64_t g_batch = getenv("SH_BATCH") ? strtoull(getenv("SH_BATCH"), 0, 10) : 4096;
// the byte ranges a failing block depends on: its table entry, the chunks of its four slices
static void block_ranges(const Dec& D, uint64_t b, std::vector<Unit>& r) {
    r.push_back({0, 68 + 64 * b, 68 + 64 * b + 64});
    { const uint64_t o = D.bo(b, 0), m = D.bo(b, 4); if (m) for (size_t t = o / D.lcsz; t <= (o + m - 1) / D.lcsz && t < D.lch.size(); t++) r.push_back({1, D.lch[t].lo, D.lch[t].hi}); }
    for (int s = 1; s < 4; s++) { const uint64_t o = D.bo(b, s), m = D.bo(b, 4 + s); if (!m) continue;
        for (size_t i = o / D.tk[s].CH; i <= (o + m - 1) / D.tk[s].CH && i < D.tk[s].ch.size(); i++) r.push_back({2, D.tk[s].ch[i].lo, D.tk[s].ch[i].hi}); }
}
}  // namespace sh

int main(int argc, char** argv) {
    using namespace sh;
    if (argc == 4 && !strcmp(argv[1], "--sidecar")) {                // block hashes of the intact archive
        std::vector<uint8_t> a; FILE* f = fopen(argv[2], "rb"); if (!f) { perror(argv[2]); return 1; } fseek(f, 0, SEEK_END); a.resize((size_t)ftell(f)); fseek(f, 0, SEEK_SET); if (fread(a.data(), 1, a.size(), f) != a.size()) return 1; fclose(f);
        Dec D; D.a = a.data(); D.n = a.size(); if (!D.full(16).empty() || !D.oracle()) { fprintf(stderr, "not an intact archive\n"); return 1; }
        std::vector<uint64_t> h(D.nb); for (uint64_t b = 0; b < D.nb; b++) h[b] = XXH3_64bits(D.out.data() + b * D.bs, std::min(D.bs, D.orig - b * D.bs));
        FILE* o = fopen(argv[3], "wb"); fwrite(h.data(), 8, h.size(), o); fclose(o); printf("%llu blocks, %zu B of block hashes\n", (unsigned long long)D.nb, h.size() * 8); return 0; }
    if (argc < 4) { fprintf(stderr, "usage: %s <archive.aet> <copies> <flips> [seed] [sidecar]\n", argv[0]); return 1; }
    static std::vector<uint64_t> bh;
    if (argc > 5) { FILE* f = fopen(argv[5], "rb"); if (!f) { perror(argv[5]); return 1; } fseek(f, 0, SEEK_END); bh.resize((size_t)ftell(f) / 8); fseek(f, 0, SEEK_SET); if (fread(bh.data(), 8, bh.size(), f) != bh.size()) return 1; fclose(f); g_bh = &bh; }
    std::vector<uint8_t> a0; { FILE* f = fopen(argv[1], "rb"); if (!f) { perror(argv[1]); return 1; } fseek(f, 0, SEEK_END); a0.resize((size_t)ftell(f)); fseek(f, 0, SEEK_SET); if (fread(a0.data(), 1, a0.size(), f) != a0.size()) return 1; fclose(f); }
    const int NC = atoi(argv[2]), NF = atoi(argv[3]); const uint64_t seed = argc > 4 ? strtoull(argv[4], 0, 10) : 1;
    const int T = (int)std::thread::hardware_concurrency();
    std::atomic<int> next{0}; std::mutex mu;
    long harmless = 0, found = 0, falsefix = 0, notfound = 0, nolocator = 0; uint64_t probes = 0, oracles = 0, gprobes = 0, glaunch = 0; double cpu = 0;
    std::vector<std::thread> th;
    for (int t = 0; t < T; t++) th.emplace_back([&] {
        for (int c; (c = next++) < NC; ) {
            std::mt19937_64 g(seed * 1000003 + (uint64_t)c); std::vector<uint8_t> a = a0;
            for (int k = 0; k < NF; k++) { const uint64_t p = g() % a.size(); a[p] ^= (uint8_t)(1u << (g() % 8)); }
            const double t0 = now(); Dec D; D.a = a.data(); D.n = a.size(); uint64_t pr = 0, orc = 0, gp = 0; int res = -1;   // 0 harmless 1 found 2 false 3 not found 4 no locator
            for (int round = 0; round < NF + 1 && res < 0; round++) {
                std::vector<Unit> u = D.full(1);
                if (u.empty()) { orc++; if (D.oracle()) { res = round == 0 ? 0 : (a == a0 ? 1 : 2); break; } res = round == 0 ? 4 : 3; break; }
                std::vector<Unit> R; if (u[0].kind == 3) block_ranges(D, (u[0].lo - 68) / 64, R); else R.push_back(u[0]);
                if (u[0].kind == 1) { const size_t t0i = (size_t)(std::lower_bound(D.lch.begin(), D.lch.end(), u[0].lo, [](const Chunk& c, uint64_t x) { return c.lo < x; }) - D.lch.begin());
                    R.push_back({0, D.lch[t0i].ent - (t0i ? 8 : 0), D.lch[t0i].ent + 8}); }          // or the size entry of this chunk / the one before
                if (u[0].kind == 2) { for (int s = 1; s < 4; s++) for (const Chunk& c : D.tk[s].ch) if (c.lo == u[0].lo) R.push_back({0, c.ent, c.ent + 8}); }
                bool fixed = false; std::vector<uint8_t> scratch, save; std::vector<uint64_t> bl; std::vector<std::vector<uint8_t>> bsave;
                auto all_ok = [&](const Dec& E) { for (size_t t2 = 0; t2 < E.lch.size(); t2++) if (!E.lok[t2]) return false;
                    for (int s2 = 1; s2 < 4; s2++) for (size_t i2 = 0; i2 < E.tk[s2].ch.size(); i2++) if (!E.tok_ok[s2][i2]) return false;
                    for (uint64_t b = 0; b < E.nb; b++) if (!E.bok[b]) return false; return true; };
                for (const Unit& r : R) { for (uint64_t byte = r.lo; byte < r.hi && !fixed; byte++) for (int bit = 0; bit < 8 && !fixed; bit++) {
                    a[byte] ^= (uint8_t)(1u << bit); pr++;
                    if (r.kind == 1 || r.kind == 2) {                  // a chunk body: that chunk into scratch; only a chunk that decodes goes further
                        int s = 0; size_t idx = 0; const Chunk* c = nullptr; bool pass;
                        if (r.kind == 1) { idx = (size_t)(std::lower_bound(D.lch.begin(), D.lch.end(), byte, [](const Chunk& q, uint64_t x) { return q.hi <= x; }) - D.lch.begin()); c = &D.lch[idx];
                            scratch.resize(c->raw + 64);
                            if (g_gpu && D.ltag) { pass = gpu_lit_probe(a.data() + c->lo, (size_t)(c->hi - c->lo), (uint32_t)c->raw, scratch.data()); gp++; }
                            else { const AxLitChunk d{(size_t)c->off, (size_t)c->raw, a.data() + c->lo, (size_t)(c->hi - c->lo), D.ltag}; pass = ax_lit_chunk_decode(d, scratch.data()); } }
                        else { for (s = 1; s < 4; s++) if (byte >= D.tk[s].hlo && byte < D.zo[s] + D.z[s]) break;
                            idx = (size_t)(std::lower_bound(D.tk[s].ch.begin(), D.tk[s].ch.end(), byte, [](const Chunk& q, uint64_t x) { return q.hi <= x; }) - D.tk[s].ch.begin()); c = &D.tk[s].ch[idx];
                            scratch.resize(c->raw + 64); pass = ax_tok_chunk(D.tk[s].cs[idx], scratch.data(), (size_t)c->raw, a.data() + c->lo, (size_t)(c->hi - c->lo)); }
                        if (pass) {                                     // in place, with the old bytes saved: the chunk, the blocks that read it
                            uint8_t* dst = (r.kind == 1 ? D.lit.data() : D.tok[s].data()) + c->off; save.assign(dst, dst + c->raw); memcpy(dst, scratch.data(), c->raw);
                            char& okf = r.kind == 1 ? D.lok[idx] : D.tok_ok[s][idx]; const char okold = okf; okf = 1;
                            D.blocks_of(r.kind == 1 ? 0 : s, c->off, c->off + c->raw, bl); bsave.resize(bl.size()); std::vector<char> bold(bl.size()); bool bpass = true;
                            for (size_t q = 0; q < bl.size(); q++) { const uint64_t b = bl[q], n0 = std::min(D.bs, D.orig - b * D.bs); bsave[q].assign(D.out.data() + b * D.bs, D.out.data() + b * D.bs + n0); bold[q] = D.bok[b];
                                if (!D.dec_block(b)) bpass = false; }
                            if (bpass && all_ok(D)) { orc++; if (D.oracle()) fixed = true; }
                            else if (bpass && NF > 1) fixed = true;     // several flips: this unit and its blocks pass, the rest is the next round's
                            if (!fixed) { memcpy(dst, save.data(), c->raw); okf = okold;
                                for (size_t q = 0; q < bl.size(); q++) { memcpy(D.out.data() + bl[q] * D.bs, bsave[q].data(), bsave[q].size()); D.bok[bl[q]] = bold[q]; } } }
                    } else {                                            // layout bytes: the whole archive again
                        Dec E; E.a = a.data(); E.n = a.size(); const std::vector<Unit> u2 = E.full(1);
                        if (u2.empty()) { orc++; if (E.oracle()) { fixed = true; D = std::move(E); } }
                        else if (NF > 1 && E.parsed && !D.parsed) { fixed = true; D = std::move(E); } }   // several flips: the layout parses again
                    if (!fixed) a[byte] ^= (uint8_t)(1u << bit); } if (fixed) break; }
                if (!fixed) { res = 3; break; }
                D.a = a.data();
            }
            const double dt = now() - t0;
            std::lock_guard<std::mutex> lk(mu); probes += pr; oracles += orc; cpu += dt; gprobes += gp; glaunch += (gp + g_batch - 1) / g_batch;
            if (res == 0) harmless++; else if (res == 1) found++; else if (res == 2) falsefix++; else if (res == 3) notfound++; else nolocator++;
        } });
    for (auto& x : th) x.join();
    const long tried = NC - harmless;
    printf("[selfheal] %s: %d copies x %d flip(s): harmless %ld, found %ld, false %ld, not found %ld (+ %ld with no locator: only the oracle failed); probes %.0f per repaired-or-tried copy, oracle calls %llu; CPU %.1f s in all, %.3f s per copy\n",
           argv[1], NC, NF, harmless, found, falsefix, notfound, nolocator, tried ? (double)probes / tried : 0.0, (unsigned long long)oracles, cpu, cpu / NC);
    if (g_bh) printf("[selfheal] with the block-hash sidecar (%zu B)\n", bh.size() * 8);
    if (g_gpu) printf("[selfheal] GPU path: %llu literal-chunk probes in %llu launches of %llu (%.1f per copy); H100 estimate at 47 212 open chunks in 19.06 ms (t2t.open library decode, 946591a): %.1f ms per copy (launches x batch probes)\n",
                      (unsigned long long)gprobes, (unsigned long long)glaunch, (unsigned long long)g_batch, (double)glaunch / NC, (double)(glaunch * g_batch) / NC / (47212.0 / 19.062e-3) * 1e3);   // whole batches are decoded
    printf("SHROW\t%d\t%d\t%ld\t%ld\t%ld\t%ld\t%ld\t%.0f\t%.3f\n", NC, NF, harmless, found, falsefix, notfound, nolocator, tried ? (double)probes / tried : 0.0, cpu / NC);
    return 0;
}
