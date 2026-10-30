// fair.cpp - one harness for three engines inside the same container: independent blocks, the same block
// size, the same decode pool (thread t takes a contiguous range of blocks, the last block of a range through a
// private buffer), the same timing (median of R, output compared byte for byte). Engines:
//   axf       ours (axf.h), level 3
//   misa77 -1 MIT, (c) 2026 Shreyas Ghildiyal, unmodified objects from the lzbench tree
//   lz4       BSD-2, (c) Yann Collet, LZ4_compress_default / LZ4_decompress_safe, from the lzbench tree
// Build: see build line in f1-fair-*.log. Usage: fair file [block=1048576] [reps=7]
#include "axf.h"
#include <misa77/misa77.h>
#include "lz4.h"
#include <functional>
#include <string>

struct Engine { std::string name; std::function<size_t(const uint8_t*, uint32_t, std::vector<uint8_t>&)> enc;
                std::function<bool(const uint8_t*, size_t, uint8_t*, uint32_t, size_t)> dec; size_t slack; };

int main(int argc, char** argv) {
    if (argc < 2) { fprintf(stderr, "usage: %s file [block=1048576] [reps=7]\n", argv[0]); return 1; }
    if (const char* lam = getenv("AXF_LAMBDA")) g_lambda = atof(lam);
    const size_t BS = argc > 2 ? strtoull(argv[2], 0, 10) : 1048576; const int R = argc > 3 ? atoi(argv[3]) : 7;
    FILE* f = fopen(argv[1], "rb"); if (!f) { perror(argv[1]); return 1; } fseek(f, 0, SEEK_END); size_t n = ftell(f); fseek(f, 0, SEEK_SET);
    std::vector<uint8_t> in(n); if (fread(in.data(), 1, n, f) != n) return 1; fclose(f);
    const size_t nb = (n + BS - 1) / BS;
    misa77::config mcfg(1);
    std::vector<Engine> E = {
        {"axf L4 (ours)", [](const uint8_t* s, uint32_t k, std::vector<uint8_t>& o) { static thread_local Enc e; o.resize(enc_bound(k) + SLACK); size_t r = enc_block(s, k, o.data(), 4, e); return r; },
                          [](const uint8_t* c, size_t cs, uint8_t* d, uint32_t k, size_t) { return dec_block(c, cs, d, k) == k; }, SLACK},
        {"axf L3 (ours)", [](const uint8_t* s, uint32_t k, std::vector<uint8_t>& o) { static thread_local Enc e; o.resize(enc_bound(k) + SLACK); size_t r = enc_block(s, k, o.data(), 3, e); return r; },
                          [](const uint8_t* c, size_t cs, uint8_t* d, uint32_t k, size_t) { return dec_block(c, cs, d, k) == k; }, SLACK},
        {"misa77 -2", [&](const uint8_t* s, uint32_t k, std::vector<uint8_t>& o) { misa77::config c2(2); o.resize(misa77::compress_bound(k, c2)); return (size_t)misa77::compress(s, k, o.data(), o.size(), c2); },
                      [](const uint8_t* c, size_t cs, uint8_t* d, uint32_t k, size_t cap) { return misa77::decompress(c, cs, d, cap) == k; },
                      (size_t)(misa77::decompressed_buffer_bound(BS) - BS)},
        {"misa77 -1", [&](const uint8_t* s, uint32_t k, std::vector<uint8_t>& o) { o.resize(misa77::compress_bound(k, mcfg)); return (size_t)misa77::compress(s, k, o.data(), o.size(), mcfg); },
                      [](const uint8_t* c, size_t cs, uint8_t* d, uint32_t k, size_t cap) { return misa77::decompress(c, cs, d, cap) == k; },
                      (size_t)(misa77::decompressed_buffer_bound(BS) - BS)},
        {"lz4 1.10.0", [](const uint8_t* s, uint32_t k, std::vector<uint8_t>& o) { o.resize(LZ4_compressBound(k)); return (size_t)LZ4_compress_default((const char*)s, (char*)o.data(), k, (int)o.size()); },
                       [](const uint8_t* c, size_t cs, uint8_t* d, uint32_t k, size_t) { return LZ4_decompress_safe((const char*)c, (char*)d, (int)cs, (int)k) == (int)k; }, 64},
    };
    printf("%s: %zu B, %zu blocks of %zu, decode pool: contiguous ranges, median of %d\n", argv[1], n, nb, BS, R);
    printf("%-14s %8s %11s %11s\n", "engine", "ratio", "1T MB/s", "8T MB/s");
    for (auto& eng : E) {
        std::vector<std::vector<uint8_t>> cb(nb); std::vector<size_t> real(nb);
        { std::vector<std::thread> th; for (int t = 0; t < 16; t++) th.emplace_back([&, t] { for (size_t b = t; b < nb; b += 16) { size_t o = b * BS; uint32_t k = (uint32_t)std::min(BS, n - o);
              real[b] = eng.enc(in.data() + o, k, cb[b]); cb[b].resize(real[b] + 256); } }); for (auto& x : th) x.join(); }
        size_t csz = 8 * nb + 8; for (size_t r : real) csz += r;
        std::vector<uint8_t> out(n + eng.slack + 256);
        double res[2]; int ti = 0; bool allok = true;
        for (int T : {1, 8}) {
            std::vector<double> ts; bool ok = true;
            for (int r = 0; r < R; r++) {
                memset(out.data(), 0, n); double t0 = now(); std::vector<std::thread> th;
                for (int t = 0; t < T; t++) th.emplace_back([&, t] { size_t b0 = nb * t / T, b1 = nb * (t + 1) / T; std::vector<uint8_t> priv;
                    for (size_t b = b0; b < b1; b++) { size_t o = b * BS; uint32_t k = (uint32_t)std::min(BS, n - o);
                        if (b + 1 == b1 && b + 1 < nb) { priv.resize(k + eng.slack + 256); if (!eng.dec(cb[b].data(), real[b], priv.data(), k, priv.size())) ok = false; memcpy(out.data() + o, priv.data(), k); }
                        else if (!eng.dec(cb[b].data(), real[b], out.data() + o, k, n + eng.slack - o)) ok = false; } });
                for (auto& x : th) x.join();
                ts.push_back(now() - t0);
            }
            ok = ok && memcmp(out.data(), in.data(), n) == 0; allok = allok && ok;
            std::sort(ts.begin(), ts.end()); res[ti++] = n / ts[ts.size() / 2] / 1e6;
        }
        printf("%-14s %7.2f%% %11.0f %11.0f  %s\n", eng.name.c_str(), 100.0 * csz / n, res[0], res[1], allok ? "bit-perfect" : "MISMATCH");
    }
    return 0;
}
