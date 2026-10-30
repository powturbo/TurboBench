// aceapex_gpu_abi.cpp - the host-only part of the GPU C ABI (src/aceapex_gpu.h): argument and flag checks of
// plan_create, the per-thread last error, the version. Plain C++ (no CUDA call), so the judge checks it on a host
// without the CUDA toolkit (scripts/gpu_abi_test.cpp, claim head_gpu_abi); the plan itself is built by
// agpu_plan_build in src/aceapex_gpu_lib.cu. Built into the library with it (nvcc compiles .cpp as host code).
#include "aceapex_gpu.h"

aceapex_gpu_plan* agpu_plan_build(const void* h_archive, size_t in_bytes, uint64_t flags, int* err, uint32_t b0 = 0, uint32_t b1 = 0);

static thread_local int g_last = ACEAPEX_GPU_OK;

extern "C" unsigned aceapex_gpu_version(void) { return ACEAPEX_GPU_API_VERSION; }
extern "C" int aceapex_gpu_last_error(void) { return g_last; }
extern "C" aceapex_gpu_plan* aceapex_gpu_plan_create(const void* h_archive, size_t in_bytes, uint64_t flags) {
    g_last = ACEAPEX_GPU_OK;
    if (!h_archive || (flags & ~(uint64_t)ACEAPEX_GPU_PLAN_FLAGS)) { g_last = ACEAPEX_GPU_E_ARGS; return nullptr; }
    int e = ACEAPEX_GPU_OK;
    aceapex_gpu_plan* p = agpu_plan_build(h_archive, in_bytes, flags, &e);
    g_last = p ? ACEAPEX_GPU_OK : (e ? e : ACEAPEX_GPU_E_CUDA);
    return p;
}
extern "C" aceapex_gpu_plan* aceapex_gpu_plan_create_blocks(const void* h_archive, size_t in_bytes, uint32_t b0, uint32_t b1, uint64_t flags) {
    g_last = ACEAPEX_GPU_OK;
    if (!h_archive || b1 <= b0 || (flags & ~(uint64_t)ACEAPEX_GPU_PLAN_FLAGS)) { g_last = ACEAPEX_GPU_E_ARGS; return nullptr; }
    int e = ACEAPEX_GPU_OK;
    aceapex_gpu_plan* p = agpu_plan_build(h_archive, in_bytes, flags, &e, b0, b1);
    g_last = p ? ACEAPEX_GPU_OK : (e ? e : ACEAPEX_GPU_E_CUDA);
    return p;
}
