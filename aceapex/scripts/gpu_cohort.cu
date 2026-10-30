// gpu_cohort.cu - pangenome resident in VRAM: G open archives at once on one card.
// 1) every genome fully decoded with ACEAPEX_GPU_VERIFY_XXH3 (status 0 == XXH3 of the original == header): aggregate GB/s
// 2) loader across the cohort: each batch = n windows spread over all G genomes (G streams at once): windows/s
//    windows of genome 0 checked against its full decode, on the device output.
// build: nvcc -std=c++17 -O3 -arch=sm_90 -Isrc scripts/gpu_cohort.cu src/aceapex_gpu_lib.cu src/aceapex_gpu_abi.cpp src/aceapex_api.cpp -lzstd -lpthread -o cohort
// run:   ./gpu_cohort a1.open.aet a2.open.aet ...
#include "aceapex_gpu.h"
#include <cuda_runtime.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <random>
#include <vector>
#include <algorithm>
#define CK(x) do { cudaError_t e_ = (x); if (e_ != cudaSuccess) { printf("CUDA %s @%d: %s\n", #x, __LINE__, cudaGetErrorString(e_)); exit(2); } } while (0)
static double now_s() { return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
static std::vector<uint8_t> slurp(const char* p) { FILE* f = fopen(p, "rb"); if (!f) { printf("open %s\n", p); exit(1); }
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET); std::vector<uint8_t> v(n); if (fread(v.data(), 1, n, f) != (size_t)n) exit(1); fclose(f); return v; }

