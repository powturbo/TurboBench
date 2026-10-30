// =============================================================================
// tile_ans_smoke_test.cu — ISOLATED go/no-go test for tile-ANS, before any
// full-pipeline integration work.
//
// THE ONE QUESTION THIS ANSWERS: does ansEncodeBatchStride / ansDecodeBatchStride
// hold at the real tile count of our largest stream (enwik9 LIT ~7500 tiles at
// 64KB), or does it hit the same batch ceiling that killed ansEncodeBatchPointer
// at n=81729 (cudaErrorInvalidConfiguration) three iterations ago?
//
// WHY THIS EXISTS AS A SEPARATE FILE: three times in this project, a batch/
// memory assumption turned out wrong only after it was buried inside a full
// integration (batch-size ceiling, StackDeviceMemory LIFO violation, raw
// cudaMalloc allocation-count blowup). Each cost a full debug cycle. This file
// tests ONLY the one open unknown, in isolation, in under a minute of GPU time,
// before a single line of the real e2e_pipe_tile.cu is written.
//
// PASS CONDITION: no CUDA/DietGPU error at numTiles swept up to ~10,000 (safety
// margin above the real max we need, ~7500), AND bit-perfect round-trip.
// FAIL CONDITION: any cudaErrorInvalidConfiguration or DietGPU CHECK-failure at
// or below ~8000 tiles -> tile-ANS needs a redesign (e.g. hierarchical/nested
// batching) BEFORE the full pipeline is touched.
//
// Build:
//   nvcc -O3 -arch=sm_90 -I/workspace/dietgpu \
//     -o tile_ans_smoke_test tile_ans_smoke_test.cu \
//     /workspace/dietgpu/dietgpu/ans/GpuANSDecode.cu \
//     /workspace/dietgpu/dietgpu/ans/GpuANSEncode.cu \
//     /workspace/dietgpu/dietgpu/utils/DeviceUtils.cpp \
//     /workspace/dietgpu/dietgpu/utils/StackDeviceMemory.cpp \
//     -lglog
//
// Run:
//   ./tile_ans_smoke_test
//   (no arguments; sweeps numTiles = 1000, 2000, 4000, 6000, 7500, 8000, 10000
//    at TILE=65536, reports pass/fail for each, stops at first failure)
// =============================================================================
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <random>
#include <cuda_runtime.h>
#include "dietgpu/ans/GpuANSCodec.h"
#include "dietgpu/utils/StackDeviceMemory.h"
using namespace dietgpu;
using namespace std;

#define CK(x) do{cudaError_t e=(x); if(e){printf("  CUDA err %s:%d %s\n",__FILE__,__LINE__,cudaGetErrorString(e)); return false;} }while(0)

static const uint32_t TILE = 65536;  // matches the block size we use in production sweeps

