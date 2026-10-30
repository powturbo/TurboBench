# ACEAPEX full device-resident pipeline (e2e_pipe)

## Что это
e2e_pipe.cu — полный GPU decode: ANS-decode 4 потоков (DietGPU) + match
в одном timed-region. Device-resident, bit-perfect. Ядро статьи 5.

## Зависимости
- CUDA 12.4, sm_90 (H100)
- DietGPU (facebookresearch/dietgpu, MIT/BSD)
- libglog: apt-get install -y libgoogle-glog-dev

## Сборка
nvcc -O3 -arch=sm_90 -I<dietgpu_root> \
  -o e2e_pipe e2e_pipe.cu \
  <dietgpu_root>/dietgpu/ans/GpuANSDecode.cu \
  <dietgpu_root>/dietgpu/ans/GpuANSEncode.cu \
  <dietgpu_root>/dietgpu/utils/DeviceUtils.cpp \
  <dietgpu_root>/dietgpu/utils/StackDeviceMemory.cpp \
  -lglog

## Запуск
# streams.bin генерится: aceapex_depth c/d с ACEAPEX_BS=<bytes>
./e2e_pipe streams.bin <original_file> [G=8|16|32] [start] [count]
# выводит FULL-PIPE GB/s + FNV MATCHES OK (bit-perfect против оригинала)

## Верификация
bash verify_mechanism.sh   # 6 проверок: override/block_size/blocks/bit-perfect/warm-cold/match!=full
