// axf.cpp - standalone bench of the axf engine (axf.h)
#include "axf.h"
int main(int argc, char** argv) {
    if (argc < 2) { fprintf(stderr, "usage: %s file [block=262144] [level=1] [reps=9]\n", argv[0]); return 1; }
    if (const char* lam = getenv("AXF_LAMBDA")) g_lambda = atof(lam);
    const size_t BS = argc > 2 ? strtoull(argv[2], 0, 10) : 262144; const int LV = argc > 3 ? atoi(argv[3]) : 1, R = argc > 4 ? atoi(argv[4]) : 9;
    if (BS > D_MAX + MIN_DIST + 1) { fprintf(stderr, "block too large for 20-bit distances\n"); return 1; }
    FILE* f = fopen(argv[1], "rb"); if (!f) { perror(argv[1]); return 1; } fseek(f, 0, SEEK_END); size_t n = ftell(f); fseek(f, 0, SEEK_SET);
    std::vector<uint8_t> in(n + 64); if (fread(in.data(), 1, n, f) != n) return 1; fclose(f); in.resize(n); in.reserve(n + 64);
    const size_t nb = (n + BS - 1) / BS; std::vector<std::vector<uint8_t>> cb(nb); std::vector<size_t> real(nb);
    double tc = now();
    { std::vector<std::thread> th; const int T = 16;
      for (int t = 0; t < T; t++) th.emplace_back([&, t] { Enc e; for (size_t b = t; b < nb; b += T) { size_t o = b * BS; uint32_t k = (uint32_t)std::min(BS, n - o);
          cb[b].resize(enc_bound(k)); real[b] = enc_block(in.data() + o, k, cb[b].data(), LV, e); cb[b].resize(real[b] + SLACK); } });   // read padding per block
      for (auto& x : th) x.join(); }
    tc = now() - tc;
    size_t csz = 8 * nb + 8; for (size_t r : real) csz += r;
    std::vector<uint8_t> out(n + SLACK);
    printf("%s: %zu B, %zu blocks of %zu, axf MIN_MATCH %u level %d: %zu B = %.2f %% (encode 16 threads %.2f s = %.0f MB/s); %.0f tokens/block, literals %.1f %%\n", argv[1], n, nb, BS, MIN_MATCH, LV, csz, 100.0 * csz / n, tc, n / tc / 1e6, (double)g_tok / nb, 100.0 * g_lit / n);
    for (int T : {1, 2, 4, 8, 16}) {
        std::vector<double> ts; bool ok = true;
        for (int r = 0; r < R; r++) {
            memset(out.data(), 0, n); double t0 = now(); std::vector<std::thread> th;
            for (int t = 0; t < T; t++) th.emplace_back([&, t] { size_t b0 = nb * t / T, b1 = nb * (t + 1) / T; std::vector<uint8_t> priv;
                for (size_t b = b0; b < b1; b++) { size_t o = b * BS; uint32_t k = (uint32_t)std::min(BS, n - o);
                    if (b + 1 == b1 && b + 1 < nb) { priv.resize(k + SLACK); if (dec_block(cb[b].data(), real[b], priv.data(), k) != k) ok = false; memcpy(out.data() + o, priv.data(), k); }
                    else if (dec_block(cb[b].data(), real[b], out.data() + o, k) != k) ok = false; } });
            for (auto& x : th) x.join();
            ts.push_back(now() - t0);
        }
        ok = ok && memcmp(out.data(), in.data(), n) == 0; std::sort(ts.begin(), ts.end()); double m = ts[ts.size() / 2];
        printf("decode %2d thread(s): %.4f s = %6.0f MB/s  %s\n", T, m, n / m / 1e6, ok ? "bit-perfect" : "MISMATCH");
    }
    return 0;
}
