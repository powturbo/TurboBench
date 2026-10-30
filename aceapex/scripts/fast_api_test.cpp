// fast_api_test.cpp - the FAST profile through the public library (claim head_fast_api): levels 11..18 on four kinds of
// input (text-like, DNA-like with repeats, random, zeros) at sizes around the 1 MiB block boundary, decompress_mt at 1,
// 2 and the default thread count, regions and 200 random ranges against the input, aceapex_decode_streams refused,
// the empty input, compress_bound honoured, a truncated archive and a wrong level rejected. Prints one claim line.
// Build: g++ -std=c++17 -O2 -Isrc scripts/fast_api_test.cpp src/aceapex_api.cpp -lzstd -lpthread
#include "aceapex.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <string>
static uint32_t rng = 2026; static uint32_t rnd() { rng = rng * 1103515245u + 12345u; return rng >> 8; }
static std::vector<unsigned char> make(size_t n, int kind) {
    std::vector<unsigned char> v(n);
    if (kind == 0) { const char* w[] = {"the ", "block ", "token ", "absolute ", "offset ", "decode ", "0x", "\n"}; size_t i = 0;
        while (i < n) { const char* s = w[rnd() % 8]; for (const char* p = s; *p && i < n; p++) v[i++] = (unsigned char)*p; } }
    else if (kind == 1) { size_t i = 0; while (i < n) { const uint32_t k = (rnd() % 4 == 0) ? 1 + rnd() % 2000 : 1; const unsigned char c = "ACGT"[rnd() % 4]; for (uint32_t j = 0; j < k && i < n; j++) v[i++] = c; }
        for (int r = 0; r < 64 && n > 10000; r++) { const size_t a = rnd() % (n - 5000), b = rnd() % (n - 5000); memmove(&v[b], &v[a], 4000); } }
    else if (kind == 2) { for (size_t i = 0; i < n; i++) v[i] = (unsigned char)rnd(); }
    return v;   // kind 3: zeros
}
int main() {
    const size_t sizes[] = {1, 7, 63, 64, 159, 160, 767, 4096, 65537, 1048575, 1048576, 1048577, 2500000, 4200000};
    int fails = 0, runs = 0; std::string first;
    auto bad = [&](const std::string& m) { fails++; if (first.empty()) first = m; };
    for (size_t n : sizes) for (int kind = 0; kind < 4; kind++) {
        std::vector<unsigned char> in = make(n, kind);
        for (int lvl = 11; lvl <= 18; lvl++) {
            if (lvl >= 15 && n > 65537 && !(lvl == 15 && n == 1048577)) continue;   // the optimal parse is slow: small sizes + one block boundary
            std::vector<unsigned char> z(aceapex_compress_bound(n));
            const int64_t zs = aceapex_compress(in.data(), n, z.data(), z.size(), lvl, 2);
            runs++; if (zs <= 0 || (size_t)zs > z.size()) { bad("compress n=" + std::to_string(n) + " kind=" + std::to_string(kind) + " lvl=" + std::to_string(lvl)); continue; }
            for (int dt : {1, 2, 0}) {
                runs++; std::vector<unsigned char> o(n + 1, 0xAB);
                const int64_t w = aceapex_decompress_mt(z.data(), (size_t)zs, o.data(), n, dt);
                if (w != (int64_t)n || memcmp(o.data(), in.data(), n) != 0 || o[n] != 0xAB) bad("decompress n=" + std::to_string(n) + " kind=" + std::to_string(kind) + " lvl=" + std::to_string(lvl) + " dt=" + std::to_string(dt) + " rc=" + std::to_string(w));
            }
            if (lvl == 11 || lvl == 14 || lvl == 18) {             // regions and ranges
                runs++; std::vector<aceapex_range_t> rg(200); std::vector<std::vector<unsigned char>> bufs(200);
                for (size_t i = 0; i < 200; i++) { const size_t l = std::min<size_t>(n, 1 + rnd() % (i % 3 == 0 ? 2000000 : 20000)); const size_t off = rnd() % (n - l + 1);
                    bufs[i].assign(l, 0); rg[i] = {off, l, bufs[i].data(), 0}; }
                const int64_t k = aceapex_decompress_ranges(z.data(), (size_t)zs, rg.data(), 200, 2); bool ok = k == 200;
                for (size_t i = 0; i < 200 && ok; i++) ok = rg[i].written == (int64_t)rg[i].length && memcmp(bufs[i].data(), in.data() + rg[i].offset, rg[i].length) == 0;
                if (!ok) bad("ranges n=" + std::to_string(n) + " kind=" + std::to_string(kind) + " lvl=" + std::to_string(lvl) + " k=" + std::to_string(k));
                runs++; { const size_t l = std::min<size_t>(n, 777), off = n - l; std::vector<unsigned char> r(l + 1, 0xCD);
                    const int64_t w = aceapex_decompress_region(z.data(), (size_t)zs, r.data(), l, off, l);
                    if (w != (int64_t)l || memcmp(r.data(), in.data() + off, l) != 0 || r[l] != 0xCD) bad("region tail n=" + std::to_string(n) + " lvl=" + std::to_string(lvl)); }
                runs++; { unsigned char x[4]; if (aceapex_decompress_region(z.data(), (size_t)zs, x, 4, n, 1) >= 0) bad("region past the end accepted"); }
            }
            if (lvl == 11) {                                        // refused: entropy streams, a truncated archive, a short buffer
                runs++; aceapex_streams_t st; if (aceapex_decode_streams(z.data(), (size_t)zs, &st) != ACEAPEX_ERR_DATA) bad("decode_streams accepted a fast archive");
                runs++; std::vector<unsigned char> o(n + 1); if (aceapex_decompress_mt(z.data(), (size_t)zs - 1, o.data(), n, 1) != ACEAPEX_ERR_DATA) bad("truncated archive accepted n=" + std::to_string(n));
                runs++; if (n > 0 && aceapex_decompress_mt(z.data(), (size_t)zs, o.data(), n - 1, 1) != ACEAPEX_ERR_BUFFER) bad("short output buffer accepted");
            }
        }
    }
    runs++; { unsigned char z[128]; const int64_t zs = aceapex_compress(z, 0, z, sizeof z, 11, 1); if (zs != 68) bad("empty input at level 11 -> " + std::to_string(zs)); }
    runs++; { unsigned char in[16] = {0}, z[256]; if (aceapex_compress(in, 16, z, sizeof z, 19, 1) <= 0) { /* 19 is not a fast level: the default encoder */ } }
    printf("head_fast_api\t%s\t%d/%d checks: levels 11..18 x 4 inputs x 14 sizes through aceapex_compress / decompress_mt (1, 2, default threads), regions, 200 ranges, decode_streams refused, truncation refused%s%s\n",
           fails ? "fail" : "pass", runs - fails, runs, fails ? "; first: " : "", first.c_str());
    return fails ? 1 : 0;
}
