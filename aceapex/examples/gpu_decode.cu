// gpu_decode.cu - decode an .aet archive on the GPU through the C ABI of src/aceapex_gpu.h.
// Build: nvcc -O3 -arch=sm_XX -Isrc examples/gpu_decode.cu src/aceapex_gpu_lib.cu src/aceapex_gpu_abi.cpp -o gpu_decode
//        or against the shared library: make gpu-lib; nvcc -Isrc examples/gpu_decode.cu -L. -laceapex_gpu -o gpu_decode
//        (+ -DACEAPEX_GPU_NVCOMP -I<nvcomp>/include -l:libnvcomp.so.5 for zstd-profile archives)
// Usage: gpu_decode <archive.aet> <output>
#include "aceapex_gpu.h"
#include <cuda_runtime.h>
#include <cstdio>
#include <vector>
int main(int argc, char** argv) {
    if (argc < 3) { fprintf(stderr, "usage: %s <archive.aet> <output>\n", argv[0]); return 1; }
    FILE* f = fopen(argv[1], "rb"); if (!f) { perror(argv[1]); return 1; }
    fseek(f, 0, SEEK_END); std::vector<char> a(ftell(f)); fseek(f, 0, SEEK_SET);
    if (fread(a.data(), 1, a.size(), f) != a.size()) return 1; fclose(f);
    aceapex_gpu_plan* plan = aceapex_gpu_plan_create(a.data(), a.size(), 0);            // phase 1: host, once
    if (!plan) { fprintf(stderr, "plan: error %d\n", aceapex_gpu_last_error()); return 1; }
    size_t n = aceapex_gpu_output_bytes(plan);
    void *d_in, *d_out, *d_temp; int* d_status; cudaStream_t s; cudaStreamCreate(&s);
    cudaMalloc(&d_in, a.size()); cudaMalloc(&d_out, n + 1); cudaMalloc(&d_temp, aceapex_gpu_temp_bytes(plan)); cudaMalloc(&d_status, sizeof(int));
    cudaMemcpyAsync(d_in, a.data(), a.size(), cudaMemcpyHostToDevice, s);
    int r = aceapex_gpu_decompress_async(plan, d_in, d_out, d_temp, d_status, ACEAPEX_GPU_VERIFY_XXH3, s);   // phase 2: async, output hash checked
    std::vector<char> out(n); int status = -1;
    cudaMemcpyAsync(&status, d_status, sizeof(int), cudaMemcpyDeviceToHost, s);
    cudaMemcpyAsync(out.data(), d_out, n, cudaMemcpyDeviceToHost, s);
    cudaStreamSynchronize(s);
    if (r || status) { fprintf(stderr, "decode: return %d, status %d\n", r, status); return 2; }
    FILE* o = fopen(argv[2], "wb"); fwrite(out.data(), 1, n, o); fclose(o);
    aceapex_gpu_plan_destroy(plan);
    printf("%zu bytes\n", n); return 0;
}
