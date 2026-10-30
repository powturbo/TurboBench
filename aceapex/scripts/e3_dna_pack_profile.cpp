// E3 (2026-09-29): what zstd buys on the literal sub-frames of a DNA archive, measured with
// real coders. For every tagged literal chunk (spec: mode byte 1 = DNA pack) each of the four
// sub-streams (seq = 2-bit bases, cse = case mask, gap = u32 exception gaps, val = exception
// bytes) is decoded and re-coded as
//   zstd     the stored frame (what the archive pays now)
//   raw      the plain bytes
//   rans     order-0 32-lane rANS (src/ax_rans.h, the ADR-018 coder) in 64 KiB pieces
//   best     per piece the smaller of raw and rans (what an encoder would write)
// and, as zstd-free codings of the two run-shaped streams:
//   cse.rle  mask as alternating 0/1 run lengths (LEB128), then best(raw, rans)
//   gap.leb  gaps as LEB128 instead of u32, then best(raw, rans)
//   exc.run  exceptions as runs of one byte value: (gap to the run, LEB128), (run length,
//            LEB128), value - three streams, each best(raw, rans); replaces gap + val
// Plain literal chunks (mode byte 0, zstd of the bytes) are measured the same way.
// Output: bytes per stream and coding, and the change of the whole archive in %.
// Build: g++ -std=c++17 -O2 -Isrc -o e3 scripts/e3_dna_pack_profile.cpp -lzstd
// Usage: e3 archive.aet
#include "ax_rans.h"
#include <zstd.h>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <vector>
#include <string>

static uint64_t rd64(const uint8_t* p) { uint64_t v; memcpy(&v, p, 8); return v; }
static uint32_t rd32(const uint8_t* p) { uint32_t v; memcpy(&v, p, 4); return v; }

static size_t rans_size(const uint8_t* p, size_t n) {           // order-0 rANS in 64 KiB pieces
    static std::vector<uint8_t> out; static std::vector<uint16_t> w;
    size_t tot = 0;
    for (size_t o = 0; o < n; o += 65536) {
        size_t k = n - o < 65536 ? n - o : 65536;
        out.resize(axr_bound(k)); w.resize(k + 2 * AXR_LANES);
        size_t r = axr_encode(p + o, k, out.data(), w.data());
        tot += r ? r : (size_t)-1 / 4;
    }
    return tot;
}
static size_t best_size(const uint8_t* p, size_t n) {
    size_t tot = 0;
    for (size_t o = 0; o < n; o += 65536) {
        size_t k = n - o < 65536 ? n - o : 65536, r = rans_size(p + o, k);
        tot += r < k ? r : k;
    }
    return tot;
}
static void leb(std::vector<uint8_t>& v, uint32_t x) { do { uint8_t b = x & 0x7F; x >>= 7; v.push_back(b | (x ? 0x80 : 0)); } while (x); }

struct Acc { uint64_t zstd = 0, raw = 0, rans = 0, best = 0, alt = 0; };

