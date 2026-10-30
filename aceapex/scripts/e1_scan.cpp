// e1_scan.cpp - E1: is a scan over the FAST archive faster than the same scan over the raw bytes in memory?
// W1 (results/w1-2026-10-27-bd6b27f.log) measured one core decoding at 18-49 GB/s when its output stays in cache (E0)
// and at 10-11 GB/s when it is written to DRAM. E1 asks the consumer's question: the data is in RAM, the job is a scan.
//   A  raw: the original bytes in RAM, the scan reads them (1 byte from DRAM per byte scanned)
//   B  fused: the archive in RAM; each block is decoded into a per-thread scratch that stays in L2 and scanned there
//      (~ratio bytes from DRAM per byte scanned, nothing written back)
// Scans: (1) count of '\n' (wc -l), (2) occurrences of a 4-byte pattern, across block boundaries too. Both paths must give
// the same counts; GB/s are bytes of the original per second. Threads 1 and T, median of R, archive at any level/block.
// Not a library path: a measurement tool (claim E1, results/e1-*.log). Prior art to name with any claim: Blosc
// ("faster than memcpy"), FastLanes (fused decode on integer encodings), DPF on GPU, tile-based GPU decompression.
// Build: g++ -O3 -std=c++17 -Isrc -o e1_scan scripts/e1_scan.cpp -lpthread
// Usage: e1_scan file [level=1] [block_KiB=256] [threads=8] [reps=7] [pattern="the "]
#include "ax_fast_enc.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <algorithm>
static double now() { return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); }

struct Arc { std::vector<uint8_t> z; std::vector<uint64_t> off; uint32_t bs, nb, mm; size_t orig; };
static Arc build(const std::vector<uint8_t>& in, int level, uint32_t bs) {
    Arc a; axf::Params P; axf::level_params(level, P); a.bs = bs; a.mm = P.mm; a.orig = in.size();
    a.nb = (uint32_t)((in.size() + bs - 1) / bs); a.off.resize(a.nb + 1);
    std::vector<std::vector<uint8_t>> blocks(a.nb);
    axf::Job j; j.src = in.data(); j.n = in.size(); j.bs = bs; j.nb = a.nb; j.P = P; j.blocks = &blocks; j.next = 0; j.err = 0;
    std::vector<std::thread> th; for (int t = 0; t < 8; t++) th.emplace_back([&] { axf::enc_worker(&j); }); for (auto& x : th) x.join();
    size_t o = 0; for (uint32_t b = 0; b < a.nb; b++) { a.off[b] = o; o += blocks[b].size(); } a.off[a.nb] = o;
    a.z.resize(o); for (uint32_t b = 0; b < a.nb; b++) memcpy(a.z.data() + a.off[b], blocks[b].data(), blocks[b].size());
    return a;
}
static inline uint32_t blen(const Arc& a, uint32_t b) { size_t st = (size_t)b * a.bs; return (uint32_t)std::min<size_t>(a.bs, a.orig - st); }

// the consumers: plain loops, the same code on both paths (the compiler vectorizes the newline count)
// newline count with 8-bit lane counters (chunks of 255 x 64 bytes, then widened): vectorizes to compare + subtract per
// 16/32 bytes, so the scan itself runs far above memory speed and the comparison measures where the bytes come from
static inline uint64_t count_nl(const uint8_t* p, size_t n) {
    uint64_t c = 0; size_t i = 0;
    while (n - i >= 64) {
        uint8_t acc[64] = {0}; const size_t steps = std::min<size_t>(255, (n - i) / 64);
        for (size_t s = 0; s < steps; s++, i += 64) for (int k = 0; k < 64; k++) acc[k] += p[i + k] == '\n';
        for (int k = 0; k < 64; k++) c += acc[k];
    }
    for (; i < n; i++) c += p[i] == '\n';
    return c;
}
static inline uint64_t count_pat(const uint8_t* p, size_t n, uint32_t pat) {   // occurrences starting in [0, n - 3)
    uint64_t c = 0; if (n < 4) return 0;
    for (size_t i = 0; i + 4 <= n; i++) { uint32_t v; memcpy(&v, p + i, 4); c += v == pat; }
    return c;
}
struct Res { uint64_t nl = 0, pat = 0; };
// occurrences of the pattern that straddle the boundary between two adjacent pieces: tail 3 bytes + head 3 bytes
static uint64_t straddle(const uint8_t* tail3, const uint8_t* head3, uint32_t pat) { uint8_t w[6]; memcpy(w, tail3, 3); memcpy(w + 3, head3, 3); return count_pat(w, 6, pat); }

