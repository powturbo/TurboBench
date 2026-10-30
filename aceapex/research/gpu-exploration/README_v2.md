# ACEAPEX v2-dev — C API + Python bindings

## Dependencies
    sudo apt-get install -y libzstd-dev g++

## Build shared library
    g++ -O3 -march=native -std=c++17 -fPIC -shared \
        -o libaceapex.so src/aceapex_api.cpp \
        -lpthread -lzstd

## C API example
    #include "src/aceapex.h"
    // Compress
    size_t bound = aceapex_compress_bound(src_size);
    int64_t csize = aceapex_compress(src, src_size, dst, bound, 2, 8);
    // Decompress  
    int64_t dsize = aceapex_decompress(src, src_size, dst, dst_cap);

## Python example
    import sys; sys.path.insert(0, 'src')
    import aceapex_py as ap
    compressed = ap.compress(data, level=2, threads=8)
    restored   = ap.decompress(compressed, len(data))

## Benchmarks (lzbench in-memory, AMD EPYC 4344P, 8 threads)
    File     Ratio    Decode
    nci      11.6x    4779 MB/s
    xml       7.7x    3241 MB/s
    dickens   2.6x    4091 MB/s
    enwik9    3.0x    4200 MB/s

All results BIT-PERFECT (MD5 verified).
