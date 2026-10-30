// gpu_h100_tests.cu - four card tests for the H100 run (scripts/gpu_run.sh ONE): saturation, the PCIe pipeline, random
// access by coordinate, and corrupt archives on the open path. Library calls only (src/aceapex_gpu.h).
//
//   sat    <archive> [max_copies]          T-H1: K decodes of the same archive at once (one resident archive, a plan,
//                                          K streams each with its own temp and output: 25-50 GB of output on an
//                                          80 GB card), K = 1, 2, 4, ... up to what fits; aggregate GB/s per K, the
//                                          peak; every copy checked once with ACEAPEX_GPU_VERIFY_XXH3 (status 0).
//   pcie   <archive> <original> [slot_MB]  T-H2: the archive in pinned host memory goes to the device in pieces of
//                                          slot_MB (4 and 8 by default; tables and token streams first), block batches
//                                          decode as their literal chunks arrive (block-range plans), outputs in a
//                                          ring of 4 slots. Against: H2D alone (whole, and in the same pieces) and the
//                                          decode alone with the archive resident. Every batch checked, untimed.
//   ra     <archive> <fasta> [n] [len] [samtools] [bgz]
//                                          T-H3: n (10 000) regions of len (5 000) bases at random coordinates
//                                          (contig by length, start uniform; offsets from the FASTA's line layout);
//                                          GPU: resident archive, pooled buffers, one region per call + D2H + sync,
//                                          direct launches and CUDA Graph capture (update or re-instantiate per
//                                          region); CPU: aceapex_decompress_region on one thread; samtools faidx on
//                                          the bgzip file per region (a process per call) and for all regions in one
//                                          call (-r). Latency p50 / p99, every result compared with the FASTA.
//                                          Writes regions.txt (chr:start-end, 1-based) next to the archive.
//   dl     <archive.open> <original> [cpu threads]   T-H5: the data loader (windows batches, see t_dl); dl-cpu: CPU rows only
//   stress <archive.open> <original> [n]   T-H4: n (10 000) corrupt copies (bit flip, random bytes, zeroed run,
//                                          truncation, header/table hit, literal or token stream hit); each: plan
//                                          (refused?), decode with a 10 s watchdog (hang?), status, output against
//                                          the original (harmless / caught / silent), then the same with
//                                          ACEAPEX_GPU_VERIFY_XXH3 (silent must be 0).
// Last line of each: H1ROW / H2ROW / H3ROW / H4ROW (tab separated) for the SUMMARY; dl: one H5ROW per window size x batch.
// Build: nvcc -std=c++17 -O3 -arch=sm_90 -Isrc [-DACEAPEX_GPU_NVCOMP <nvcomp>] scripts/gpu_h100_tests.cu
//        src/aceapex_gpu_lib.cu src/aceapex_gpu_abi.cpp src/aceapex_api.cpp -lzstd -lpthread
#include "aceapex_gpu.h"
#include "aceapex.h"
#define XXH_INLINE_ALL
#include "xxhash.h"
#include <cuda_runtime.h>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <random>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
extern char** environ;
#define CK(x) do { cudaError_t e_ = (x); if (e_ != cudaSuccess) { printf("CUDA %s @%d: %s\n", #x, __LINE__, cudaGetErrorString(e_)); exit(2); } } while (0)
static double now_s() { return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
static std::vector<uint8_t> slurp(const char* p) { std::vector<uint8_t> v; FILE* f = fopen(p, "rb"); if (!f) { perror(p); exit(1); }
    fseek(f, 0, SEEK_END); v.resize((size_t)ftell(f)); fseek(f, 0, SEEK_SET); if (fread(v.data(), 1, v.size(), f) != v.size()) exit(1); fclose(f); return v; }
static double pct(std::vector<double> v, double p) { if (v.empty()) return 0; std::sort(v.begin(), v.end()); return v[std::min(v.size() - 1, (size_t)(p * v.size()))]; }
static const char* base(const char* p) { const char* s = strrchr(p, '/'); return s ? s + 1 : p; }
struct BOEnt { uint64_t lit_off, off_off, len_off, cmd_off, lit_sz, off_sz, len_sz, cmd_sz; };   // a block-table entry (64 B)
// wait for an event with a deadline; false = the deadline passed (a hang: the context is not usable any more)
static bool wait_ev(cudaEvent_t e, double sec) { const double t = now_s(); for (;;) { const cudaError_t r = cudaEventQuery(e);
    if (r == cudaSuccess) return true; if (r != cudaErrorNotReady) { printf("CUDA event: %s\n", cudaGetErrorString(r)); exit(2); }
    if (now_s() - t > sec) return false; usleep(50); } }

// ---------------------------------------------------------------- T-H1 saturation
static int t_sat(int argc, char** argv) {
    const std::vector<uint8_t> a = slurp(argv[2]); const int maxk = argc > 3 ? atoi(argv[3]) : 16;
    aceapex_gpu_plan* pl = aceapex_gpu_plan_create(a.data(), a.size(), 0); if (!pl) { printf("plan_create: %d\n", aceapex_gpu_last_error()); return 3; }
    const size_t out = aceapex_gpu_output_bytes(pl), tmp = aceapex_gpu_temp_bytes(pl);
    uint8_t* d_in; CK(cudaMalloc(&d_in, a.size())); CK(cudaMemcpy(d_in, a.data(), a.size(), cudaMemcpyHostToDevice));
    size_t fr, tot; CK(cudaMemGetInfo(&fr, &tot));
    int K = (int)std::min<size_t>((size_t)maxk, (size_t)((double)fr * 0.92 / (double)(out + tmp + 4096)));
    if (K < 1) { printf("[sat] one copy does not fit (%.1f GB free, %.1f GB per copy)\n", fr / 1e9, (out + tmp) / 1e9); return 4; }
    printf("[sat] %s: output %.2f GB + temp %.2f GB per copy; %.1f of %.1f GB free: up to %d copies at once\n", base(argv[2]), out / 1e9, tmp / 1e9, fr / 1e9, tot / 1e9, K);
    std::vector<uint8_t*> d_out(K), d_tmp(K); std::vector<int*> d_st(K); std::vector<cudaStream_t> s(K); std::vector<cudaEvent_t> e0(K), e1(K);
    for (int k = 0; k < K; k++) { CK(cudaMalloc(&d_out[k], out)); CK(cudaMalloc(&d_tmp[k], tmp)); CK(cudaMalloc(&d_st[k], 4));
        CK(cudaStreamCreateWithFlags(&s[k], cudaStreamNonBlocking)); CK(cudaEventCreate(&e0[k])); CK(cudaEventCreate(&e1[k])); }
    // check once: every copy with the on-device XXH3
    int bad = 0;
    for (int k = 0; k < K; k++) { if (aceapex_gpu_decompress_async(pl, d_in, d_out[k], d_tmp[k], d_st[k], ACEAPEX_GPU_VERIFY_XXH3, s[k])) bad++; }
    CK(cudaDeviceSynchronize()); for (int k = 0; k < K; k++) { int st = -1; CK(cudaMemcpy(&st, d_st[k], 4, cudaMemcpyDeviceToHost)); if (st) bad++; }
    double peak = 0; int peak_k = 0; std::string rows;
    for (int k = 1; k <= K; k = (k == K ? K + 1 : std::min(K, k * 2))) {
        std::vector<double> w;
        for (int rep = 0; rep < 3; rep++) {
            CK(cudaDeviceSynchronize()); const double t0 = now_s();
            for (int j = 0; j < k; j++) aceapex_gpu_decompress_async(pl, d_in, d_out[j], d_tmp[j], d_st[j], 0, s[j]);
            CK(cudaDeviceSynchronize()); w.push_back(now_s() - t0); }
        const double med = pct(w, 0.5), gbs = (double)out * k / med / 1e9;
        printf("[sat] %2d at once: %.3f s for %.1f GB -> %.1f GB/s\n", k, med, (double)out * k / 1e9, gbs);
        char b[64]; snprintf(b, sizeof b, "%s%d:%.1f", rows.empty() ? "" : ",", k, gbs); rows += b;
        if (gbs > peak) { peak = gbs; peak_k = k; }
    }
    printf("[sat] peak %.1f GB/s with %d decodes at once (%.1f GB of output); XXH3 check of %d copies: %s\n", peak, peak_k, (double)out * peak_k / 1e9, K, bad ? "FAILED" : "all status 0");
    printf("H1ROW\t%s\t%zu\t%d\t%.1f\t%d\t%s\t%s\n", base(argv[2]), out, K, peak, peak_k, rows.c_str(), bad ? "FAILED" : "ok");
    return bad ? 5 : 0;
}

// ---------------------------------------------------------------- T-H2 PCIe pipeline
// The archive goes to the device in file order in pieces of slot_MB, but the header, the block table and the token
// streams (the tail of the file: tens of MB) first: a block batch can decode once the piece holding the end of its
// literal chunks has arrived. A batch's jobs read the archive slice [in_lo, in_hi) of its plan, which reaches from its
// literal chunks to the token streams at the end - so the archive has one device buffer, filled piece by piece, and the
// outputs go to a ring of 4 slots. Piece copies on one copy stream (events per piece), batches on 4 compute streams.
static int t_pcie(int argc, char** argv) {
    const std::vector<uint8_t> av = slurp(argv[2]); const std::vector<uint8_t> orig = slurp(argv[3]);
    std::vector<int> slots_mb; if (argc > 4) slots_mb.push_back(atoi(argv[4])); else slots_mb = {4, 8};
    uint8_t* h_a; CK(cudaHostAlloc(&h_a, av.size(), cudaHostAllocDefault)); memcpy(h_a, av.data(), av.size());
    uint32_t nb, bs; uint64_t n, z[4]; memcpy(&nb, h_a + 24, 4); memcpy(&bs, h_a + 20, 4); memcpy(&n, h_a + 12, 8); memcpy(z, h_a + 36, 32);
    const uint64_t lit0 = 68 + 64ull * nb, lit1 = lit0 + z[0];
    // literal chunk table: file offset of the end of every chunk (chunked layout only)
    uint64_t lw; memcpy(&lw, h_a + lit0, 8);
    if (!(((lw >> 62) & 1) && ((lw >> 61) & 1))) { printf("[pcie] literal stream not chunked: test skipped\n"); printf("H2ROW\t%s\t-\t-\tskipped\n", base(argv[2])); return 0; }
    uint64_t CH; memcpy(&CH, h_a + lit0 + 8, 8); const uint64_t lsz = lw & ~((7ull) << 60), NW = (lsz + CH - 1) / CH;
    std::vector<uint64_t> cend(NW); { uint64_t q = lit0 + 16 + 8 * NW; for (uint64_t t = 0; t < NW; t++) { uint64_t c; memcpy(&c, h_a + lit0 + 16 + 8 * t, 8); q += c; cend[t] = q; } }
    uint8_t* d_a; CK(cudaMalloc(&d_a, av.size() + 256));
    cudaEvent_t x0, x1; CK(cudaEventCreate(&x0)); CK(cudaEventCreate(&x1)); float ms = 0;
    std::vector<double> hw; for (int r = 0; r < 3; r++) { CK(cudaEventRecord(x0)); CK(cudaMemcpyAsync(d_a, h_a, av.size(), cudaMemcpyHostToDevice)); CK(cudaEventRecord(x1)); CK(cudaEventSynchronize(x1)); CK(cudaEventElapsedTime(&ms, x0, x1)); hw.push_back(ms / 1e3); }
    const double h2d = av.size() / pct(hw, 0.5) / 1e9;
    aceapex_gpu_plan* full = aceapex_gpu_plan_create(h_a, av.size(), 0); if (!full) { printf("plan_create: %d\n", aceapex_gpu_last_error()); return 3; }
    double dec = 0;
    { uint8_t *o, *t; int* st; CK(cudaMalloc(&o, aceapex_gpu_output_bytes(full))); CK(cudaMalloc(&t, aceapex_gpu_temp_bytes(full))); CK(cudaMalloc(&st, 4));
      std::vector<double> dw; for (int r = 0; r < 4; r++) { CK(cudaEventRecord(x0)); aceapex_gpu_decompress_async(full, d_a, o, t, st, 0, 0); CK(cudaEventRecord(x1)); CK(cudaEventSynchronize(x1)); CK(cudaEventElapsedTime(&ms, x0, x1)); if (r) dw.push_back(ms / 1e3); }
      dec = pct(dw, 0.5); CK(cudaFree(o)); CK(cudaFree(t)); CK(cudaFree(st)); }
    aceapex_gpu_plan_destroy(full);
    printf("[pcie] %s: archive %.1f MB (literal stream %.1f MB, the rest %.1f MB) -> %.2f GB; H2D pinned %.1f GB/s (%.1f ms); decode resident %.2f ms = %.1f GB/s of output\n",
           base(argv[2]), av.size() / 1e6, z[0] / 1e6, (av.size() - z[0]) / 1e6, n / 1e9, h2d, av.size() / h2d / 1e6, dec * 1e3, n / dec / 1e9);
    printf("[pcie] the decode takes archive bytes %.1fx faster than the bus brings them: the bus bounds the pipeline at %.1f GB/s of output\n", av.size() / dec / 1e9 / h2d, h2d * n / av.size());
    std::string row;
    for (int smb : slots_mb) {
        const uint64_t P = (uint64_t)smb << 20; const int NS = 4;
        // pieces: [0, lit0+16+8NW) and [lit1, end) first, then the literal chunks in pieces of ~P
        struct Piece { uint64_t lo, hi; }; std::vector<Piece> pc; pc.push_back({0, lit0 + 16 + 8 * NW}); pc.push_back({lit1, av.size()});
        { uint64_t lo = lit0 + 16 + 8 * NW; while (lo < lit1) { const uint64_t hi = std::min(lit1, lo + P); pc.push_back({lo, hi}); lo = hi; } }
        auto piece_of = [&](uint64_t off) { size_t a = 2, b = pc.size() - 1; while (a < b) { const size_t m = (a + b) / 2; if (pc[m].hi < off) a = m + 1; else b = m; } return a; };
        // batches: blocks whose literal chunks end within ~one piece of the previous batch's end
        struct Batch { aceapex_gpu_plan* p; uint64_t lo; size_t need; size_t out; };
        std::vector<Batch> B; { uint32_t b0 = 0; const BOEnt* bo = (const BOEnt*)(h_a + 68);
            while (b0 < nb) { uint32_t b1 = b0 + 1; const uint64_t start = bo[b0].lit_off;
                while (b1 < nb && bo[b1].lit_off + bo[b1].lit_sz - start < P * 3) b1++;     // literal bytes ~3.5x the compressed piece
                aceapex_gpu_plan* p = aceapex_gpu_plan_create_blocks(h_a, av.size(), b0, b1, 0); if (!p) { printf("plan_create_blocks: %d\n", aceapex_gpu_last_error()); return 3; }
                uint64_t l, h; aceapex_gpu_plan_input_window(p, &l, &h);
                const uint64_t le = bo[b1 - 1].lit_off + bo[b1 - 1].lit_sz, ck = le ? std::min<uint64_t>(NW - 1, (le - 1) / CH) : 0;
                B.push_back({p, l, piece_of(cend[ck]), aceapex_gpu_output_bytes(p)}); b0 = b1; } }
        size_t mo = 0, mt = 0; for (auto& b : B) { mo = std::max(mo, b.out); mt = std::max(mt, aceapex_gpu_temp_bytes(b.p)); }
        uint8_t *d_o[NS], *d_t[NS], *h_o[NS]; int* d_s[NS]; cudaStream_t s[NS], cs; std::vector<cudaEvent_t> pe(pc.size()); cudaEvent_t used[NS];
        CK(cudaStreamCreateWithFlags(&cs, cudaStreamNonBlocking)); for (auto& e : pe) CK(cudaEventCreateWithFlags(&e, cudaEventDisableTiming));
        for (int k = 0; k < NS; k++) { CK(cudaMalloc(&d_o[k], mo + 256)); CK(cudaMalloc(&d_t[k], mt + 256)); CK(cudaMalloc(&d_s[k], 4)); CK(cudaHostAlloc(&h_o[k], mo + 256, cudaHostAllocDefault));
            CK(cudaStreamCreateWithFlags(&s[k], cudaStreamNonBlocking)); CK(cudaEventCreateWithFlags(&used[k], cudaEventDisableTiming)); }
        std::vector<double> pw; for (int r = 0; r < 3; r++) { CK(cudaDeviceSynchronize()); const double t0 = now_s();
            for (auto& q : pc) CK(cudaMemcpyAsync(d_a + q.lo, h_a + q.lo, q.hi - q.lo, cudaMemcpyHostToDevice, cs)); CK(cudaStreamSynchronize(cs)); pw.push_back(now_s() - t0); }
        const double h2d_p = av.size() / pct(pw, 0.5) / 1e9;
        auto run = [&](bool d2h) { std::vector<double> w; for (int r = 0; r < 3; r++) { CK(cudaMemset(d_a, 0, av.size())); CK(cudaDeviceSynchronize()); const double t0 = now_s();
                for (size_t i = 0; i < pc.size(); i++) { CK(cudaMemcpyAsync(d_a + pc[i].lo, h_a + pc[i].lo, pc[i].hi - pc[i].lo, cudaMemcpyHostToDevice, cs)); CK(cudaEventRecord(pe[i], cs)); }
                for (size_t i = 0; i < B.size(); i++) { const int k = (int)(i % NS);
                    CK(cudaStreamWaitEvent(s[k], pe[0], 0)); CK(cudaStreamWaitEvent(s[k], pe[1], 0)); CK(cudaStreamWaitEvent(s[k], pe[B[i].need], 0));
                    aceapex_gpu_decompress_async(B[i].p, d_a + B[i].lo, d_o[k], d_t[k], d_s[k], 0, s[k]);
                    if (d2h) CK(cudaMemcpyAsync(h_o[k], d_o[k], B[i].out, cudaMemcpyDeviceToHost, s[k])); }
                CK(cudaDeviceSynchronize()); w.push_back(now_s() - t0); } return pct(w, 0.5); };
        const double tp = run(false), tpd = run(true);
        size_t bad = 0; uint64_t pos = 0;                                       // check, untimed: every batch back
        for (size_t i = 0; i < B.size(); i++) { aceapex_gpu_decompress_async(B[i].p, d_a + B[i].lo, d_o[0], d_t[0], d_s[0], 0, s[0]); CK(cudaStreamSynchronize(s[0]));
            int st = -1; CK(cudaMemcpy(&st, d_s[0], 4, cudaMemcpyDeviceToHost)); CK(cudaMemcpy(h_o[0], d_o[0], B[i].out, cudaMemcpyDeviceToHost));
            if (st || pos + B[i].out > orig.size() || memcmp(h_o[0], orig.data() + pos, B[i].out)) bad++; pos += B[i].out; }
        if (pos != orig.size()) bad++;
        printf("[pcie] pieces of %d MB (tables + token streams first, %.1f MB), %zu batches (<= %.1f MB of output), outputs in a ring of %d: H2D in these pieces %.1f GB/s; "
               "H2D -> decode %.3f s = %.1f GB/s of output = %.2fx the bus (H2D %.1f GB/s); + D2H %.3f s = %.1f GB/s; batches %s\n",
               smb, (pc[0].hi + pc[1].hi - pc[1].lo) / 1e6, B.size(), mo / 1e6, NS, h2d_p, tp, n / tp / 1e9, (n / tp / 1e9) / h2d, h2d, tpd, n / tpd / 1e9, bad ? "DIFFER" : "== original");
        char b[160]; snprintf(b, sizeof b, "%s%dMB:%.1f/%.1f/%.2fx", row.empty() ? "" : ",", smb, n / tp / 1e9, n / tpd / 1e9, (n / tp / 1e9) / h2d); row += b;
        for (int k = 0; k < NS; k++) { cudaFree(d_o[k]); cudaFree(d_t[k]); cudaFree(d_s[k]); cudaFreeHost(h_o[k]); cudaStreamDestroy(s[k]); }
        for (auto& b : B) aceapex_gpu_plan_destroy(b.p);
        if (bad) { printf("H2ROW\t%s\t%.1f\t%s\tFAILED\n", base(argv[2]), h2d, row.c_str()); return 5; }
    }
    printf("H2ROW\t%s\t%.1f\t%s\tok\n", base(argv[2]), h2d, row.c_str());   // bus GB/s; per piece size: output GB/s / with D2H / output over bus (x)
    return 0;
}

// ---------------------------------------------------------------- T-H3 random access
struct Rec { std::string name; uint64_t seq_len, off, lb, lw; };          // .fai fields: length, offset, line bases, line width
static std::vector<Rec> fai(const std::vector<uint8_t>& f) {
    std::vector<Rec> r; size_t i = 0; const size_t n = f.size();
    while (i < n) { if (f[i] != '>') { i++; continue; }
        size_t e = i; while (e < n && f[e] != '\n') e++;
        std::string nm((const char*)&f[i + 1], e - i - 1); nm = nm.substr(0, nm.find_first_of(" \t"));
        Rec q{nm, 0, e + 1, 0, 0}; size_t p = e + 1;
        while (p < n && f[p] != '>') { size_t le = p; while (le < n && f[le] != '\n') le++; const uint64_t L = le - p;
            if (!q.lb) { q.lb = L; q.lw = L + 1; } q.seq_len += L; p = le + 1; }
        r.push_back(q); i = p; }
    return r;
}
static uint64_t boff(const Rec& r, uint64_t pos) { return r.off + pos / r.lb * r.lw + pos % r.lb; }
static std::string bases(const uint8_t* p, size_t n) { std::string s; s.reserve(n); for (size_t i = 0; i < n; i++) if (p[i] != '\n' && p[i] != '\r') s += (char)p[i]; return s; }
static double run_cmd(const std::vector<std::string>& args, std::string& out) {   // posix_spawn, stdout captured; seconds
    int fd[2]; if (pipe(fd)) return -1; posix_spawn_file_actions_t fa; posix_spawn_file_actions_init(&fa);
    posix_spawn_file_actions_adddup2(&fa, fd[1], 1); posix_spawn_file_actions_addclose(&fa, fd[0]);
    std::vector<char*> av; for (auto& a : args) av.push_back((char*)a.c_str()); av.push_back(nullptr);
    const double t0 = now_s(); pid_t pid; if (posix_spawnp(&pid, av[0], &fa, nullptr, av.data(), environ)) { close(fd[0]); close(fd[1]); return -1; }
    close(fd[1]); out.clear(); char b[65536]; ssize_t k; while ((k = read(fd[0], b, sizeof b)) > 0) out.append(b, (size_t)k); close(fd[0]);
    int st; waitpid(pid, &st, 0); posix_spawn_file_actions_destroy(&fa); return now_s() - t0;
}
static int t_ra(int argc, char** argv) {
    const std::vector<uint8_t> a = slurp(argv[2]), f = slurp(argv[3]);
    const int N = argc > 4 ? atoi(argv[4]) : 10000; const uint64_t LEN = argc > 5 ? strtoull(argv[5], 0, 10) : 5000;
    const char* sam = argc > 6 ? argv[6] : nullptr; const char* bgz = argc > 7 ? argv[7] : nullptr;
    const std::vector<Rec> R = fai(f); uint64_t tot = 0; for (auto& r : R) if (r.seq_len > LEN) tot += r.seq_len - LEN;
    std::mt19937_64 g(20261002); struct Q { size_t rec; uint64_t s, lo, len; }; std::vector<Q> q;
    for (int i = 0; i < N; i++) { uint64_t x = g() % tot; size_t k = 0; for (; k < R.size(); k++) { if (R[k].seq_len <= LEN) continue; const uint64_t span = R[k].seq_len - LEN; if (x < span) break; x -= span; }
        const uint64_t lo = boff(R[k], x), hi = boff(R[k], x + LEN - 1) + 1; q.push_back({k, x, lo, hi - lo}); }
    { std::string rp = std::string(argv[2]) + ".regions.txt"; FILE* o = fopen(rp.c_str(), "w");
      for (auto& x : q) fprintf(o, "%s:%llu-%llu\n", R[x.rec].name.c_str(), (unsigned long long)x.s + 1, (unsigned long long)(x.s + LEN)); fclose(o); }
    uint64_t maxlen = 0; for (auto& x : q) maxlen = std::max(maxlen, x.len);
    printf("[ra] %s: %d regions of %llu bases over %zu records (byte length <= %llu)\n", base(argv[3]), N, (unsigned long long)LEN, R.size(), (unsigned long long)maxlen);
    const bool gpu = strcmp(argv[1], "ra-cpu") != 0;                      // ra-cpu: the CPU and samtools rows only (a host without a card)
    std::vector<double> tg, tgg, tc, ts; int bad_g = 0, bad_gg = 0, bad_c = 0, bad_s = 0, graph_upd = 0, graph_new = 0, graph_fail = 0;
    auto check = [&](const Q& x, const uint8_t* p) { return bases(p, x.len) == bases(f.data() + x.lo, x.len); };
    char phase_row[128] = "-"; double plan_ms = 0;
    if (gpu) {
    // GPU: resident archive, pooled buffers
    const double tp0 = now_s();
    aceapex_gpu_plan* pl = aceapex_gpu_plan_create(a.data(), a.size(), 0); if (!pl) { printf("plan_create: %d\n", aceapex_gpu_last_error()); return 3; }
    plan_ms = (now_s() - tp0) * 1e3;
    uint8_t *d_in, *d_out, *d_tmp, *h_out; int *d_st, *h_st; cudaStream_t s; CK(cudaStreamCreateWithFlags(&s, cudaStreamNonBlocking));
    CK(cudaMalloc(&d_in, a.size())); CK(cudaMemcpy(d_in, a.data(), a.size(), cudaMemcpyHostToDevice));
    const size_t rt = aceapex_gpu_range_temp_bytes(pl, maxlen); CK(cudaMalloc(&d_out, maxlen + 256)); CK(cudaMalloc(&d_tmp, rt)); CK(cudaMalloc(&d_st, 4));
    CK(cudaHostAlloc(&h_out, maxlen + 256, cudaHostAllocDefault)); CK(cudaHostAlloc(&h_st, 4, cudaHostAllocDefault));
    for (int i = 0; i < std::min(N, 200); i++) { aceapex_gpu_decompress_range_async(pl, d_in, q[i].lo, q[i].len, d_out, d_tmp, d_st, s); } CK(cudaStreamSynchronize(s));   // warm-up
    // phases of one region (archive resident, plan made once, buffers pooled - no H2D, no allocation per call):
    // launch = host time of the range call (job selection + kernel launches), kernels = GPU time between events
    // around the call, D2H = the copy of the region and the status, wait = the rest of the host's wait in the sync
    std::vector<double> ph_launch, ph_kern, ph_d2h, ph_wait; cudaEvent_t ea, eb, ec; CK(cudaEventCreate(&ea)); CK(cudaEventCreate(&eb)); CK(cudaEventCreate(&ec));
    for (int i = 0; i < N; i++) { const Q& x = q[i]; const double t0 = now_s();
        CK(cudaEventRecord(ea, s));
        const int r = aceapex_gpu_decompress_range_async(pl, d_in, x.lo, x.len, d_out, d_tmp, d_st, s);
        const double t1 = now_s(); CK(cudaEventRecord(eb, s));
        CK(cudaMemcpyAsync(h_out, d_out, x.len, cudaMemcpyDeviceToHost, s)); CK(cudaMemcpyAsync(h_st, d_st, 4, cudaMemcpyDeviceToHost, s)); CK(cudaEventRecord(ec, s));
        const double t2 = now_s(); CK(cudaStreamSynchronize(s)); const double t3 = now_s();
        float k_ms = 0, d_ms = 0; CK(cudaEventElapsedTime(&k_ms, ea, eb)); CK(cudaEventElapsedTime(&d_ms, eb, ec));
        ph_launch.push_back(t1 - t0); ph_kern.push_back(k_ms / 1e3); ph_d2h.push_back(d_ms / 1e3); ph_wait.push_back(std::max(0.0, (t3 - t0) - (t1 - t0) - (t2 - t1) - k_ms / 1e3 - d_ms / 1e3));
        tg.push_back(t3 - t0); if (r || *h_st || !check(x, h_out)) bad_g++; }
    printf("[ra] GPU phases p50 / p99, us: H2D 0 (archive resident), plan 0 (once: %.1f ms), launch (host: selection + kernel launches) %.1f / %.1f, "
           "kernels %.1f / %.1f, D2H %.1f / %.1f, other wait %.1f / %.1f; total %.1f / %.1f\n", plan_ms,
           pct(ph_launch, .5) * 1e6, pct(ph_launch, .99) * 1e6, pct(ph_kern, .5) * 1e6, pct(ph_kern, .99) * 1e6, pct(ph_d2h, .5) * 1e6, pct(ph_d2h, .99) * 1e6,
           pct(ph_wait, .5) * 1e6, pct(ph_wait, .99) * 1e6, pct(tg, .5) * 1e6, pct(tg, .99) * 1e6);
    snprintf(phase_row, sizeof phase_row, "%.1f/%.1f/%.1f/%.1f", pct(ph_launch, .5) * 1e6, pct(ph_kern, .5) * 1e6, pct(ph_d2h, .5) * 1e6, pct(ph_wait, .5) * 1e6);
    // CUDA Graph: the range call captured; the executable graph updated in place when the topology allows, else rebuilt
    cudaGraphExec_t ex = nullptr;
    for (int i = 0; i < N; i++) { const Q& x = q[i]; const double t0 = now_s(); cudaGraph_t gr;
        if (cudaStreamBeginCapture(s, cudaStreamCaptureModeThreadLocal) != cudaSuccess) { graph_fail++; break; }
        const int r = aceapex_gpu_decompress_range_async(pl, d_in, x.lo, x.len, d_out, d_tmp, d_st, s);
        cudaMemcpyAsync(h_out, d_out, x.len, cudaMemcpyDeviceToHost, s); cudaMemcpyAsync(h_st, d_st, 4, cudaMemcpyDeviceToHost, s);
        if (cudaStreamEndCapture(s, &gr) != cudaSuccess || r) { graph_fail++; cudaGetLastError(); break; }
        bool upd = false;
        if (ex) { cudaGraphExecUpdateResultInfo info; upd = cudaGraphExecUpdate(ex, gr, &info) == cudaSuccess; if (!upd) { cudaGetLastError(); cudaGraphExecDestroy(ex); ex = nullptr; } }
        if (!ex) { if (cudaGraphInstantiate(&ex, gr, 0) != cudaSuccess) { graph_fail++; cudaGraphDestroy(gr); break; } graph_new++; } else graph_upd++;
        CK(cudaGraphLaunch(ex, s)); CK(cudaStreamSynchronize(s)); cudaGraphDestroy(gr);
        tgg.push_back(now_s() - t0); if (*h_st || !check(x, h_out)) bad_gg++; }
    if (ex) cudaGraphExecDestroy(ex);
    }
    // CPU: the region call, one thread
    std::vector<uint8_t> cb(maxlen + 64);
    for (int i = 0; i < N; i++) { const Q& x = q[i]; const double t0 = now_s();
        const int64_t r = aceapex_decompress_region(a.data(), a.size(), cb.data(), cb.size(), x.lo, x.len); tc.push_back(now_s() - t0);
        if (r != (int64_t)x.len || !check(x, cb.data())) bad_c++; }
    // samtools faidx on the bgzip file: a process per region, and every region in one call (-r)
    double sam_all = -1; int sam_n = 0;
    if (sam && bgz) {
        for (int i = 0; i < N; i++) { const Q& x = q[i]; char reg[256]; snprintf(reg, sizeof reg, "%s:%llu-%llu", R[x.rec].name.c_str(), (unsigned long long)x.s + 1, (unsigned long long)(x.s + LEN));
            std::string out; const double t = run_cmd({sam, "faidx", bgz, reg}, out); if (t < 0) break; ts.push_back(t); sam_n++;
            const size_t nl = out.find('\n'); std::string b = nl == std::string::npos ? "" : bases((const uint8_t*)out.data() + nl + 1, out.size() - nl - 1);
            if (b != bases(f.data() + x.lo, x.len)) bad_s++; }
        std::string out; sam_all = run_cmd({sam, "faidx", bgz, "-r", std::string(argv[2]) + ".regions.txt"}, out);
    }
    auto line = [&](const char* nm, const std::vector<double>& v, int bad) { if (v.empty()) { printf("[ra] %-38s -\n", nm); return; }
        double m = 0; for (double x : v) m += x; printf("[ra] %-38s p50 %8.1f us  p99 %8.1f us  mean %8.1f us  %s\n", nm, pct(v, 0.5) * 1e6, pct(v, 0.99) * 1e6, m / v.size() * 1e6, bad ? "DIFFERS" : "== FASTA"); };
    line("GPU range call + D2H + sync", tg, bad_g);
    char gl[96]; snprintf(gl, sizeof gl, "GPU CUDA Graph (%d updated, %d built)", graph_upd, graph_new); line(gl, tgg, bad_gg + graph_fail);
    line("CPU aceapex_decompress_region, 1 thread", tc, bad_c);
    line("samtools faidx bgzip, process per region", ts, bad_s);
    if (sam_all >= 0) printf("[ra] samtools faidx bgzip, %d regions in one call (-r): %.2f s = %.1f us per region\n", N, sam_all, sam_all / N * 1e6);
    const bool ok = !bad_g && !bad_gg && !graph_fail && !bad_c && !bad_s && (!gpu || (!tg.empty() && !tgg.empty()));
    printf("H3ROW\t%s\t%d\t%llu\t%.1f/%.1f\t%.1f/%.1f\t%.1f/%.1f\t%.1f/%.1f\t%.1f\t%s\t%s\n", base(argv[2]), N, (unsigned long long)LEN, pct(tg, .5) * 1e6, pct(tg, .99) * 1e6,
           pct(tgg, .5) * 1e6, pct(tgg, .99) * 1e6, pct(tc, .5) * 1e6, pct(tc, .99) * 1e6, pct(ts, .5) * 1e6, pct(ts, .99) * 1e6, sam_all >= 0 ? sam_all / N * 1e6 : -1.0, ok ? "ok" : "FAILED", phase_row);
    return ok ? 0 : 5;
}

// ---------------------------------------------------------------- T-H5 data loader (random windows, output stays on the device)
// For every window size W (1 / 8 / 32 KiB) and batch size n (256 / 4096 / 65536): the archive resident, the plan made once,
// the windows of 20 batches (fixed seed) uploaded once; GPU = aceapex_gpu_decompress_windows_async into one n x W buffer
// (no D2H): latency per batch (call to sync, p50 / p99), host time of the call (selection on the device: launches only),
// throughput (20 batches back to back). 1 % of the windows of the first batch copied back and compared with the
// original. CPU = aceapex_decompress_region per window into a pinned buffer (1 thread: one batch; 16 threads: 20 batches)
// and the H2D of that buffer to the device. dl-cpu: the CPU rows only (a host without a card).
static int t_dl(int argc, char** argv) {
    const bool gpu = strcmp(argv[1], "dl-cpu") != 0;
    const std::vector<uint8_t> a = slurp(argv[2]), o = slurp(argv[3]); const int NB = 20, TH = argc > 4 ? atoi(argv[4]) : 16;
    uint64_t orig; memcpy(&orig, a.data() + 12, 8); if (orig != o.size()) { printf("[dl] original size differs\n"); return 2; }
    aceapex_gpu_plan* pl = nullptr; uint8_t* d_in = nullptr; cudaStream_t s = 0; double plan_ms = 0;
    if (gpu) { const double t0 = now_s(); pl = aceapex_gpu_plan_create(a.data(), a.size(), 0); plan_ms = (now_s() - t0) * 1e3;
        if (!pl) { printf("plan_create: %d\n", aceapex_gpu_last_error()); return 3; }
        CK(cudaMalloc(&d_in, a.size())); CK(cudaMemcpy(d_in, a.data(), a.size(), cudaMemcpyHostToDevice)); CK(cudaStreamCreateWithFlags(&s, cudaStreamNonBlocking)); }
    printf("[dl] %s: %.2f GB original, archive resident, plan %.0f ms (once)\n", base(argv[2]), orig / 1e9, plan_ms);
    const uint32_t Ws[3] = {1024, 8192, 32768}; const uint32_t Ns[3] = {256, 4096, 65536}; bool all_ok = true;
    for (uint32_t W : Ws) for (uint32_t n : Ns) {
        std::mt19937_64 g(20261002ull + W * 7 + n); std::vector<uint64_t> off((size_t)n * NB); for (auto& x : off) x = g() % (orig - W + 1);
        double g_p50 = 0, g_p99 = 0, g_wps = 0, g_host = 0; bool ok = true;
        if (gpu) {
            uint64_t *d_off; uint8_t *d_out, *d_tmp; int* d_st; const size_t tb = aceapex_gpu_windows_temp_bytes(pl, n, W);
            CK(cudaMalloc(&d_off, off.size() * 8)); CK(cudaMemcpy(d_off, off.data(), off.size() * 8, cudaMemcpyHostToDevice));
            CK(cudaMalloc(&d_out, (size_t)n * W)); CK(cudaMalloc(&d_tmp, tb)); CK(cudaMalloc(&d_st, 4));
            std::vector<double> lat, host;
            for (int r = -2; r < NB; r++) { const int k = r < 0 ? 0 : r; const double t0 = now_s();
                const int rc = aceapex_gpu_decompress_windows_async(pl, d_in, d_off + (size_t)k * n, n, W, d_out, d_tmp, d_st, s); const double t1 = now_s();
                CK(cudaStreamSynchronize(s)); const double t2 = now_s(); if (rc) { ok = false; printf("[dl] call: %d\n", rc); break; }
                if (r >= 0) { lat.push_back(t2 - t0); host.push_back(t1 - t0); }
                if (r == 0) { int st = -1; CK(cudaMemcpy(&st, d_st, 4, cudaMemcpyDeviceToHost)); if (st) { ok = false; printf("[dl] status %d\n", st); }
                    std::vector<uint8_t> w(W); for (uint32_t i = 0; i < n; i += 100) { CK(cudaMemcpy(w.data(), d_out + (size_t)i * W, W, cudaMemcpyDeviceToHost));
                        if (memcmp(w.data(), o.data() + off[i], W)) { ok = false; break; } } } }
            CK(cudaStreamSynchronize(s)); const double t0 = now_s();
            for (int r = 0; r < NB; r++) aceapex_gpu_decompress_windows_async(pl, d_in, d_off + (size_t)r * n, n, W, d_out, d_tmp, d_st, s);
            CK(cudaStreamSynchronize(s)); const double tt = now_s() - t0;
            g_p50 = pct(lat, .5); g_p99 = pct(lat, .99); g_host = pct(host, .5); g_wps = (double)n * NB / tt;
            cudaFree(d_off); cudaFree(d_out); cudaFree(d_tmp); cudaFree(d_st);
        }
        // CPU: region calls into a pinned buffer, 16 threads over 20 batches, 1 thread over one, + H2D of the buffer
        uint8_t* h_buf = nullptr; uint8_t* d_buf = nullptr;
        if (gpu) { CK(cudaHostAlloc(&h_buf, (size_t)n * W, cudaHostAllocDefault)); CK(cudaMalloc(&d_buf, (size_t)n * W)); } else h_buf = (uint8_t*)malloc((size_t)n * W);
        auto cpu_batch = [&](int k, int T) { std::vector<std::thread> th; std::atomic<uint32_t> nx{0}; std::atomic<int> bad{0};
            for (int t = 0; t < T; t++) th.emplace_back([&] { for (uint32_t i; (i = nx++) < n; ) { const uint64_t x = off[(size_t)k * n + i];
                if (aceapex_decompress_region(a.data(), a.size(), h_buf + (size_t)i * W, W, x, W) != (int64_t)W) bad = 1; } });
            for (auto& x : th) x.join(); return !bad; };
        double t0 = now_s(); if (!cpu_batch(0, 1)) ok = false; const double c1 = now_s() - t0;
        for (uint32_t i = 0; i < n; i += 100) if (memcmp(h_buf + (size_t)i * W, o.data() + off[i], W)) { ok = false; break; }
        std::vector<double> c16, h2d;
        for (int r = 0; r < NB; r++) { t0 = now_s(); if (!cpu_batch(r, TH)) ok = false; c16.push_back(now_s() - t0);
            if (gpu) { t0 = now_s(); CK(cudaMemcpy(d_buf, h_buf, (size_t)n * W, cudaMemcpyHostToDevice)); h2d.push_back(now_s() - t0); } }
        const double c16m = pct(c16, .5), h2dm = gpu ? pct(h2d, .5) : 0;
        if (gpu) { cudaFreeHost(h_buf); cudaFree(d_buf); } else free(h_buf);
        const double cpu1_wps = n / c1, cpu16_wps = n / c16m, cpuh2d_wps = n / (c16m + h2dm);
        printf("[dl] W %5u n %5u: GPU %10.0f windows/s %6.1f GB/s, batch p50 %.3f p99 %.3f ms, host %.1f us | CPU 1 t %8.0f w/s, %d t %9.0f w/s, + H2D %9.0f w/s | x%.1f | %s\n",
               W, n, g_wps, g_wps * W / 1e9, g_p50 * 1e3, g_p99 * 1e3, g_host * 1e6, cpu1_wps, TH, cpu16_wps, cpuh2d_wps, gpu ? g_wps / cpuh2d_wps : 0.0, ok ? "1 % checked == original" : "DIFFERS");
        printf("H5ROW\t%u\t%u\t%.0f\t%.2f\t%.3f\t%.3f\t%.1f\t%.0f\t%.0f\t%.0f\t%.2f\t%s\n", W, n, g_wps, g_wps * W / 1e9, g_p50 * 1e3, g_p99 * 1e3, g_host * 1e6,
               cpu1_wps, cpu16_wps, cpuh2d_wps, gpu ? g_wps / cpuh2d_wps : 0.0, ok ? "ok" : "FAILED");
        all_ok = all_ok && ok;
    }
    if (pl) aceapex_gpu_plan_destroy(pl);
    return all_ok ? 0 : 5;
}

// ---------------------------------------------------------------- T-H4 corrupt archives
static int t_stress(int argc, char** argv) {
    const std::vector<uint8_t> a0 = slurp(argv[2]), orig = slurp(argv[3]); const int N = argc > 4 ? atoi(argv[4]) : 10000;
    uint32_t nb; memcpy(&nb, a0.data() + 24, 4); uint64_t z[4]; memcpy(z, a0.data() + 36, 32);
    const uint64_t tab_end = 68 + 64ull * nb, lit_lo = tab_end, lit_hi = lit_lo + z[0], tok_hi = std::min<uint64_t>(a0.size(), lit_hi + z[1] + z[2] + z[3]);
    uint8_t *d_in, *d_out, *d_tmp, *h_out; int* d_st; int h_st; cudaStream_t s; cudaEvent_t ev; size_t tcap = 0;
    CK(cudaMalloc(&d_in, a0.size() + 256)); CK(cudaMalloc(&d_out, orig.size() + 256)); CK(cudaMalloc(&d_st, 4)); CK(cudaHostAlloc(&h_out, orig.size() + 256, cudaHostAllocDefault));
    CK(cudaStreamCreateWithFlags(&s, cudaStreamNonBlocking)); CK(cudaEventCreateWithFlags(&ev, cudaEventDisableTiming)); d_tmp = nullptr;
    std::mt19937_64 g(4242); const char* kinds[7] = {"bit flip", "random bytes", "zeroed run", "truncation", "header/table", "literal stream", "token streams"};
    long refused[7] = {0}, caught[7] = {0}, harmless[7] = {0}, silent[7] = {0}, caught_h[7] = {0}, harmless_h[7] = {0}, silent_h[7] = {0}, hang = 0;
    std::vector<uint8_t> a; const double t0 = now_s();
    for (int it = 0; it < N; it++) {
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
        aceapex_gpu_plan* pl = aceapex_gpu_plan_create(a.data(), a.size(), 0);
        if (!pl) { refused[k]++; continue; }
        const size_t tb = aceapex_gpu_temp_bytes(pl), ob = aceapex_gpu_output_bytes(pl);
        if (ob > orig.size() + 256) { caught[k]++; caught_h[k]++; aceapex_gpu_plan_destroy(pl); continue; }   // another size: a plan the caller sizes; not ours to run
        if (tb > tcap) { if (d_tmp) cudaFree(d_tmp); tcap = tb + (tb >> 3); CK(cudaMalloc(&d_tmp, tcap)); }
        CK(cudaMemcpy(d_in, a.data(), a.size(), cudaMemcpyHostToDevice));
        for (int pass = 0; pass < 2; pass++) {
            CK(cudaMemsetAsync(d_out, 0xA5, ob, s));
            if (aceapex_gpu_decompress_async(pl, d_in, d_out, d_tmp, d_st, pass ? ACEAPEX_GPU_VERIFY_XXH3 : 0, s)) { (pass ? caught_h : caught)[k]++; continue; }
            CK(cudaEventRecord(ev, s));
            if (!wait_ev(ev, 10.0)) { hang++; printf("[stress] HANG at copy %d (%s, pass %d): stopped\n", it, kinds[k], pass); goto out; }
            CK(cudaMemcpy(&h_st, d_st, 4, cudaMemcpyDeviceToHost));
            bool same = false;
            if (ob == orig.size()) { CK(cudaMemcpy(h_out, d_out, ob, cudaMemcpyDeviceToHost)); same = memcmp(h_out, orig.data(), ob) == 0; }
            if (h_st) (pass ? caught_h : caught)[k]++; else if (same) (pass ? harmless_h : harmless)[k]++; else (pass ? silent_h : silent)[k]++;
        }
        aceapex_gpu_plan_destroy(pl);
    }
out:
    long R = 0, C = 0, H = 0, S = 0, Ch = 0, Hh = 0, Sh = 0;
    printf("[stress] %s: %d corrupt copies in %.1f s; per kind: refused by the plan / caught on the device / harmless / SILENT, then with XXH3\n", base(argv[2]), N, now_s() - t0);
    for (int k = 0; k < 7; k++) { printf("[stress]   %-15s %5ld / %5ld / %5ld / %3ld   | %5ld / %5ld / %3ld\n", kinds[k], refused[k], caught[k], harmless[k], silent[k], caught_h[k], harmless_h[k], silent_h[k]);
        R += refused[k]; C += caught[k]; H += harmless[k]; S += silent[k]; Ch += caught_h[k]; Hh += harmless_h[k]; Sh += silent_h[k]; }
    printf("[stress] total: refused %ld, caught %ld, harmless %ld, silent %ld (no hash: bytes stored raw carry no check of their own); with XXH3: caught %ld, harmless %ld, silent %ld; hangs %ld\n", R, C, H, S, Ch, Hh, Sh, hang);
    printf("H4ROW\t%s\t%d\t%ld\t%ld\t%ld\t%ld\t%ld\t%ld\t%s\n", base(argv[2]), N, R, C + Ch, H, S, Sh, hang, (!hang && !Sh) ? "ok" : "FAILED");
    return (!hang && !Sh) ? 0 : 5;
}

int main(int argc, char** argv) {
    setvbuf(stdout, nullptr, _IONBF, 0);
    if (argc < 3) { fprintf(stderr, "usage: %s sat|pcie|ra|stress <archive> ...\n", argv[0]); return 1; }
    if (!strcmp(argv[1], "sat")) return t_sat(argc, argv);
    if (!strcmp(argv[1], "pcie") && argc >= 4) return t_pcie(argc, argv);
    if ((!strcmp(argv[1], "ra") || !strcmp(argv[1], "ra-cpu")) && argc >= 4) return t_ra(argc, argv);
    if (!strcmp(argv[1], "stress") && argc >= 4) return t_stress(argc, argv);
    if ((!strcmp(argv[1], "dl") || !strcmp(argv[1], "dl-cpu")) && argc >= 4) return t_dl(argc, argv);
    fprintf(stderr, "bad arguments\n"); return 1;
}
