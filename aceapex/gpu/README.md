# ACEAPEX GPU Wavefront Decoder

First BIT-PERFECT GPU LZ77 decoder based on ACEAPEX absolute offsets.

## Results (H100 SXM, 2026-06-01)

| File    | GPU        | CPU       | Speedup |
|---------|------------|-----------|---------|
| enwik9  | 44.0 GB/s  | 3.5 GB/s  | 12.7x   |
| FASTQ   | 19.3 GB/s  | 11.3 GB/s | 1.70x   |
| silesia | 5.3 GB/s   | 4.4 GB/s  | 1.20x   |
| nci     | 2.7 GB/s   | 9.5 GB/s  | 0.28x   |

## Build
```bash
nvcc -O3 -arch=sm_90 -o wf_real wf_real.cu
g++ -std=c++17 -O2 -o aceapex_depth aceapex_depth.cpp -lpthread -lzstd
Status

	•	BIT-PERFECT: verified on all datasets
	•	Algorithm: wavefront per-token by level + CUDA graphs
	•	Production: needs lit[] stream integration
