libzling
========

**fast and lightweight compression library and utility.**

Introduction
============

Libzling is an improved lightweight compression utility and library. libzling uses fast order-1 ROLZ (16MB block size and 10MB dictionary size) followed with Huffman encoding, making it **3 times as fast as gzip on compressing, while still getting much better compression ratio and decompression speed**.

Simple benchmark with **enwik8**(100,000,000 bytes), also on [Large Text Compression Benchmark](http://mattmahoney.net/dc/text.html#2702) (thanks to Matt Mahoney)

Tool    | Compressed Size | Encode | Decode |
--------|-----------------|--------|--------|
xz      |26376KB          |83.870s |2.237s  |
bzip2   |29009KB          |10.733s |4.649s  |
zling e4|29721KB          |4.150s  |1.016s  |
zling e3|30000KB          |3.554s  |1.061s  |
zling e2|30478KB          |2.947s  |1.030s  |
zling e1|30856KB          |2.634s  |1.034s  |
zling e0|31456KB          |2.302s  |1.080s  |
gzip    |36518KB          |6.597s  |1.094s  |

Build & Install
===============

You can build and install libzling automatically by **cmake** with the following command:

    cd ./build
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/path/to/install
    make
    make install

Usage
=====

Libling provides simple and lightweight interface. here is a simple program showing the basic usage of libzling. (compiled with `g++ -Wall -O3 zling_sample.cpp -o zling_sample -lzling`)

```C++
#include "libzling/libzling.h"

int main() {
    // compress
    {
        const int level = 0;  // valid levels: 0, 1, 2, 3, 4
        FILE* fin = fopen("./1.txt", "rb");
        FILE* fout = fopen("./1.txt.zlng", "wb");

        baidu::zling::FileInputter  inputter(fin);
        baidu::zling::FileOutputter outputter(fout);

        baidu::zling::Encode(&inputter, &outputter, level);
        fclose(fin);
        fclose(fout);
    }

    // decompress
    {
        FILE* fin = fopen("./1.txt.zlng", "rb");
        FILE* fout = fopen("./2.txt", "wb");

        baidu::zling::FileInputter  inputter(fin);
        baidu::zling::FileOutputter outputter(fout);

        baidu::zling::Decode(&inputter, &outputter);
        fclose(fin);
        fclose(fout);
    }
    return 0;
}
```
However libzling supports more complicated interface, see **./demo/zling.cpp** for details.
