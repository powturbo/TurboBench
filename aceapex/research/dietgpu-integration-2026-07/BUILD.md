# DietGPU ANS benchmark (489 GB/s literals, vendor-neutral proof)
## Build fixes для CUDA 12.4 / H100 sm_90:
cmake .. -G Ninja -DCMAKE_CUDA_ARCHITECTURES=90 -DCMAKE_POLICY_VERSION_MINIMUM=3.5
# в CMakeLists.txt hardcode arch 90 (автодетект давал compute_86)
cmake --build . --target gpu_ans
## Bench:
nvcc -O3 -arch=sm_90a -I<dietgpu> ANSBench.cu -o ans_bench -lgpu_ans -ldietgpu_utils -lglog
./ans_bench lits.bin
## Результат: DietGPU ANS 489.6 GB/s vs nvcomp 167.9, ratio 1.46 vs 1.51, bit-perfect
