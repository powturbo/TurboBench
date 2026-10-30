// gpu_stress_emu.cpp - T-H4 on the CPU: the same 10 000 corrupt copies scripts/gpu_h100_tests.cu `stress` makes
// (same generator, seed 4242, same seven kinds in turn) of the same archive (the open profile is byte-deterministic),
// each through the GPU library's plan (agp::build, as aceapex_gpu_plan_create) and the CPU executor of
// scripts/gpu_plan_emu.cpp (the jobs as the device runs them), twice - status only, then with the XXH3 of the output
// against the header (ACEAPEX_GPU_VERIFY_XXH3). Besides the outcome it counts the device's loop trips the plan
// implies: per rANS job ceil(n/32) warp groups (k_rans: one warp, one group of 32 symbols per trip, no early exit),
// per open chunk ceil(ncse/256) + ceil(ngap/256) block trips (k_open_cg). A copy whose trips exceed HANG_TRIPS
// (2^22: the intact archive needs at most 2^9 per job) is what the device turns into a multi-second kernel - a hang
// for the T-H4 watchdog. Such copies are saved (archive + kind + index) when a directory is given.
// Usage: gpu_stress_emu <archive.open.aet> <original> [n=10000] [save_dir]  |  gpu_stress_emu --check <archive>
//        gpu_stress_emu --decode <archive> <original>   (the GPU plan + executor, one archive)
// Last line: STRESSEMU <tab> n refused caught harmless silent silent_xxh3 hangs ok|FAILED
// Build: g++ -std=c++17 -O2 -Isrc scripts/gpu_stress_emu.cpp src/aceapex_api.cpp -lzstd -lpthread
#define main gpu_plan_emu_main
#include "gpu_plan_emu.cpp"
#undef main
#include <sys/stat.h>
int main(int argc, char** argv) {
    if (argc == 3 && !strcmp(argv[1], "--check")) {                   // one archive: does the plan accept it, and how long its longest loop
        FILE* f = fopen(argv[2], "rb"); if (!f) { perror(argv[2]); return 1; } std::vector<uint8_t> a; fseek(f, 0, SEEK_END); a.resize((size_t)ftell(f)); fseek(f, 0, SEEK_SET);
        if (fread(a.data(), 1, a.size(), f) != a.size()) return 1; fclose(f);
        agp::Plan P; if (agp::build(a.data(), a.size(), P, nvt_cpu)) { printf("refused\n"); return 0; }
        uint64_t trips = 0; for (const auto& r : P.rans) trips = std::max<uint64_t>(trips, (r.n + 31) / 32);
        printf("accepted, longest loop %llu trips, temp %llu B\n", (unsigned long long)trips, (unsigned long long)P.temp_bytes); return 0; }
    if (argc == 4 && !strcmp(argv[1], "--decode")) {                  // one archive through the plan and the executor == original
        auto rd = [](const char* q) { std::vector<uint8_t> v; FILE* f = fopen(q, "rb"); if (!f) { perror(q); exit(1); } fseek(f, 0, SEEK_END); v.resize((size_t)ftell(f)); fseek(f, 0, SEEK_SET); if (fread(v.data(), 1, v.size(), f) != v.size()) exit(1); fclose(f); return v; };
        const std::vector<uint8_t> a = rd(argv[2]), o = rd(argv[3]); agp::Plan P; if (agp::build(a.data(), a.size(), P, nvt_cpu)) { printf("refused\n"); return 1; }
        std::vector<uint8_t> T(P.temp_bytes), out(P.orig + 64); Exec X(P, a.data(), T); X.run(nullptr, out.data());
        const bool ok = !X.err && P.orig == o.size() && !memcmp(out.data(), o.data(), o.size()); printf("%s\n", ok ? "bit-perfect" : "DIFFERS"); return ok ? 0 : 1; }
    if (argc < 3) { fprintf(stderr, "usage: %s <archive.open.aet> <original> [n] [save_dir]\n", argv[0]); return 1; }
    auto slurp = [](const char* p) { std::vector<uint8_t> v; FILE* f = fopen(p, "rb"); if (!f) { perror(p); exit(1); }
        fseek(f, 0, SEEK_END); v.resize((size_t)ftell(f)); fseek(f, 0, SEEK_SET); if (fread(v.data(), 1, v.size(), f) != v.size()) exit(1); fclose(f); return v; };
    const std::vector<uint8_t> a0 = slurp(argv[1]), orig = slurp(argv[2]); const int N = argc > 3 ? atoi(argv[3]) : 10000; const char* save = argc > 4 ? argv[4] : nullptr;
    const uint64_t HANG_TRIPS = 1ull << 22;
    uint32_t nb; memcpy(&nb, a0.data() + 24, 4); uint64_t z[4]; memcpy(z, a0.data() + 36, 32);
    const uint64_t tab_end = 68 + 64ull * nb, lit_lo = tab_end, lit_hi = lit_lo + z[0], tok_hi = std::min<uint64_t>(a0.size(), lit_hi + z[1] + z[2] + z[3]);
    std::mt19937_64 g(4242); const char* kinds[7] = {"bit flip", "random bytes", "zeroed run", "truncation", "header/table", "literal stream", "token streams"};
    long refused = 0, caught = 0, harmless = 0, silent = 0, caught_h = 0, harmless_h = 0, silent_h = 0, hangs = 0; uint64_t max_trips_ok = 0;
    std::vector<uint8_t> a;
    for (int it = 0; it < N; it++) {                                  // the mutation code of gpu_h100_tests.cu t_stress, verbatim
        const int k = it % 7; a = a0; auto rpos = [&](uint64_t lo, uint64_t hi) { return hi > lo ? lo + g() % (hi - lo) : g() % a.size(); };
        switch (k) {
            case 0: { const uint64_t p = g() % a.size(); a[p] ^= (uint8_t)(1u << (g() % 8)); break; }
            case 1: { const int m = 1 + (int)(g() % 8); for (int j = 0; j < m; j++) a[g() % a.size()] = (uint8_t)g(); break; }
            case 2: { const uint64_t p = g() % a.size(), L = 1 + g() % 256; memset(a.data() + p, 0, std::min<uint64_t>(L, a.size() - p)); break; }
            case 3: a.resize(68 + g() % (a.size() - 68)); break;
            case 4: a[rpos(0, tab_end)] ^= (uint8_t)(1u << (g() % 8)); break;
            case 5: a[rpos(lit_lo, lit_hi)] ^= (uint8_t)(1u << (g() % 8)); break;
            case 6: a[rpos(lit_hi, tok_hi)] ^= (uint8_t)(1u << (g() % 8)); break;
        }
        agp::Plan P; if (agp::build(a.data(), a.size(), P, nvt_cpu)) { refused++; continue; }
        if (P.orig > orig.size() + 256) { caught++; caught_h++; continue; }
        uint64_t trips = 0;                                           // the longest device loop this plan asks for
        for (const auto& r : P.rans) trips = std::max<uint64_t>(trips, (r.n + 31) / 32);
        for (const auto& d : P.open) trips = std::max<uint64_t>(trips, (uint64_t)(d.ncse + 255) / 256 + (d.ngap + 255) / 256);
        if (trips > HANG_TRIPS || P.temp_bytes > (1ull << 31)) {
            hangs++; printf("[stress-emu] copy %d (%s): plan accepted, %llu loop trips in one job, temp %.1f GB: the device runs for seconds (hang)\n",
                            it, kinds[k], (unsigned long long)trips, P.temp_bytes / 1e9);
            if (save) { mkdir(save, 0755); char p[512]; snprintf(p, sizeof p, "%s/copy%05d.aet", save, it); FILE* f = fopen(p, "wb"); if (f) { fwrite(a.data(), 1, a.size(), f); fclose(f); }
                snprintf(p, sizeof p, "%s/copy%05d.txt", save, it); f = fopen(p, "w");
                if (f) { fprintf(f, "T-H4 copy %d of scripts/gpu_h100_tests.cu stress (seed 4242, kind %d = %s), archive %s; plan accepted, longest device loop %llu trips, temp %llu B\n",
                                 it, k, kinds[k], argv[1], (unsigned long long)trips, (unsigned long long)P.temp_bytes); fclose(f); } }
            continue; }
        max_trips_ok = std::max(max_trips_ok, trips);
        for (int pass = 0; pass < 2; pass++) {
            std::vector<uint8_t> T(P.temp_bytes), out(P.orig + 64, 0xA5); Exec X(P, a.data(), T); X.run(nullptr, out.data());
            int st = X.err; if (X.limit) st |= 32;
            if (pass && !st && XXH3_64bits(out.data(), P.orig) != P.xxh) st |= 16;
            const bool same = P.orig == orig.size() && !memcmp(out.data(), orig.data(), P.orig);
            if (st) (pass ? caught_h : caught)++; else if (same) (pass ? harmless_h : harmless)++; else (pass ? silent_h : silent)++;
        }
    }
    const bool ok = !hangs && !silent_h;
    printf("[stress-emu] %d copies: refused by the plan %ld, caught %ld, harmless %ld, silent %ld (no hash); with XXH3: caught %ld, harmless %ld, silent %ld; device hangs %ld; longest accepted loop %llu trips\n",
           N, refused, caught, harmless, silent, caught_h, harmless_h, silent_h, hangs, (unsigned long long)max_trips_ok);
    printf("STRESSEMU\t%d\t%ld\t%ld\t%ld\t%ld\t%ld\t%ld\t%s\n", N, refused, caught + caught_h, harmless, silent, silent_h, hangs, ok ? "ok" : "FAILED");
    return ok ? 0 : 5;
}
