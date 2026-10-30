// Library calls from several threads at once (lzbench -T, a server): each worker compresses and
// decompresses its own inputs through aceapex_compress / aceapex_decompress_mt / region. Before
// 2.2.0 the block size, the decode error flag and the DNA hint were process globals: two calls at
// once could write each other's block size into the header (2.1.0: wrong bytes) or see each other's
// errors. Inputs: DNA, text-like and random buffers of 1 B .. 5 MiB, levels 1-3. Prints one claim line.
// Build: g++ -std=c++17 -O2 -Isrc scripts/api_concurrent.cpp src/aceapex_api.cpp -lzstd -lpthread
#include "aceapex.h"
#include <atomic>
#include <cstdio>
#include <cstring>
#include <thread>
#include <vector>
int main() {
    const int NW = 4, NJ = 96;
    std::atomic<int> next{0}, bad{0}, runs{0};
    auto work = [&]() {
        for (int j; (j = next++) < NJ;) {
            uint32_t r = 2026u + 7919u * (uint32_t)j;
            auto rnd = [&]() { r = r * 1103515245u + 12345u; return (uint8_t)(r >> 16); };
            const size_t sizes[] = {1, 767, 65537, 300000, 1048577, 2500000, 5242881};
            const size_t n = sizes[j % 7];
            std::vector<unsigned char> in(n);
            const int kind = (j / 7) % 3;
            for (size_t i = 0; i < n; i++)
                in[i] = kind == 0 ? (uint8_t)("ACGTACGTTGCAN"[(i * 7 + (i >> 9) + rnd() % 3) % 13])
                      : kind == 1 ? (uint8_t)("the quick brown fox "[(i + (i >> 11) * 3) % 20])
                      : rnd();
            const int lvl = 1 + j % 3, dt = 1 + (j & 1);
            std::vector<unsigned char> z(aceapex_compress_bound(n)), o(n + 64);
            int64_t zs = aceapex_compress(in.data(), n, z.data(), z.size(), lvl, 1 + (j % 2));
            int64_t d = zs > 0 ? aceapex_decompress_mt(z.data(), (size_t)zs, o.data(), o.size(), dt) : -99;
            bool ok = d == (int64_t)n && !memcmp(o.data(), in.data(), n);
            if (ok && n > 4096) {                                   // one region in the middle
                size_t off = n / 3, len = 4096; std::vector<unsigned char> g(len);
                int64_t rr = aceapex_decompress_region(z.data(), (size_t)zs, g.data(), len, off, len);
                ok = rr == (int64_t)len && !memcmp(g.data(), in.data() + off, len);
            }
            runs++; if (!ok) bad++;
        }
    };
    std::vector<std::thread> th; for (int t = 0; t < NW; t++) th.emplace_back(work);
    for (auto& t : th) t.join();
    printf("head_api_concurrent\t%s\t%d jobs on %d threads at once (DNA/text/random, 1 B..5 MiB, levels 1-3, full + region): %d failed\n",
           bad ? "fail" : "pass", runs.load(), NW, bad.load());
    return bad ? 1 : 0;
}