int main(int argc, char** argv) {
    if (argc < 2) { fprintf(stderr, "usage: %s file [level=1] [block_KiB=256] [threads=8] [reps=7] [pattern]\n", argv[0]); return 1; }
    FILE* f = fopen(argv[1], "rb"); if (!f) { perror(argv[1]); return 1; }
    fseek(f, 0, SEEK_END); size_t n = (size_t)ftell(f); fseek(f, 0, SEEK_SET);
    std::vector<uint8_t> in(n); if (fread(in.data(), 1, n, f) != n) return 1; fclose(f);
    const int level = argc > 2 ? atoi(argv[2]) : 1; const uint32_t bs = (argc > 3 ? (uint32_t)atoi(argv[3]) : 256u) * 1024u;
    const int T = argc > 4 ? atoi(argv[4]) : 8, R = argc > 5 ? atoi(argv[5]) : 7;
    const std::string ps = argc > 6 ? argv[6] : "the "; if (ps.size() != 4) { fprintf(stderr, "pattern: 4 bytes\n"); return 1; }
    uint32_t pat; memcpy(&pat, ps.data(), 4);
    if (bs > AXF_BLOCK_MAX || bs < 4096 || level < 1 || level > 8 || n < 8) { fprintf(stderr, "level 1..8, block 4..1024 KiB\n"); return 1; }
    Arc a = build(in, level, bs);
    printf("E1 %s: %zu B, level %d (min match %u), %u blocks of %u KiB -> %zu B = %.2f %%; pattern \"%s\"; reps %d\n",
           argv[1], n, level, a.mm, a.nb, bs >> 10, a.z.size(), 100.0 * a.z.size() / n, ps.c_str(), R);

    bool PAT = false;   // scan 1: newlines only (a consumer cheaper than memory); scan 2: newlines + the 4-byte pattern
    // A: raw bytes, each thread scans a contiguous range of whole blocks (the same ranges as B), straddles added after
    auto scanA = [&](int th) {
        std::vector<Res> part(th); std::vector<std::thread> ts;
        for (int t = 0; t < th; t++) ts.emplace_back([&, t] { const uint32_t b0 = a.nb * (uint64_t)t / th, b1 = a.nb * (uint64_t)(t + 1) / th;
            const size_t s = (size_t)b0 * a.bs, e = std::min<size_t>((size_t)b1 * a.bs, n);
            if (e > s) { part[t].nl = count_nl(in.data() + s, e - s); if (PAT) part[t].pat = count_pat(in.data() + s, e - s, pat); } });
        for (auto& x : ts) x.join();
        Res r; for (auto& p : part) { r.nl += p.nl; r.pat += p.pat; }
        for (int t = 1; t < th; t++) { const uint32_t b = a.nb * (uint64_t)t / th; if (b == 0 || b >= a.nb) continue;
            const size_t o = (size_t)b * a.bs; if (PAT && o >= 3 && o + 3 <= n) r.pat += straddle(in.data() + o - 3, in.data() + o, pat); }
        return r;
    };
    // B: decode block into scratch (L2), scan it there; per block keep first/last 3 bytes for the straddles
    std::vector<uint8_t> edge((size_t)a.nb * 6);
    auto scanB = [&](int th, bool& ok) {
        std::vector<Res> part(th); std::vector<int> bad(th, 0); std::vector<std::thread> ts;
        for (int t = 0; t < th; t++) ts.emplace_back([&, t] {
            std::vector<uint8_t> scr(a.bs + 64); const uint32_t b0 = a.nb * (uint64_t)t / th, b1 = a.nb * (uint64_t)(t + 1) / th;
            for (uint32_t b = b0; b < b1; b++) { const uint32_t k = blen(a, b);
                if (!axf_dec_block(a.z.data() + a.off[b], a.off[b + 1] - a.off[b], scr.data(), k, a.mm)) { bad[t] = 1; return; }
                part[t].nl += count_nl(scr.data(), k); if (PAT) part[t].pat += count_pat(scr.data(), k, pat);
                if (k >= 3) { memcpy(&edge[6ull * b], scr.data(), 3); memcpy(&edge[6ull * b + 3], scr.data() + k - 3, 3); } } });
        for (auto& x : ts) x.join();
        Res r; for (int t = 0; t < th; t++) { r.nl += part[t].nl; r.pat += part[t].pat; ok = ok && !bad[t]; }
        if (PAT) for (uint32_t b = 1; b < a.nb; b++) if (blen(a, b - 1) >= 3 && blen(a, b) >= 3) r.pat += straddle(&edge[6ull * (b - 1) + 3], &edge[6ull * b], pat);
        return r;
    };
    // reference: one pass over the raw bytes
    const Res ref{count_nl(in.data(), n), count_pat(in.data(), n, pat)};
    printf("reference: %llu newlines, %llu x \"%s\"\n", (unsigned long long)ref.nl, (unsigned long long)ref.pat, ps.c_str());
    bool all = true;
    for (int sc = 0; sc < 2; sc++) { PAT = sc == 1; printf("scan %d: %s\n", sc + 1, PAT ? "newlines + 4-byte pattern" : "newlines only (wc -l)");
    for (int th : {1, T}) {
        double ta, tb; bool okA = true, okB = true;
        { std::vector<double> v; for (int r = 0; r < R; r++) { double t0 = now(); Res x = scanA(th); v.push_back(now() - t0); okA = okA && x.nl == ref.nl && (!PAT || x.pat == ref.pat); } std::sort(v.begin(), v.end()); ta = v[R / 2]; }
        { std::vector<double> v; for (int r = 0; r < R; r++) { double t0 = now(); bool ok = true; Res x = scanB(th, ok); v.push_back(now() - t0); okB = okB && ok && x.nl == ref.nl && (!PAT || x.pat == ref.pat); } std::sort(v.begin(), v.end()); tb = v[R / 2]; }
        printf("threads %2d: A raw %7.2f GB/s %s | B fused decode+scan %7.2f GB/s %s | B/A %.2fx\n", th, n / ta / 1e9, okA ? "counts ok" : "COUNTS WRONG",
               n / tb / 1e9, okB ? "counts ok" : "COUNTS WRONG", ta / tb);
        all = all && okA && okB;
    } }
    printf("E1 %s\n", all ? "all counts equal the reference" : "MISMATCH");
    return all ? 0 : 1;
}
