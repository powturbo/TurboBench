# D1-dense two-kernel decoder — VERIFIED build recipe (2026-07-20, H100)
# Reproduces 199.9 GB/s, bit-perfect, from THIS repo + dietgpu. Tested from a
# clean clone: these exact files compile and reproduce the number.

## Files (all in this repo)
#   dense.cu               - host pipeline (D1-dense two-kernel variant)
#   d2p_dense_v4_282.cuh   - k_parse_d2p / k_copy_d2p kernels
#   d2p_dense_v2.cuh       - struct D2PTrip (the dense triplet type)

## Prerequisites on a fresh machine (the hour we wasted, now saved)
apt-get update && apt-get install -y libgoogle-glog-dev libzstd-dev
# dietgpu built at /path/to/dietgpu with build/lib/{libgpu_ans,libdietgpu_utils}.so

## Encoder (writes streams.bin). Edit min_match_len() in aceapex_depth.cpp:
#   base  6/8/10/12  -> ratio 3.90 ; tuned 12/16/24/32 -> ratio 3.97
# aceapex_main_depth.cpp gives 4.117 - WRONG FILE, do NOT use for the GPU path.
g++ -O3 -march=native -funroll-loops -std=c++17 -Isrc \
    -o aceapex_depth aceapex_depth.cpp -lpthread -lzstd

## D1-dense decoder (dietgpu link chain is the non-obvious part)
nvcc -O3 -arch=sm_90 -I/path/to/dietgpu -I. -Isrc -o e2e_dense dense.cu \
    -L/path/to/dietgpu/build/lib -lgpu_ans -ldietgpu_utils -lpthread

## Run
export LD_LIBRARY_PATH=/path/to/dietgpu/build/lib:$LD_LIBRARY_PATH
export ACEAPEX_BS=16384
./aceapex_depth c --in NA12878_REAL.fastq --out /tmp/c.aet --threads 8
./aceapex_depth d --in /tmp/c.aet --out /tmp/c_dec     # writes streams.bin
./e2e_dense streams.bin NA12878_REAL.fastq 32
# expect ~200 GB/s, MATCHES OK, ratio 3.970
# data: ENA ERR194147, first 1073741620 bytes, md5 9af9ffaa0e15dba938408a711740e101
