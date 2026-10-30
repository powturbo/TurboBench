nvCOMP batched zstd decompression does not return on a corrupt frame.
Build:  nvcc -O2 -I$NVCOMP/include -L$NVCOMP/lib64 -o repro repro.cu -l:libnvcomp.so.5   (NVCOMP = pip package dir of nvidia-nvcomp-cu12)
Run:    export LD_LIBRARY_PATH=$NVCOMP/lib64; ./repro good.zst; ./repro bad.zst
good.zst: valid zstd frame, 1136 B, content size 8192 B, one compressed block.
bad.zst:  the same frame with one byte changed: offset 291, 0x3f -> 0xc6 (inside the sequences section).
          libzstd 1.5.5 ZSTD_decompress: "Data corruption detected". Frame and block headers are valid.
Expected: bad.zst returns quickly with an error status (nvcompErrorBadChecksum / nvcompErrorCannotDecompress or similar).
Actual:   good.zst finishes with status 0 and 8192 bytes; bad.zst: the kernel is still running after 60 s ("HANG"),
          cudaStreamSynchronize would never return.
Seen with nvCOMP 5.3.0.16 (pip nvidia-nvcomp-cu12), CUDA 12.8, driver 580.82.07, NVIDIA RTX PRO 6000 Blackwell (sm_120), Linux.
