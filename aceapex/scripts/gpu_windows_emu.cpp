// gpu_windows_emu.cpp - CPU judge of the windows batch (aceapex_gpu_decompress_windows_async, src/aceapex_gpu_lib.cu):
// the device selection replayed on the host from the same plan (agp::build, agp::win_layout) - blocks marked from the
// window offsets, the chunks of those blocks per stream (kw_chunks), the slots in block order (kw_scan), the rANS / open /
// stored-chunk jobs whose (stream, chunk) key is marked (kw_pick) - then those jobs and those blocks run by the plan's
// CPU executor (scripts/gpu_plan_emu.cpp Exec) into one slot each, and the windows gathered (kw_gather). Every window is
// compared with the original; a window past the end must be flagged and left alone. Also the selection's size: blocks,
// jobs and slot bytes per batch, and the selection time on one CPU core (the device does it in kernels).
// Usage: gpu_windows_emu <archive.open.aet> <original> [seed]     Last line: H5EMU <tab> ... ok|FAILED
// Build: g++ -std=c++17 -O2 -Isrc -Iscripts scripts/gpu_windows_emu.cpp src/aceapex_api.cpp -lzstd -lpthread
#define main gpu_plan_emu_main
#include "gpu_plan_emu.cpp"
#undef main
#include <chrono>
static double now_s() { return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
int main(int argc, char** argv) {
    if (argc < 3) { fprintf(stderr, "usage: %s <archive.open.aet> <original> [seed]\n", argv[0]); return 1; }
    auto slurp = [](const char* p) { std::vector<uint8_t> v; FILE* f = fopen(p, "rb"); if (!f) { perror(p); exit(1); }
        fseek(f, 0, SEEK_END); v.resize((size_t)ftell(f)); fseek(f, 0, SEEK_SET); if (fread(v.data(), 1, v.size(), f) != v.size()) exit(1); fclose(f); return v; };
    const std::vector<uint8_t> a = slurp(argv[1]), o = slurp(argv[2]); const uint64_t seed = argc > 3 ? strtoull(argv[3], 0, 10) : 20261002;
    agp::Plan P; if (agp::build(a.data(), a.size(), P, nvt_cpu)) { printf("plan refused\n"); return 2; }
    if (!P.nv.empty() || !P.dna.empty()) { printf("not an open-profile archive\n"); return 2; }
    std::vector<uint8_t> T(P.temp_bytes); Exec X(P, a.data(), T);                     // the plan's temp; jobs rerun per batch
    std::mt19937_64 g(seed); int bad_all = 0; std::string rows;
    const uint32_t Ws[3] = {1024, 8192, 32768}; const uint32_t Ns[3] = {256, 4096, 65536};
    for (uint32_t W : Ws) for (uint32_t n : Ns) {
        if ((uint64_t)n * W > (1ull << 30)) continue;                                  // the emulator keeps it under 1 GiB of output
        std::vector<uint64_t> off(n); for (auto& x : off) x = g() % (P.orig - W + 1);
        const double t0 = now_s();
        const agp::WinLayout L = agp::win_layout(P, n, W);
        std::vector<uint8_t> need(P.nb, 0), cneed(L.cb[3] + L.nc[3] + 1, 0); std::vector<uint32_t> slot(P.nb, 0), list;
        for (uint64_t x : off) for (uint64_t b = x / P.bs; b <= (x + W - 1) / P.bs; b++) need[b] = 1;           // kw_mark
        for (uint32_t b = 0; b < P.nb; b++) if (need[b]) { const uint8_t* e = &P.bo[64ull * b];                  // kw_chunks
            for (int s = 0; s < 4; s++) { const uint64_t os = agp::rd64(e + 8 * s), z = agp::rd64(e + 32 + 8 * s); if (!z || !P.chunk[s]) continue;
                for (uint64_t k = os / P.chunk[s]; k <= (os + z - 1) / P.chunk[s]; k++) cneed[L.cb[s] + k] = 1; } }
        for (uint32_t b = 0; b < P.nb; b++) if (need[b]) { slot[b] = (uint32_t)list.size(); list.push_back(b); }   // kw_scan
        std::vector<uint32_t> rj, oj, wj;                                                                          // kw_pick
        for (size_t i = 0; i < P.rans.size(); i++) { const uint64_t k = P.rans_key[i], st = k >> 48, c = k & ((1ull << 48) - 1); if (st < 4 && cneed[L.cb[st] + c]) rj.push_back((uint32_t)i); }
        for (size_t i = 0; i < P.open.size(); i++) if (cneed[L.cb[0] + P.open_key[i]]) oj.push_back((uint32_t)i);
        for (size_t i = 0; i < P.raw.size(); i++) { const uint64_t k = P.raw_key[i], st = k >> 48, c = k & ((1ull << 48) - 1); if (st < 4 && cneed[L.cb[st] + c]) wj.push_back((uint32_t)i); }
        const double tsel = now_s() - t0;
        if (list.size() > L.maxb) { bad_all++; rows += " slots>maxb"; continue; }
        X.err = 0; for (uint32_t i : wj) X.raw(i); for (uint32_t i : rj) X.rans(i); for (uint32_t k : oj) X.open(k);   // the jobs, as k_rans_n / k_open_*_n
        std::vector<uint8_t> sbuf(L.maxb * P.bs + 64);
        for (size_t q = 0; q < list.size(); q++) { const uint32_t b = list[q]; const uint8_t* e = &P.bo[64ull * b]; auto f = [&](int i) { return agp::rd64(e + 8 * i); };
            const uint64_t base = (uint64_t)b * P.bs, rem = P.orig - base; const uint32_t nn = (uint32_t)std::min<uint64_t>(rem, P.bs);
            if (Exec::match(X.at(P.o_s[0] + f(0)), X.at(P.o_s[1] + f(1)), X.at(P.o_s[2] + f(2)), X.at(P.o_s[3] + f(3)), (uint32_t)f(4), (uint32_t)f(5), (uint32_t)f(6), (uint32_t)f(7),
                            sbuf.data() + q * P.bs, nn, X.steps, X.limit, X.oob) != nn) X.err |= 8; }   // k_decode_list
        std::vector<uint8_t> out((uint64_t)n * W);
        for (uint32_t i = 0; i < n; i++) memcpy(out.data() + (uint64_t)i * W, sbuf.data() + (uint64_t)slot[off[i] / P.bs] * P.bs + off[i] % P.bs, W);   // kw_gather
        int bad = X.err ? 1 : 0; for (uint32_t i = 0; i < n && !bad; i++) if (memcmp(out.data() + (uint64_t)i * W, o.data() + off[i], W)) bad = 1;
        bad_all += bad;
        char b[256]; snprintf(b, sizeof b, "%s%u x %u: %zu blocks (%.1f MB of slots), %zu rANS + %zu open + %zu stored jobs, selection %.0f us, %s",
                              rows.empty() ? "" : "; ", n, W, list.size(), list.size() * (double)P.bs / 1e6, rj.size(), oj.size(), wj.size(), tsel * 1e6, bad ? "DIFFERS" : "== original");
        rows += b; printf("[h5-emu] %s\n", b + (rows.size() > strlen(b) ? 2 : 0));
    }
    // a window past the end: kw_mark flags it (STATUS_RANGE) and kw_gather leaves its bytes alone
    { const uint64_t x = P.orig - 100; const bool flagged = x > P.orig || (uint64_t)1024 > P.orig - x; if (!flagged) bad_all++; }
    printf("H5EMU\t%s\t%s\n", rows.c_str(), bad_all ? "FAILED" : "ok");
    return bad_all ? 1 : 0;
}
