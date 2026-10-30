// region_htslib.cpp - one region at a time, in process, one thread: ACEAPEX against htslib on the same regions.
// Regions: a samtools-style list (name:start-end, 1-based; `gpu_h100_tests ra` writes <archive>.regions.txt, seed
// 20261002), offsets from the FASTA's .fai. Rows:
//   aceapex_decompress_region          bytes of the region as stored (bases + line ends), archive in memory
//   aceapex region + line ends out     the same, then the bases alone (what faidx_fetch_seq64 returns)
//   htslib faidx_fetch_seq64           bgzip file + .fai + .gzi, fai_load3 once, default BGZF cache, the returned
//                                      string freed per call
// The regions are visited in file order (random); one warm-up pass of 200 regions per row. Every result compared with
// the FASTA. p50 / p99 / mean per region in microseconds.
// Usage: region_htslib <archive.aet> <fasta> <fasta.bgz> <regions.txt>
// Build: g++ -std=c++17 -O2 -Isrc -I<htslib headers> scripts/region_htslib.cpp src/aceapex_api.cpp <libhts.so> -lzstd -lpthread
#include "aceapex.h"
#include <htslib/faidx.h>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>
static double now_s() { return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
static std::vector<uint8_t> slurp(const char* p) { FILE* f = fopen(p, "rb"); if (!f) { perror(p); exit(1); }
    fseek(f, 0, SEEK_END); std::vector<uint8_t> v((size_t)ftell(f)); fseek(f, 0, SEEK_SET); if (fread(v.data(), 1, v.size(), f) != v.size()) exit(1); fclose(f); return v; }
static double pct(std::vector<double> v, double q) { std::sort(v.begin(), v.end()); return v[std::min(v.size() - 1, (size_t)(q * v.size()))]; }
struct Fai { uint64_t len, off, lb, lw; };
struct Q { std::string name, reg; uint64_t s, e, lo, hi; };          // s, e: 0-based bases [s, e); lo, hi: bytes
static size_t strip(const uint8_t* p, size_t n, uint8_t* o) { size_t k = 0; for (size_t i = 0; i < n; i++) if (p[i] != '\n' && p[i] != '\r') o[k++] = p[i]; return k; }

int main(int argc, char** argv) {
    if (argc < 5) { fprintf(stderr, "usage: %s <archive.aet> <fasta> <fasta.bgz> <regions.txt>\n", argv[0]); return 1; }
    const std::vector<uint8_t> a = slurp(argv[1]), f = slurp(argv[2]);
    std::map<std::string, Fai> fai; { std::string fp = std::string(argv[2]) + ".fai"; FILE* x = fopen(fp.c_str(), "r");
        if (!x) { fp = std::string(argv[3]) + ".fai"; x = fopen(fp.c_str(), "r"); } if (!x) { fprintf(stderr, "no .fai\n"); return 1; }
        char nm[4096]; unsigned long long l, o, b, w; while (fscanf(x, "%4095s %llu %llu %llu %llu", nm, &l, &o, &b, &w) == 5) fai[nm] = {l, o, b, w}; fclose(x); }
    std::vector<Q> q; { FILE* x = fopen(argv[4], "r"); if (!x) { perror(argv[4]); return 1; } char ln[4096];
        while (fgets(ln, sizeof ln, x)) { std::string r(ln); while (!r.empty() && (r.back() == '\n' || r.back() == '\r')) r.pop_back(); if (r.empty()) continue;
            const size_t c = r.rfind(':'), d = r.find('-', c); const std::string nm = r.substr(0, c); auto it = fai.find(nm);
            if (c == std::string::npos || d == std::string::npos || it == fai.end()) { fprintf(stderr, "bad region %s\n", r.c_str()); return 1; }
            const Fai& F = it->second; const uint64_t s = strtoull(r.c_str() + c + 1, 0, 10) - 1, e = strtoull(r.c_str() + d + 1, 0, 10);
            auto bo = [&](uint64_t b) { return F.off + b / F.lb * F.lw + b % F.lb; };
            q.push_back({nm, r, s, e, bo(s), bo(e - 1) + 1}); } fclose(x); }
    const int N = (int)q.size(); size_t maxb = 0; for (auto& x : q) maxb = std::max<size_t>(maxb, x.hi - x.lo);
    std::vector<uint8_t> buf(maxb + 64), bs(maxb + 64), ref(maxb + 64);
    auto check = [&](const Q& x, const uint8_t* p, size_t n) { const size_t k = strip(f.data() + x.lo, x.hi - x.lo, ref.data()); return n == k && !memcmp(p, ref.data(), k); };
    std::vector<double> t1, t2, th; int b1 = 0, b2 = 0, bh = 0;
    for (int i = 0; i < std::min(N, 200); i++) aceapex_decompress_region(a.data(), a.size(), buf.data(), buf.size(), q[i].lo, q[i].hi - q[i].lo);
    for (const Q& x : q) { const double t0 = now_s(); const int64_t r = aceapex_decompress_region(a.data(), a.size(), buf.data(), buf.size(), x.lo, x.hi - x.lo);
        t1.push_back(now_s() - t0); if (r != (int64_t)(x.hi - x.lo) || memcmp(buf.data(), f.data() + x.lo, x.hi - x.lo)) b1++; }
    for (const Q& x : q) { const double t0 = now_s(); const int64_t r = aceapex_decompress_region(a.data(), a.size(), buf.data(), buf.size(), x.lo, x.hi - x.lo);
        const size_t k = r > 0 ? strip(buf.data(), (size_t)r, bs.data()) : 0; t2.push_back(now_s() - t0); if (!check(x, bs.data(), k)) b2++; }
    faidx_t* fx = fai_load3(argv[3], nullptr, nullptr, 0); if (!fx) { fprintf(stderr, "fai_load3 %s failed\n", argv[3]); return 1; }
    for (int i = 0; i < std::min(N, 200); i++) { hts_pos_t l = 0; free(faidx_fetch_seq64(fx, q[i].name.c_str(), q[i].s, q[i].e - 1, &l)); }
    for (const Q& x : q) { hts_pos_t l = 0; const double t0 = now_s(); char* s = faidx_fetch_seq64(fx, x.name.c_str(), x.s, x.e - 1, &l); const double t = now_s() - t0;
        th.push_back(t); if (!s || !check(x, (const uint8_t*)s, (size_t)l)) bh++; free(s); }
    fai_destroy(fx);
    auto line = [&](const char* nm, const std::vector<double>& v, int bad) { double m = 0; for (double x : v) m += x;
        printf("[rh] %-36s p50 %7.1f us  p99 %7.1f us  mean %7.1f us  %s\n", nm, pct(v, .5) * 1e6, pct(v, .99) * 1e6, m / v.size() * 1e6, bad ? "DIFFERS" : "== FASTA"); };
    printf("[rh] %d regions from %s, archive %zu B in memory, %s\n", N, argv[4], a.size(), argv[3]);
    line("aceapex_decompress_region", t1, b1); line("aceapex region + line ends out", t2, b2); line("htslib faidx_fetch_seq64", th, bh);
    printf("RHROW\t%d\t%.1f/%.1f\t%.1f/%.1f\t%.1f/%.1f\t%s\n", N, pct(t1, .5) * 1e6, pct(t1, .99) * 1e6, pct(t2, .5) * 1e6, pct(t2, .99) * 1e6,
           pct(th, .5) * 1e6, pct(th, .99) * 1e6, b1 || b2 || bh ? "FAILED" : "ok");
    return b1 || b2 || bh ? 5 : 0;
}