int main(int argc, char** argv) {
    if (argc < 2) { fprintf(stderr, "usage: %s archive.aet\n", argv[0]); return 1; }
    FILE* f = fopen(argv[1], "rb"); if (!f) { perror(argv[1]); return 1; }
    std::vector<uint8_t> a; { uint8_t b[1 << 16]; size_t r; while ((r = fread(b, 1, sizeof b, f)) > 0) a.insert(a.end(), b, b + r); } fclose(f);
    if (a.size() < 68 || memcmp(a.data(), "ACEPX2\0\0", 8)) { fprintf(stderr, "not ACEPX2\n"); return 1; }
    uint32_t nb = rd32(&a[24]); uint64_t zl = rd64(&a[36]);
    const uint8_t* z = a.data() + 68 + 64ull * nb;
    uint64_t h = rd64(z);
    if (!(h >> 62 & 1) || !(h >> 61 & 1) || !(h >> 60 & 1)) { fprintf(stderr, "literal stream is not chunked+tagged\n"); return 1; }
    uint64_t sz = h & ((1ull << 60) - 1), CH = rd64(z + 8), NW = (sz + CH - 1) / CH;
    size_t pos = 16 + 8 * NW;
    Acc seq, cse, gap, val, pl; uint64_t excrun = 0; uint64_t hdr = 0, ndna = 0, nplain = 0, nexc_all = 0;
    std::vector<uint8_t> d1, d2, d3, d4, tmp;
    for (uint64_t t = 0; t < NW; t++) {
        uint64_t raw = (t + 1) * CH <= sz ? CH : sz - t * CH, csz = rd64(z + 16 + 8 * t);
        const uint8_t* c = z + pos; pos += csz;
        if (pos > zl) { fprintf(stderr, "chunk past stream\n"); return 1; }
        if (c[0] == 1) {
            ndna++;
            uint32_t nexc = rd32(c + 1), s[4] = {rd32(c + 5), rd32(c + 9), rd32(c + 13), rd32(c + 17)};
            nexc_all += nexc; hdr += 21;
            size_t osz[4] = {(size_t)(raw + 3) / 4, (size_t)(raw + 7) / 8, (size_t)nexc * 4, (size_t)nexc};
            std::vector<uint8_t>* d[4] = {&d1, &d2, &d3, &d4}; Acc* ac[4] = {&seq, &cse, &gap, &val};
            const uint8_t* q = c + 21;
            for (int k = 0; k < 4; k++) {
                d[k]->assign(osz[k] ? osz[k] : 1, 0);
                if (s[k]) { size_t r = ZSTD_decompress(d[k]->data(), osz[k], q, s[k]); if (ZSTD_isError(r) || r != osz[k]) { fprintf(stderr, "zstd error chunk %llu sub %d\n", (unsigned long long)t, k); return 1; } }
                q += s[k];
                ac[k]->zstd += s[k]; ac[k]->raw += osz[k];
                if (osz[k]) { ac[k]->rans += rans_size(d[k]->data(), osz[k]); ac[k]->best += best_size(d[k]->data(), osz[k]); }
            }
            tmp.clear();                                              // cse as alternating runs of 0/1 bits
            { uint32_t run = 0; int cur = 0;
              for (uint64_t i = 0; i < raw; i++) { int bit = d2[i >> 3] >> (7 - (i & 7)) & 1; if (bit != cur) { leb(tmp, run); run = 0; cur = bit; } run++; }
              leb(tmp, run); }
            cse.alt += best_size(tmp.data(), tmp.size());
            tmp.clear();
            for (uint32_t e = 0; e < nexc; e++) leb(tmp, rd32(&d3[4 * e]));
            if (nexc) gap.alt += best_size(tmp.data(), tmp.size());
            if (nexc) {                                               // exceptions as runs
                std::vector<uint8_t> g, l, v; uint64_t p = 0, prev_end = 0;
                for (uint32_t e = 0; e < nexc;) {
                    p += rd32(&d3[4 * e]); uint64_t start = p; uint8_t b = d4[e]; uint32_t len = 1; e++;
                    while (e < nexc && rd32(&d3[4 * e]) == 1 && d4[e] == b) { p++; len++; e++; }
                    leb(g, (uint32_t)(start - prev_end)); leb(l, len); v.push_back(b); prev_end = p + 1;
                }
                excrun += best_size(g.data(), g.size()) + best_size(l.data(), l.size()) + best_size(v.data(), v.size());
            }
        } else {
            nplain++; hdr += 1;
            tmp.assign(raw ? raw : 1, 0);
            size_t r = ZSTD_decompress(tmp.data(), raw, c + 1, csz - 1);
            if (ZSTD_isError(r) || r != raw) { fprintf(stderr, "zstd error plain chunk %llu\n", (unsigned long long)t); return 1; }
            pl.zstd += csz - 1; pl.raw += raw; pl.rans += rans_size(tmp.data(), raw); pl.best += best_size(tmp.data(), raw);
        }
    }
    const double A = (double)a.size();
    printf("E3 %s: archive %zu B, literal stream %llu B, %llu chunks of %llu (DNA %llu, plain %llu), exceptions %llu\n",
           argv[1], a.size(), (unsigned long long)zl, (unsigned long long)NW, (unsigned long long)CH,
           (unsigned long long)ndna, (unsigned long long)nplain, (unsigned long long)nexc_all);
    printf("%-6s %12s %12s %12s %12s %12s   %9s %9s\n", "stream", "zstd", "raw", "rans", "best", "alt(best)", "best-zstd", "alt-zstd");
    auto row = [&](const char* n, const Acc& x, bool alt) {
        printf("%-6s %12llu %12llu %12llu %12llu %12s   %+8.3f%% %s\n", n, (unsigned long long)x.zstd, (unsigned long long)x.raw,
               (unsigned long long)x.rans, (unsigned long long)x.best, alt ? std::to_string(x.alt).c_str() : "-",
               100.0 * ((double)x.best - (double)x.zstd) / A,
               alt ? (std::to_string(100.0 * ((double)x.alt - (double)x.zstd) / A).substr(0, 7) + "%").c_str() : "");
    };
    row("seq", seq, false); row("cse", cse, true); row("gap", gap, true); row("val", val, false); row("plain", pl, false);
    uint64_t Z = seq.zstd + cse.zstd + gap.zstd + val.zstd + pl.zstd;
    uint64_t B = seq.best + cse.best + gap.best + val.best + pl.best;
    uint64_t C = seq.best + cse.alt + gap.alt + val.best + pl.best;
    printf("exc.run (replaces gap+val): %llu vs zstd %llu (%+.3f%% of archive)\n", (unsigned long long)excrun,
           (unsigned long long)(gap.zstd + val.zstd), 100.0 * ((double)excrun - (double)(gap.zstd + val.zstd)) / A);
    uint64_t E = seq.best + cse.alt + excrun + pl.best;
    printf("zstd-free pack (seq best, cse.rle, exc.run, plain best): %llu vs zstd %llu (%+.3f%% of archive)\n",
           (unsigned long long)E, (unsigned long long)(seq.zstd + cse.zstd + gap.zstd + val.zstd + pl.zstd),
           100.0 * ((double)E - (double)(seq.zstd + cse.zstd + gap.zstd + val.zstd + pl.zstd)) / A);
    printf("all literal payload: zstd %llu, best(raw,rans) %llu (%+.3f%% of archive), with cse.rle+gap.leb %llu (%+.3f%% of archive)\n",
           (unsigned long long)Z, (unsigned long long)B, 100.0 * ((double)B - (double)Z) / A, (unsigned long long)C, 100.0 * ((double)C - (double)Z) / A);
    return 0;
}