// Returns true on success (no error, bit-perfect), false on any failure.
// Never throws/aborts on the expected failure mode -- DietGPU's own CHECK
// macros call glog FATAL, which WILL abort the process; this is a known
// limitation of testing against this library (we cannot catch it in-process).
// If the process aborts with a glog F-line, that itself IS the answer: FAIL
// at that numTiles. This is why we sweep via repeated small process runs
// conceptually, but implement as one binary that prints progress before each
// attempt, so a crash still tells us exactly which numTiles failed from the
// last printed line.
static bool try_numtiles(uint32_t numTiles, StackDeviceMemory& res, cudaStream_t stream) {
    uint64_t padded = (uint64_t)numTiles * TILE;

    // synthetic data: reproducible pseudo-random bytes, NOT all-zero (all-zero
    // can hide real ANS behavior since it compresses trivially and may take a
    // different code path). Fixed seed for reproducibility across runs.
    vector<uint8_t> hostData(padded);
    mt19937 rng(42);
    for (auto& b : hostData) b = (uint8_t)(rng() & 0xFF);

    uint8_t* dIn; CK(cudaMalloc(&dIn, padded));
    CK(cudaMemcpy(dIn, hostData.data(), padded, cudaMemcpyHostToDevice));

    uint32_t maxComp = getMaxCompressedSize(TILE);
    uint8_t* dComp; CK(cudaMalloc(&dComp, (size_t)numTiles * maxComp));
    uint32_t* dOutSize; CK(cudaMalloc(&dOutSize, numTiles * 4));

    ANSCodecConfig cfg(10, false);

    printf("  [numTiles=%u] encoding (padded=%.1f MB, maxComp/tile=%u)...\n",
           numTiles, padded/1e6, maxComp);
    ansEncodeBatchStride(res, cfg, numTiles,
                          dIn, TILE, TILE,
                          nullptr,
                          dComp, maxComp,
                          dOutSize, stream);
    CK(cudaStreamSynchronize(stream));
    CK(cudaGetLastError());
    printf("  [numTiles=%u] encode OK, decoding...\n", numTiles);

    uint8_t* dOut; CK(cudaMalloc(&dOut, padded));
    CK(cudaMemset(dOut, 0, padded));  // ensure we are not reading pre-existing correct data

    uint8_t*  dSucc; CK(cudaMalloc(&dSucc, numTiles));
    uint32_t* dDsz;  CK(cudaMalloc(&dDsz, numTiles*4));

    ansDecodeBatchStride(res, cfg, numTiles,
                          dComp, maxComp,
                          dOut, TILE, TILE,
                          dSucc, dDsz, stream);
    CK(cudaStreamSynchronize(stream));
    CK(cudaGetLastError());

    vector<uint8_t> hostOut(padded);
    CK(cudaMemcpy(hostOut.data(), dOut, padded, cudaMemcpyDeviceToHost));

    bool bitperfect = (memcmp(hostOut.data(), hostData.data(), padded) == 0);
    printf("  [numTiles=%u] decode OK, bit-perfect=%s\n", numTiles, bitperfect ? "YES" : "NO (MISMATCH!)");

    cudaFree(dIn); cudaFree(dComp); cudaFree(dOutSize);
    cudaFree(dOut); cudaFree(dSucc); cudaFree(dDsz);

    return bitperfect;
}

int main() {
    printf("=== tile-ANS smoke test: does BatchStride hold at real tile counts? ===\n");
    printf("Target: enwik9 LIT stream at 64KB tiles needs ~7500 tiles.\n");
    printf("Sweeping with safety margin above and below that number.\n\n");

    auto res = dietgpu::makeStackMemory((size_t)2 * 1024 * 1024 * 1024);
    cudaStream_t stream; cudaStreamCreate(&stream);

    uint32_t sweep[] = {1000, 2000, 4000, 6000, 7500, 8000, 10000};
    bool all_pass = true;
    for (uint32_t n : sweep) {
        printf("--- Testing numTiles=%u ---\n", n);
        bool ok = try_numtiles(n, res, stream);
        if (!ok) {
            printf("*** FAIL at numTiles=%u -- STOP HERE, this is the ceiling. ***\n", n);
            all_pass = false;
            break;
        }
        printf("--- numTiles=%u PASSED (encode+decode+bit-perfect) ---\n\n", n);
    }

    cudaStreamDestroy(stream);

    printf("\n=== VERDICT ===\n");
    if (all_pass) {
        printf("ALL TESTED SIZES PASSED, including 10000 (> our real need of ~7500).\n");
        printf("=> Stride API does NOT share the Pointer API's ceiling at this range.\n");
        printf("=> GREEN LIGHT to proceed with the full e2e_pipe_tile.cu integration.\n");
    } else {
        printf("FAILED before reaching our real need. Tile-ANS as designed will NOT\n");
        printf("work for large streams at TILE=%u without redesign (e.g. hierarchical\n", TILE);
        printf("batching: split into groups under the discovered ceiling, loop groups).\n");
        printf("=> DO NOT proceed to full integration yet -- redesign first.\n");
    }
    return all_pass ? 0 : 1;
}
