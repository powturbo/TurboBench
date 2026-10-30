// fast_fuzz.cpp - the FAST profile decoder (ax_fast.h) under ASan/UBSan (claim head_fast_fuzz). Seeds: valid archives of
// random slices of an input (0..80 KiB, every level), each decoded back with exact-size buffers. Mutants: byte flips, runs
// of random bytes, truncation, extension, header and table fields, a wrong output size, pure garbage, token fields
// (ll/mc = 63, distances) - through axf_open + axf_dec_block_of with src and dst on exact-size heap buffers, so any read or
// write outside them is an ASan report. A mutant either decodes to n bytes or is rejected; nothing else.
// Build: g++ -O1 -g -std=c++17 -fsanitize=address,undefined -fno-sanitize-recover=all -Isrc -o fast_fuzz scripts/fast_fuzz.cpp src/aceapex_api.cpp -lzstd -lpthread
// Usage: fast_fuzz file seconds [seed]
#include "aceapex.h"
#include "ax_fast.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <memory>
#include <random>
#include <chrono>
static double now() { return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
int main(int argc, char** argv) {
    if (argc < 3) { fprintf(stderr, "usage: %s file seconds [seed]\n", argv[0]); return 1; }
    FILE* f = fopen(argv[1], "rb"); if (!f) { perror(argv[1]); return 1; }
    fseek(f, 0, SEEK_END); const size_t fn = (size_t)ftell(f); fseek(f, 0, SEEK_SET);
    std::vector<uint8_t> in(fn); if (fread(in.data(), 1, fn, f) != fn) return 1; fclose(f);
    const double T = atof(argv[2]); std::mt19937_64 rng(argc > 3 ? strtoull(argv[3], 0, 10) : 1);
    uint64_t seeds = 0, mutants = 0, accepted = 0, rejected = 0; const double t0 = now();
    while (now() - t0 < T) {
        uint32_t n = (uint32_t)(rng() % 81921); if (rng() % 8 == 0) n = (uint32_t)(rng() % 300); if (n > fn) n = (uint32_t)fn; if (!n) n = 1;
        const size_t off = fn > n ? rng() % (fn - n) : 0;
        const int lvl = 11 + (int)(rng() % 8);
        std::vector<uint8_t> z(aceapex_compress_bound(n));
        const int64_t zs = aceapex_compress(in.data() + off, n, z.data(), z.size(), lvl, 1);
        if (zs <= 0) { printf("COMPRESS FAILED n=%u lvl=%d\n", n, lvl); return 2; }
        z.resize((size_t)zs);
        {   // the valid archive on exact buffers, whole and block by block
            std::unique_ptr<uint8_t[]> s(new uint8_t[z.size()]), d(new uint8_t[n]); memcpy(s.get(), z.data(), z.size());
            axf_arc_t a; if (axf_open(s.get(), z.size(), &a) != 0) { printf("VALID ARCHIVE REFUSED n=%u lvl=%d\n", n, lvl); return 2; }
#ifdef AXF_NEGATIVE
            // negative control: the output buffer one byte short of the block -> the decoder's last write must be an ASan report
            for (uint32_t b = 0; b < a.nb; b++) { const uint32_t k = axf_block_len(&a, b); std::unique_ptr<uint8_t[]> o(new uint8_t[k - 1]); axf_dec_block_of(&a, b, o.get()); }
            printf("NEGATIVE CONTROL NOT CAUGHT\n"); return 4;
#endif
            for (uint32_t b = 0; b < a.nb; b++) { const uint32_t k = axf_block_len(&a, b); std::unique_ptr<uint8_t[]> o(new uint8_t[k]);
                if (axf_dec_block_of(&a, b, o.get()) != k || memcmp(o.get(), in.data() + off + (size_t)b * a.bs, k)) { printf("DECODER WRONG on a valid block n=%u lvl=%d b=%u\n", n, lvl, b); return 2; } }
            if (aceapex_decompress_mt(s.get(), z.size(), d.get(), n, 1) != (int64_t)n || memcmp(d.get(), in.data() + off, n)) { printf("LIBRARY WRONG n=%u lvl=%d\n", n, lvl); return 2; }
            seeds++;
        }
        for (int m = 0; m < 64; m++) {
            std::vector<uint8_t> x = z; uint32_t nn = n; const int kind = (int)(rng() % 9);
            if (kind == 0) { int k = 1 + rng() % 8; while (k--) x[rng() % x.size()] ^= (uint8_t)(1u << (rng() % 8)); }
            else if (kind == 1) { const size_t p = rng() % x.size(), L = std::min<size_t>(x.size() - p, 1 + rng() % 64); for (size_t k = 0; k < L; k++) x[p + k] = (uint8_t)rng(); }
            else if (kind == 2) x.resize(rng() % (x.size() + 1));
            else if (kind == 3) { size_t L = 1 + rng() % 300; while (L--) x.push_back((uint8_t)rng()); }
            else if (kind == 4) { const size_t fld[] = {12, 20, 24, 36, 40, 44, 52, 60, 68, 76}; const size_t p = fld[rng() % 10]; if (p + 8 <= x.size()) { uint64_t v = rng(); if (rng() % 2) v %= 1u << (rng() % 24); if (rng() % 3 == 0) v = (uint32_t)v; memcpy(x.data() + p, &v, rng() % 2 ? 4 : 8); } }
            else if (kind == 5) nn = (rng() % 2) ? (uint32_t)(rng() % (n + 200)) : n + (uint32_t)(rng() % 3) - 1;
            else if (kind == 6) { x.resize(68 + rng() % 4096); for (auto& b : x) b = (uint8_t)rng(); memcpy(x.data(), "ACEPX2\0\0", 8); const uint32_t v3 = 3, pr = 1, mm = 5 + (uint32_t)(rng() % 12); memcpy(x.data() + 8, &v3, 4); memcpy(x.data() + 36, &pr, 4); memcpy(x.data() + 40, &mm, 4); nn = (uint32_t)(rng() % 20000); }
            else if (kind == 7) { const uint32_t nb = (uint32_t)((n + (1u << 20) - 1) >> 20); const size_t tp = 68 + 8ull * (nb + 1) + 12; if (tp + 4 <= x.size()) { const size_t i = rng() % 64; if (tp + 4 * i + 4 <= x.size()) { uint32_t t; memcpy(&t, x.data() + tp + 4 * i, 4); t ^= (uint32_t)(rng() % 2 ? 63u : 63u << 6) | (uint32_t)((rng() & 0xfffff) << 12) * (uint32_t)(rng() % 2); memcpy(x.data() + tp + 4 * i, &t, 4); } } }
            else { const size_t tp = 68 + 8; if (tp + 8 <= x.size()) { uint64_t v; memcpy(&v, x.data() + tp, 8); v += (rng() % 2) ? 1 : (uint64_t)-1; memcpy(x.data() + tp, &v, 8); } }   // block table off by one
            std::unique_ptr<uint8_t[]> s(new uint8_t[x.size() ? x.size() : 1]), d(new uint8_t[nn ? nn : 1]); if (!x.empty()) memcpy(s.get(), x.data(), x.size());
            mutants++;
            axf_arc_t a;
            if (axf_open(s.get(), x.size(), &a) != 0) { rejected++; continue; }
            if (a.orig != nn) { rejected++; continue; }   // the caller's buffer is nn bytes: a different size is a mismatch, not a decode
            bool ok = true;
            for (uint32_t b = 0; b < a.nb && ok; b++) ok = axf_dec_block_of(&a, b, d.get() + (size_t)b * a.bs) != 0;
            if (ok) accepted++; else rejected++;
        }
    }
    printf("head_fast_fuzz\tpass\t%llu valid archives (levels 11..18, exact buffers, block decoder and library), %llu mutants: %llu rejected, %llu decoded to the declared size; no ASan/UBSan report in %.0f s\n",
           (unsigned long long)seeds, (unsigned long long)mutants, (unsigned long long)rejected, (unsigned long long)accepted, now() - t0);
    return 0;
}
