// fuzz_safe.cpp - #D1-2: dec_block_safe under ASan/UBSan. Seeds: valid axf blocks of random slices of a file (random
// length 0..80 KiB, levels 3 and 4). Each round: (1) the valid block decodes to the slice with both decoders;
// (2) mutants (byte flips, runs of random bytes, truncation, extension, header fields, a wrong output size, pure
// garbage) go to the safe decoder with src and dst on exact-size heap buffers, so any read or write outside them
// is an ASan report. A mutant that decodes must give exactly n bytes (return n) - its content is not checked.
// Build: g++ -O1 -g -std=c++20 -fsanitize=address,undefined -fno-sanitize-recover=all [-DMIN_MATCH=16u] -o fuzz_safe fuzz_safe.cpp
// Usage: fuzz_safe file seconds [seed]
#include "axf.h"
#include <random>
#include <memory>

int main(int argc, char** argv) {
    if (argc < 3) { fprintf(stderr, "usage: %s file seconds [seed]\n", argv[0]); return 1; }
    FILE* f = fopen(argv[1], "rb"); if (!f) { perror(argv[1]); return 1; }
    fseek(f, 0, SEEK_END); size_t fn = ftell(f); fseek(f, 0, SEEK_SET);
    std::vector<uint8_t> in(fn); if (fread(in.data(), 1, fn, f) != fn) return 1; fclose(f);
    const double T = atof(argv[2]); std::mt19937_64 rng(argc > 3 ? strtoull(argv[3], 0, 10) : 1);
    g_lambda = 2.0; Enc e; uint64_t seeds = 0, mutants = 0, accepted = 0, rejected = 0; const double t0 = now();
    while (now() - t0 < T) {
        uint32_t n = (uint32_t)(rng() % 81921); if (rng() % 8 == 0) n = (uint32_t)(rng() % 300);
        size_t off = fn > n ? rng() % (fn - n) : 0; if (n > fn) n = (uint32_t)fn;
        std::vector<uint8_t> blk(enc_bound(n) + SLACK);
        size_t c = enc_block(in.data() + off, n, blk.data(), (rng() % 4 == 0) ? 4 : 3, e); blk.resize(c);
        {   // valid: both decoders, exact buffers for the safe one
            std::unique_ptr<uint8_t[]> s(new uint8_t[c]), d(new uint8_t[n ? n : 1]); memcpy(s.get(), blk.data(), c);
            if (dec_block_safe(s.get(), c, d.get(), n) != n || memcmp(d.get(), in.data() + off, n)) { printf("SAFE DECODER WRONG on a valid block n=%u\n", n); return 2; }
            std::vector<uint8_t> o(n + SLACK); std::vector<uint8_t> sp(c + SLACK); memcpy(sp.data(), blk.data(), c);
            if (dec_block(sp.data(), c, o.data(), n) != n || memcmp(o.data(), in.data() + off, n)) { printf("FAST DECODER WRONG on a valid block n=%u\n", n); return 2; }
            seeds++;
        }
        for (int m = 0; m < 64; m++) {
            std::vector<uint8_t> x = blk; uint32_t nn = n; const int kind = (int)(rng() % 8);
            if (kind == 0 && !x.empty()) { int k = 1 + rng() % 8; while (k--) x[rng() % x.size()] ^= (uint8_t)(1u << (rng() % 8)); }
            else if (kind == 1 && !x.empty()) { size_t p = rng() % x.size(), L = std::min<size_t>(x.size() - p, 1 + rng() % 64); for (size_t k = 0; k < L; k++) x[p + k] = (uint8_t)rng(); }
            else if (kind == 2) x.resize(rng() % (x.size() + 1));
            else if (kind == 3) { size_t L = 1 + rng() % 300; while (L--) x.push_back((uint8_t)rng()); }
            else if (kind == 4 && x.size() >= 12) { uint32_t v = (uint32_t)rng(); if (rng() % 2) v %= 1u << (rng() % 20); memcpy(x.data() + 4 * (rng() % 3), &v, 4); }
            else if (kind == 5) nn = (rng() % 2) ? (uint32_t)(rng() % (n + 200)) : n + (uint32_t)(rng() % 3) - 1;
            else if (kind == 6) { x.resize(rng() % 4096); for (auto& b : x) b = (uint8_t)rng(); if (x.size() >= 12) { uint32_t v = (uint32_t)(rng() % 512); memcpy(x.data(), &v, 4); v = (uint32_t)(rng() % 64); memcpy(x.data() + 4, &v, 4); } nn = (uint32_t)(rng() % 20000); }
            else if (x.size() > 12) { size_t ntok = rd32(x.data()); if (ntok) { size_t i = rng() % ntok; if (12 + 4 * i + 4 <= x.size()) { uint32_t t = rd32(x.data() + 12 + 4 * i); t ^= (uint32_t)(rng() % 2 ? 63u : 63u << 6) | (uint32_t)((rng() & 0xfffff) << 12) * (rng() % 2); memcpy(x.data() + 12 + 4 * i, &t, 4); } } }
            std::unique_ptr<uint8_t[]> s(new uint8_t[x.size() ? x.size() : 1]), d(new uint8_t[nn ? nn : 1]); if (!x.empty()) memcpy(s.get(), x.data(), x.size());
            uint32_t r = dec_block_safe(s.get(), x.size(), d.get(), nn); mutants++;
            if (r == 0) rejected++; else if (r == nn) accepted++; else { printf("BAD RETURN %u for n=%u\n", r, nn); return 3; }
        }
    }
    printf("MIN_MATCH %u: %llu valid blocks (both decoders exact), %llu mutants: %llu rejected, %llu decoded to n bytes; no ASan/UBSan report in %.0f s\n",
           (unsigned)MIN_MATCH, (unsigned long long)seeds, (unsigned long long)mutants, (unsigned long long)rejected, (unsigned long long)accepted, now() - t0);
    return 0;
}