int main(int argc, char** argv) {
    if (argc < 2) { printf("usage: %s a.open.aet ...\n", argv[0]); return 1; }
    const int G = argc - 1; std::vector<aceapex_gpu_plan*> pl(G); std::vector<uint8_t*> din(G); std::vector<uint64_t> orig(G), asz(G);
    size_t mo = 0, mt = 0; double tplan = 0;
    for (int i = 0; i < G; i++) { std::vector<uint8_t> a = slurp(argv[i + 1]); asz[i] = a.size(); memcpy(&orig[i], a.data() + 12, 8);
        const double t0 = now_s(); pl[i] = aceapex_gpu_plan_create(a.data(), a.size(), 0); tplan += now_s() - t0;
        if (!pl[i]) { printf("plan %s: %d\n", argv[i + 1], aceapex_gpu_last_error()); return 3; }
        CK(cudaMalloc(&din[i], a.size())); CK(cudaMemcpy(din[i], a.data(), a.size(), cudaMemcpyHostToDevice));
        mo = std::max(mo, aceapex_gpu_output_bytes(pl[i])); mt = std::max(mt, aceapex_gpu_temp_bytes(pl[i])); }
    uint64_t A = 0, O = 0; for (int i = 0; i < G; i++) { A += asz[i]; O += orig[i]; }
    size_t fr, to; CK(cudaMemGetInfo(&fr, &to));
    printf("[cohort] %d genomes resident: %.2f GB compressed for %.2f GB of sequence (x%.2f); plans %.1f s on host; card free %.1f of %.1f GB\n",
           G, A / 1e9, O / 1e9, (double)O / A, tplan, fr / 1e9, to / 1e9);

    // 1) full decode of every genome, XXH3 on the device
    uint8_t *dout, *dtmp; int* dst; CK(cudaMalloc(&dout, mo + 256)); CK(cudaMalloc(&dtmp, mt + 256)); CK(cudaMalloc(&dst, 4));
    int bad = 0; double tsum = 0;
    for (int r = 0; r < 2; r++) { tsum = 0; bad = 0;
        for (int i = 0; i < G; i++) { CK(cudaMemset(dst, 0, 4)); CK(cudaDeviceSynchronize()); const double t0 = now_s();
            int rc = aceapex_gpu_decompress_async(pl[i], din[i], dout, dtmp, dst, ACEAPEX_GPU_VERIFY_XXH3, 0); CK(cudaDeviceSynchronize()); const double t = now_s() - t0;
            int st = -1; CK(cudaMemcpy(&st, dst, 4, cudaMemcpyDeviceToHost)); if (rc || st) { bad++; printf("[cohort] genome %d: rc %d status %d\n", i, rc, st); }
            tsum += t; } }
    printf("[cohort] full decode of all %d genomes, XXH3 checked on the card: %.1f ms total, %.1f GB/s; %d of %d verified\n", G, tsum * 1e3, O / tsum / 1e9, G - bad, G);
    // keep genome 0 decoded for the window check
    CK(cudaMemset(dst, 0, 4)); aceapex_gpu_decompress_async(pl[0], din[0], dout, dtmp, dst, 0, 0); CK(cudaDeviceSynchronize());

    // 2) loader across the cohort
    const uint32_t Ws[2] = {1024, 8192}; const uint32_t Ns[2] = {4096, 65536}; const int NB = 10;
    std::vector<cudaStream_t> s(G); for (auto& x : s) CK(cudaStreamCreateWithFlags(&x, cudaStreamNonBlocking));
    for (uint32_t W : Ws) for (uint32_t n : Ns) {
        const uint32_t per = (n + G - 1) / G; std::mt19937_64 g(20261003ull + W + n);
        std::vector<uint64_t*> doff(G); std::vector<uint8_t*> dw(G), dt(G); std::vector<int*> ds(G); std::vector<std::vector<uint64_t>> off(G);
        for (int i = 0; i < G; i++) { off[i].resize((size_t)per * NB); for (auto& x : off[i]) x = g() % (orig[i] - W + 1);
            CK(cudaMalloc(&doff[i], off[i].size() * 8)); CK(cudaMemcpy(doff[i], off[i].data(), off[i].size() * 8, cudaMemcpyHostToDevice));
            CK(cudaMalloc(&dw[i], (size_t)per * W)); CK(cudaMalloc(&dt[i], aceapex_gpu_windows_temp_bytes(pl[i], per, W))); CK(cudaMalloc(&ds[i], 4)); CK(cudaMemset(ds[i], 0, 4)); }
        auto batch = [&](int k) { for (int i = 0; i < G; i++) aceapex_gpu_decompress_windows_async(pl[i], din[i], doff[i] + (size_t)k * per, per, W, dw[i], dt[i], ds[i], s[i]); };
        batch(0); CK(cudaDeviceSynchronize());
        bool ok = true; for (int i = 0; i < G; i++) { int st = -1; CK(cudaMemcpy(&st, ds[i], 4, cudaMemcpyDeviceToHost)); if (st) { ok = false; printf("[cohort] windows genome %d status %d\n", i, st); } }
        { std::vector<uint8_t> a(W), b(W); for (uint32_t j = 0; j < per; j += std::max<uint32_t>(1, per / 64)) {
              CK(cudaMemcpy(a.data(), dw[0] + (size_t)j * W, W, cudaMemcpyDeviceToHost)); CK(cudaMemcpy(b.data(), dout + off[0][j], W, cudaMemcpyDeviceToHost));
              if (memcmp(a.data(), b.data(), W)) { ok = false; break; } } }
        std::vector<double> lat; for (int k = 0; k < NB; k++) { const double t0 = now_s(); batch(k); CK(cudaDeviceSynchronize()); lat.push_back(now_s() - t0); }
        const double t0 = now_s(); for (int k = 0; k < NB; k++) batch(k); CK(cudaDeviceSynchronize()); const double tt = now_s() - t0;
        std::sort(lat.begin(), lat.end());
        const double wps = (double)per * G * NB / tt;
        printf("[cohort] W %5u, %6u windows/batch over %d genomes: %10.0f windows/s, %6.2f GB/s, batch p50 %.3f ms | %s\n",
               W, per * G, G, wps, wps * W / 1e9, lat[lat.size() / 2] * 1e3, ok ? "genome-0 windows == its full decode" : "DIFFERS");
        for (int i = 0; i < G; i++) { cudaFree(doff[i]); cudaFree(dw[i]); cudaFree(dt[i]); cudaFree(ds[i]); }
    }
    for (int i = 0; i < G; i++) { aceapex_gpu_plan_destroy(pl[i]); cudaFree(din[i]); }
    return bad ? 5 : 0;
}
