// Library round-trip: aceapex_compress -> aceapex_decompress_mt on sizes that cross the
// adaptive block-size boundaries, at several thread counts. Prints one claim line.
// Build: g++ -std=c++17 -O2 -Isrc scripts/api_roundtrip.cpp src/aceapex_api.cpp -lzstd -lpthread
#include "aceapex.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
static uint32_t rng = 2026; static uint8_t rnd() { rng = rng * 1103515245u + 12345u; return (uint8_t)(rng >> 16); }
int main() {
    const size_t sizes[] = {1, 100, 767, 4096, 65537, 262143, 262144, 262145, 300000, 700000, 1048577, 2500000, 4200000};
    int fails = 0, runs = 0;
    for (size_t n : sizes) for (int kind = 0; kind < 2; kind++) {
        std::vector<unsigned char> in(n);
        for (size_t i = 0; i < n; i++) in[i] = kind ? rnd() : (uint8_t)("ACGTNacgt"[(i * 7 + (i >> 9)) % 9]);
        for (int lvl = 1; lvl <= 2; lvl++) for (int et : {1, 3, 8}) {
            std::vector<unsigned char> z(aceapex_compress_bound(n));
            int64_t zs = aceapex_compress(in.data(), n, z.data(), z.size(), lvl, et);
            for (int dt : {1, 2, 8}) {
                runs++;
                std::vector<unsigned char> o(n, 0);
                int64_t w = zs > 0 ? aceapex_decompress_mt(z.data(), (size_t)zs, o.data(), n, dt) : -9;
                if (w != (int64_t)n || memcmp(o.data(), in.data(), n) != 0) {
                    fails++; if (fails <= 3) fprintf(stderr, "FAIL n=%zu kind=%d lvl=%d encT=%d decT=%d rc=%lld\n", n, kind, lvl, et, dt, (long long)w);
                }
            }
        }
    }
    printf("head_api_roundtrip\t%s\t%d/%d library compress->decompress_mt round-trips bit-perfect (13 sizes x 2 kinds x 2 levels x 3x3 threads)\n",
           fails ? "fail" : "pass", runs - fails, runs);
    return fails ? 1 : 0;
}
