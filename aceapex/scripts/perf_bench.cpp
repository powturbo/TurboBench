// perf_bench.cpp - decode timing for the performance gate (scripts/perf_gate.sh): the archive and the original in
// memory, aceapex_decompress_mt with the given thread budget (0 = all hardware threads), N timed runs after one warm-up,
// every run compared byte for byte with the original.
// Prints: median_s min_s output_bytes tau_model dram_read_bytes (or MISMATCH, exit 1)
//   tau_model   bytes moved through memory per (archive + output) byte, by the model: archive read once; token
//               streams written (with read-for-ownership) and read: 3 x; literal stream 3 x unless tiled (AX_LIT_TILE);
//               output 2 x (write + read-for-ownership), 1 x when streamed (AX_NT on this budget and size)
//   dram_read   L1D fills from local DRAM during one decode, x 64 B: demand and software-prefetch fills
//               (ls_any_fills_from_sys.mem_io_local, raw 0x844) + hardware-prefetch fills (ls_hw_pf_dc_fills.mem_io_local,
//               raw 0x85a) - Zen 4 core counters, this process and its threads; reads only (write-backs and
//               non-temporal writes are not counted); -1 when the counters are not available
// Build: g++ -std=c++17 -O3 -march=native -Isrc scripts/perf_bench.cpp src/aceapex_api.cpp -lzstd -lpthread
#include "aceapex.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <vector>
#ifdef __linux__
#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif
static std::vector<uint8_t> slurp(const char* p) {
    std::vector<uint8_t> v; FILE* f = fopen(p, "rb"); if (!f) { perror(p); exit(2); }
    fseek(f, 0, SEEK_END); v.resize(ftell(f)); fseek(f, 0, SEEK_SET);
    if (fread(v.data(), 1, v.size(), f) != v.size()) exit(2);
    fclose(f); return v;
}
static int env_i(const char* k, int d) { const char* e = getenv(k); return e ? atoi(e) : d; }
static double env_f(const char* k, double d) { const char* e = getenv(k); return e ? atof(e) : d; }
int main(int argc, char** argv) {
    if (argc < 5) { fprintf(stderr, "usage: %s <archive> <original> <threads> <runs>\n", argv[0]); return 2; }
    const auto a = slurp(argv[1]), o = slurp(argv[2]); const int T = atoi(argv[3]), R = atoi(argv[4]);
    std::vector<uint8_t> d(o.size() + 64); memset(d.data(), 0, d.size());
    std::vector<double> t; long long fills = -1;
    static const unsigned long long EV[2] = {0x844, 0x85a};
    for (int r = 0; r <= R; r++) {
        int fd[2] = {-1, -1};
#ifdef __linux__
        if (r == 1) for (int k = 0; k < 2; k++) {                // the first timed run: count DRAM fills
            perf_event_attr pe; memset(&pe, 0, sizeof pe); pe.type = PERF_TYPE_RAW; pe.size = sizeof pe; pe.config = EV[k];
            pe.disabled = 1; pe.inherit = 1; pe.exclude_hv = 1;
            fd[k] = (int)syscall(__NR_perf_event_open, &pe, 0, -1, -1, 0);
            if (fd[k] < 0) { pe.exclude_kernel = 1; fd[k] = (int)syscall(__NR_perf_event_open, &pe, 0, -1, -1, 0); }
            if (fd[k] >= 0) { ioctl(fd[k], PERF_EVENT_IOC_RESET, 0); ioctl(fd[k], PERF_EVENT_IOC_ENABLE, 0); }
        }
#endif
        const auto t0 = std::chrono::steady_clock::now();
        const int64_t n = aceapex_decompress_mt(a.data(), a.size(), d.data(), d.size(), T);
        const double s = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
#ifdef __linux__
        if (fd[0] >= 0 && fd[1] >= 0) { fills = 0; for (int k = 0; k < 2; k++) { ioctl(fd[k], PERF_EVENT_IOC_DISABLE, 0); long long c = 0;
            if (read(fd[k], &c, sizeof c) == sizeof c) fills += c; close(fd[k]); } }
        else for (int k = 0; k < 2; k++) if (fd[k] >= 0) close(fd[k]);
#endif
        if (n != (int64_t)o.size() || memcmp(d.data(), o.data(), o.size())) { printf("MISMATCH\n"); return 1; }
        if (r) t.push_back(s);                                   // run 0: warm-up
    }
    std::sort(t.begin(), t.end());
    // the model needs the stream sizes: one more entropy decode, not timed
    aceapex_streams_t st; double tau = -1;
    if (aceapex_decode_streams(a.data(), a.size(), &st) == 0) {
        const double A = (double)a.size(), O = (double)o.size(), L = (double)st.lit_sz, K = (double)(st.off_sz + st.len_sz + st.cmd_sz);
        const int budget = T > 0 ? T : (int)std::thread::hardware_concurrency();
        const bool tile = env_i("AX_LIT_TILE", 1) != 0, nt = env_i("AX_NT", 1) != 0 && budget >= env_i("AX_NT_THREADS", 4) && O >= env_f("AX_NT_MIN", 64.0 * (1 << 20));
        tau = (A + 3 * K + (tile ? 0 : 3 * L) + (nt ? 1 : 2) * O) / (A + O);
        aceapex_streams_free(&st);
    }
    printf("%.4f %.4f %zu %.2f %lld\n", t[t.size() / 2], t[0], o.size(), tau, fills < 0 ? -1LL : fills * 64);
    return 0;
}
