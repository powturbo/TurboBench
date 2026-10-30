// gpu_stream.cu - an output larger than the card: the archive on the device once (or its slice per batch when even that
// does not fit), the blocks decoded in batches of K through block-range plans (aceapex_gpu_plan_create_blocks: temp and
// output sized for the batch only), two batches in flight on two streams - the decode of batch i+1 runs while batch i
// is copied to pinned host memory - and the host hashes the batches in order (XXH3 streaming) against the archive
// header; with an original file given every batch is also compared with it. K from the free device memory (two slots
// of window + temp inside 40 % of it) or AX_STREAM_BLOCKS; AX_STREAM_MAX_MB caps a slot (to exercise many batches on a
// big card). Times: decode only (events, sum of batches) and wall (decode + D2H + host hash).
// Build: nvcc -std=c++17 -O3 -arch=sm_XX -Isrc [-DACEAPEX_GPU_NVCOMP -I<nvcomp>/include] scripts/gpu_stream.cu
//        src/aceapex_gpu_lib.cu src/aceapex_gpu_abi.cpp [-l:libnvcomp.so.5 -lzstd]
// Usage: gpu_stream <archive.aet> [original]
// Last line: STREAMROW <tab> archive bytes output batches blocks_per_batch slot_MB decode_s wall_s GB/s_wall hash cmp
#include "aceapex_gpu.h"
#define XXH_INLINE_ALL
#include "xxhash.h"
#include <cuda_runtime.h>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#define CK(x) do { cudaError_t e_ = (x); if (e_ != cudaSuccess) { printf("CUDA %s @%d: %s\n", #x, __LINE__, cudaGetErrorString(e_)); exit(2); } } while (0)
static double now_s() { return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
static uint64_t env_u(const char* k, uint64_t d) { const char* e = getenv(k); return e ? strtoull(e, 0, 10) : d; }
int main(int argc, char** argv) {
    setvbuf(stdout, nullptr, _IONBF, 0);
    if (argc < 2) { fprintf(stderr, "usage: %s <archive.aet> [original]\n", argv[0]); return 1; }
    FILE* f = fopen(argv[1], "rb"); if (!f) { perror(argv[1]); return 1; }
    fseek(f, 0, SEEK_END); std::vector<uint8_t> a((size_t)ftell(f)); fseek(f, 0, SEEK_SET);
    if (fread(a.data(), 1, a.size(), f) != a.size()) return 1;
    fclose(f);
    FILE* fo = argc > 2 ? fopen(argv[2], "rb") : nullptr;
    uint32_t nb, bs; uint64_t n, hx; memcpy(&nb, a.data() + 24, 4); memcpy(&bs, a.data() + 20, 4); memcpy(&n, a.data() + 12, 8); memcpy(&hx, a.data() + 28, 8);
    // the whole plan once: temp per block for the batch size
    aceapex_gpu_plan* full = aceapex_gpu_plan_create(a.data(), a.size(), 0);
    if (!full) { printf("plan_create failed: %d\n", aceapex_gpu_last_error()); return 3; }
    const double tpb = (double)aceapex_gpu_temp_bytes(full) / nb; aceapex_gpu_plan_destroy(full);
    size_t fr = 0, tot = 0; CK(cudaMemGetInfo(&fr, &tot));
    const bool arch_fits = (double)a.size() < 0.5 * (double)fr;
    double slot = ((double)fr - (arch_fits ? (double)a.size() : 0)) * 0.4 / 2;
    const uint64_t cap = env_u("AX_STREAM_MAX_MB", 0); if (cap) slot = std::min(slot, (double)cap * 1048576.0);
    uint64_t K = env_u("AX_STREAM_BLOCKS", 0); if (!K) K = (uint64_t)std::max(1.0, slot / (1.2 * ((double)bs + tpb)));
    K = std::min<uint64_t>(K, nb); const uint64_t NBATCH = (nb + K - 1) / K;
    printf("[stream] archive %zu B, output %llu B, %u blocks of %u; device %.1f GB free: %s, batches of %llu blocks (%llu batches, slot ~%.0f MB), 2 in flight\n",
           a.size(), (unsigned long long)n, nb, bs, fr / 1e9, arch_fits ? "archive resident" : "archive slice per batch", (unsigned long long)K, (unsigned long long)NBATCH, (K * ((double)bs + tpb)) / 1e6);
    // the batch plans (host), their sizes
    std::vector<aceapex_gpu_plan*> pl(NBATCH); std::vector<uint64_t> ilo(NBATCH), ihi(NBATCH); size_t max_out = 0, max_tmp = 0, max_in = 0;
    const double tp0 = now_s();
    for (uint64_t i = 0; i < NBATCH; i++) { const uint32_t b0 = (uint32_t)(i * K), b1 = (uint32_t)std::min<uint64_t>(nb, (i + 1) * K);
        pl[i] = aceapex_gpu_plan_create_blocks(a.data(), a.size(), b0, b1, 0);
        if (!pl[i]) { printf("plan_create_blocks(%u,%u) failed: %d\n", b0, b1, aceapex_gpu_last_error()); return 3; }
        aceapex_gpu_plan_input_window(pl[i], &ilo[i], &ihi[i]);
        max_out = std::max(max_out, aceapex_gpu_output_bytes(pl[i])); max_tmp = std::max(max_tmp, aceapex_gpu_temp_bytes(pl[i])); max_in = std::max(max_in, (size_t)(ihi[i] - ilo[i])); }
    printf("[stream] %llu batch plans in %.3f s; slot: window %.1f MB + temp %.1f MB%s\n", (unsigned long long)NBATCH, now_s() - tp0, max_out / 1e6, max_tmp / 1e6,
           arch_fits ? "" : (std::string(" + archive slice ") + std::to_string(max_in / 1000000) + " MB").c_str());
    uint8_t* d_arch = nullptr; if (arch_fits) { CK(cudaMalloc(&d_arch, a.size() + 256)); CK(cudaMemcpy(d_arch, a.data(), a.size(), cudaMemcpyHostToDevice)); }
    uint8_t *d_out[2], *d_tmp[2], *d_in[2] = {nullptr, nullptr}, *h_out[2]; int* d_st[2]; cudaStream_t s[2]; cudaEvent_t e0[2], e1[2], ed[2];
    for (int k = 0; k < 2; k++) { CK(cudaMalloc(&d_out[k], max_out + 256)); CK(cudaMalloc(&d_tmp[k], max_tmp + 256)); CK(cudaMalloc(&d_st[k], 4)); CK(cudaHostAlloc(&h_out[k], max_out + 256, cudaHostAllocDefault));
        if (!arch_fits) CK(cudaMalloc(&d_in[k], max_in + 256));
        CK(cudaStreamCreate(&s[k])); CK(cudaEventCreate(&e0[k])); CK(cudaEventCreate(&e1[k])); CK(cudaEventCreate(&ed[k])); }
    XXH3_state_t* hs = XXH3_createState(); XXH3_64bits_reset(hs);
    std::vector<uint8_t> ref(fo ? max_out : 0); bool cmp_ok = true, st_ok = true; double dec = 0;
    const double t0 = now_s();
    auto launch = [&](uint64_t i) { const int k = (int)(i & 1);
        const uint8_t* din = d_arch ? d_arch + ilo[i] : d_in[k];
        if (!d_arch) CK(cudaMemcpyAsync(d_in[k], a.data() + ilo[i], ihi[i] - ilo[i], cudaMemcpyHostToDevice, s[k]));
        CK(cudaEventRecord(e0[k], s[k]));
        const int r = aceapex_gpu_decompress_async(pl[i], din, d_out[k], d_tmp[k], d_st[k], 0, s[k]);
        if (r) { printf("decode of batch %llu: %d\n", (unsigned long long)i, r); exit(4); }
        CK(cudaEventRecord(e1[k], s[k])); CK(cudaMemcpyAsync(h_out[k], d_out[k], aceapex_gpu_output_bytes(pl[i]), cudaMemcpyDeviceToHost, s[k])); CK(cudaEventRecord(ed[k], s[k])); };
    if (NBATCH) launch(0);
    for (uint64_t i = 0; i < NBATCH; i++) {
        if (i + 1 < NBATCH) launch(i + 1);
        const int k = (int)(i & 1); const size_t len = aceapex_gpu_output_bytes(pl[i]);
        CK(cudaEventSynchronize(ed[k])); float ms = 0; CK(cudaEventElapsedTime(&ms, e0[k], e1[k])); dec += ms / 1e3;
        int st = -1; CK(cudaMemcpy(&st, d_st[k], 4, cudaMemcpyDeviceToHost)); if (st) { st_ok = false; printf("[stream] batch %llu: status %d\n", (unsigned long long)i, st); }
        XXH3_64bits_update(hs, h_out[k], len);
        if (fo) { if (fread(ref.data(), 1, len, fo) != len || memcmp(ref.data(), h_out[k], len)) { if (cmp_ok) printf("[stream] batch %llu differs from the original\n", (unsigned long long)i); cmp_ok = false; } }
    }
    const double wall = now_s() - t0; const uint64_t h = XXH3_64bits_digest(hs); XXH3_freeState(hs);
    const bool hash_ok = h == hx && st_ok;
    printf("[stream] %llu batches of %llu blocks: decode %.3f s (sum of batches), wall %.3f s -> %.2f GB/s with D2H and host hash; XXH3 %016llx %s header%s\n",
           (unsigned long long)NBATCH, (unsigned long long)K, dec, wall, n / wall / 1e9, (unsigned long long)h, hash_ok ? "==" : "!=", fo ? (cmp_ok ? ", every batch == the original" : ", BATCHES DIFFER") : "");
    printf("STREAMROW\t%s\t%zu\t%llu\t%llu\t%llu\t%.0f\t%.3f\t%.3f\t%.2f\t%s\t%s\n", argv[1], a.size(), (unsigned long long)n, (unsigned long long)NBATCH, (unsigned long long)K,
           (max_out + max_tmp) / 1e6, dec, wall, n / wall / 1e9, hash_ok ? "match" : "DIFFERS", fo ? (cmp_ok ? "match" : "DIFFERS") : "-");
    if (fo) fclose(fo);
    for (auto p : pl) aceapex_gpu_plan_destroy(p);
    return hash_ok && cmp_ok ? 0 : 5;
}
