// Thread budget of the library (2.2.1): with threads=1 neither aceapex_compress nor aceapex_decompress_mt
// nor a region read starts a thread. Built as one translation unit with the library, so it reads the
// codec's own counter of started threads (g_ax_spawned, incremented by every ax_thread). Inputs: DNA-like
// and text-like buffers of 3 MiB, levels 1-3, profiles default / interactive / rANS tokens / open. A
// run with threads=4 must start threads (the counter works). Before 2.2.1 the encoder's entropy stage
// started 3 + up to CPU-count threads at threads=1. Prints one claim line.
// Build: g++ -std=c++17 -O2 -Isrc scripts/api_threads.cpp -lzstd -lpthread
#include "aceapex_api.cpp"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
int main() {
    const size_t n = 3u << 20;
    std::vector<unsigned char> dna(n), txt(n);
    uint32_t r = 2026;
    for (size_t i = 0; i < n; i++) { r = r * 1103515245u + 12345u;
        dna[i] = (uint8_t)("ACGTACGTTGCAacgtN"[((i * 7) ^ (r >> 20)) % 17]);
        txt[i] = (uint8_t)("the quick brown fox jumps over the lazy dog "[(i + (r >> 28)) % 44]); }
    const char* prof[][3] = { {nullptr, nullptr, nullptr},
                              {"ACEAPEX_BS=16384", "LIT_CHUNK=65536", "FSE_CHUNK=4096"},
                              {"AX_TOK=rans", nullptr, nullptr},
                              {"AX_PROFILE=open", nullptr, nullptr} };
    int runs = 0, bad = 0; long leaked = 0;
    for (auto& pr : prof) {
        for (const char* e : pr) if (e) putenv((char*)e);
        for (int k = 0; k < 2; k++) for (int lvl = 1; lvl <= 3; lvl++) {
            const std::vector<unsigned char>& in = k ? txt : dna;
            std::vector<unsigned char> z(aceapex_compress_bound(n)), o(n + 64), g(4096);
            long s0 = g_ax_spawned.load();
            int64_t zs = aceapex_compress(in.data(), n, z.data(), z.size(), lvl, 1);
            int64_t d = zs > 0 ? aceapex_decompress_mt(z.data(), (size_t)zs, o.data(), o.size(), 1) : -1;
            long s1 = g_ax_spawned.load();
            int64_t rr = zs > 0 ? aceapex_decompress_region(z.data(), (size_t)zs, g.data(), g.size(), n / 3, 4096) : -1;
            long s2 = g_ax_spawned.load();
            runs++;
            bool ok = d == (int64_t)n && !memcmp(o.data(), in.data(), n) && rr == 4096 && !memcmp(g.data(), in.data() + n / 3, 4096);
            if (!ok || s1 != s0 || s2 != s1) { bad++; leaked += s2 - s0; }
        }
        for (const char* e : pr) if (e) { std::string k(e); unsetenv(k.substr(0, k.find('=')).c_str()); }
    }
    std::vector<unsigned char> z(aceapex_compress_bound(n));
    long c0 = g_ax_spawned.load(); aceapex_compress(txt.data(), n, z.data(), z.size(), 2, 4); long c1 = g_ax_spawned.load();
    const bool counter_ok = c1 > c0;
    printf("head_enc_threads\t%s\t%d round-trips at threads=1 (4 profiles x DNA/text x levels 1-3, compress + decompress + region): %d with a thread started or a wrong result (%ld threads); threads=4 started %ld\n",
           (bad == 0 && counter_ok) ? "pass" : "fail", runs, bad, leaked, c1 - c0);
    return (bad == 0 && counter_ok) ? 0 : 1;
}
