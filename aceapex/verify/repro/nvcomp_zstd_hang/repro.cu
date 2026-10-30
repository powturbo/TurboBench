// Decode one zstd frame with nvCOMP's batched zstd decompressor (batch of 1).
// Usage: ./repro <frame.zst>
// The decompressed size is read from the frame header (Frame_Content_Size).
// A watchdog polls the stream for 60 s; if the kernel has not finished by then, it prints HANG and exits.
#include <cuda_runtime.h>
#include <nvcomp/zstd.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <thread>
#include <unistd.h>
#include <vector>

#define CK(x) do { cudaError_t e_ = (x); if (e_ != cudaSuccess) { printf("CUDA error %s at line %d\n", cudaGetErrorString(e_), __LINE__); return 2; } } while (0)

static size_t content_size(const std::vector<unsigned char>& f) {   // RFC 8878 frame header
    if (f.size() < 6 || f[0] != 0x28 || f[1] != 0xB5 || f[2] != 0x2F || f[3] != 0xFD) return 0;
    const unsigned fhd = f[4], flag = fhd >> 6, single = (fhd >> 5) & 1, dict = fhd & 3;
    const size_t p = 5 + (single ? 0 : 1) + (dict == 3 ? 4 : dict);
    const unsigned n = flag == 0 ? (single ? 1 : 0) : (flag == 1 ? 2 : flag == 2 ? 4 : 8);
    size_t v = 0;
    for (unsigned i = 0; i < n && p + i < f.size(); i++) v |= (size_t)f[p + i] << (8 * i);
    return n == 2 ? v + 256 : v;
}

int main(int argc, char** argv) {
    if (argc < 2) { fprintf(stderr, "usage: %s <frame.zst>\n", argv[0]); return 1; }
    FILE* fp = fopen(argv[1], "rb");
    if (!fp) { perror(argv[1]); return 1; }
    std::vector<unsigned char> in;
    fseek(fp, 0, SEEK_END); in.resize(ftell(fp)); fseek(fp, 0, SEEK_SET);
    if (fread(in.data(), 1, in.size(), fp) != in.size()) return 1;
    fclose(fp);
    const size_t out_size = content_size(in);
    if (!out_size) { printf("no Frame_Content_Size in %s\n", argv[1]); return 1; }
    printf("%s: %zu bytes, frame content size %zu\n", argv[1], in.size(), out_size);

    void *d_in, *d_out, *d_temp; void **d_in_ptrs, **d_out_ptrs;
    size_t *d_in_bytes, *d_out_bytes, *d_actual; nvcompStatus_t* d_status;
    size_t temp_bytes = 0;
    nvcompStatus_t s = nvcompBatchedZstdDecompressGetTempSizeAsync(1, out_size, nvcompBatchedZstdDecompressDefaultOpts, &temp_bytes, out_size);
    if (s != nvcompSuccess) { printf("GetTempSizeAsync: status %d\n", (int)s); return 3; }
    CK(cudaMalloc(&d_in, in.size())); CK(cudaMalloc(&d_out, out_size)); CK(cudaMalloc(&d_temp, temp_bytes ? temp_bytes : 1));
    CK(cudaMalloc(&d_in_ptrs, sizeof(void*))); CK(cudaMalloc(&d_out_ptrs, sizeof(void*)));
    CK(cudaMalloc(&d_in_bytes, sizeof(size_t))); CK(cudaMalloc(&d_out_bytes, sizeof(size_t)));
    CK(cudaMalloc(&d_actual, sizeof(size_t))); CK(cudaMalloc(&d_status, sizeof(nvcompStatus_t)));
    const size_t in_bytes = in.size();
    CK(cudaMemcpy(d_in, in.data(), in.size(), cudaMemcpyHostToDevice));
    CK(cudaMemcpy(d_in_ptrs, &d_in, sizeof(void*), cudaMemcpyHostToDevice));
    CK(cudaMemcpy(d_out_ptrs, &d_out, sizeof(void*), cudaMemcpyHostToDevice));
    CK(cudaMemcpy(d_in_bytes, &in_bytes, sizeof(size_t), cudaMemcpyHostToDevice));
    CK(cudaMemcpy(d_out_bytes, &out_size, sizeof(size_t), cudaMemcpyHostToDevice));
    cudaStream_t st; CK(cudaStreamCreate(&st));

    const auto t0 = std::chrono::steady_clock::now();
    s = nvcompBatchedZstdDecompressAsync((const void* const*)d_in_ptrs, d_in_bytes, d_out_bytes, d_actual, 1, d_temp, temp_bytes,
                                         d_out_ptrs, nvcompBatchedZstdDecompressDefaultOpts, d_status, st);
    if (s != nvcompSuccess) { printf("DecompressAsync: status %d\n", (int)s); return 3; }
    for (;;) {                                         // watchdog: cudaStreamSynchronize would wait forever
        const cudaError_t q = cudaStreamQuery(st);
        if (q == cudaSuccess) break;
        if (q != cudaErrorNotReady) { printf("CUDA error %s\n", cudaGetErrorString(q)); return 2; }
        if (std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count() > 60) {
            printf("HANG: the decompression kernel has not finished after 60 s\n"); fflush(stdout); _exit(4);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    CK(cudaStreamSynchronize(st));
    const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    nvcompStatus_t status; size_t actual = 0;
    CK(cudaMemcpy(&status, d_status, sizeof status, cudaMemcpyDeviceToHost));
    CK(cudaMemcpy(&actual, d_actual, sizeof actual, cudaMemcpyDeviceToHost));
    printf("finished in %.3f ms: status %d, decompressed %zu bytes\n", ms, (int)status, actual);
    return 0;
}
