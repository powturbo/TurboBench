
#include <stdlib.h>
#include <sys/time.h>
#include <string>
#include <iostream>

struct bm {
    struct timeval b;
    std::string msg;
    bm(const std::string& s) : msg(s) {
        gettimeofday(&b, NULL);
    }
    ~bm() {
        struct timeval e;
        gettimeofday(&e, NULL);
        size_t a = (e.tv_sec*1e6 + e.tv_usec);
        size_t q = (b.tv_sec*1e6 + b.tv_usec);
        std::cout << msg << ": " << ((double)a-(double)q)/1e6 << std::endl;
    }
};

struct bm_s {
    struct timeval b;
    double& _s;
    bm_s(double& s) : _s(s) {
        gettimeofday(&b, NULL);
    }
    ~bm_s() {
        struct timeval e;
        gettimeofday(&e, NULL);
        size_t a = (e.tv_sec*1e6 + e.tv_usec);
        size_t q = (b.tv_sec*1e6 + b.tv_usec);
        _s += ((double)a-(double)q)/1e6;
    }
};


#include "lz77.h"

#include <fstream>

int main(int argc, char** argv) {

    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " 'string to compress'" << std::endl
                  << "       " << argv[0] << " -f file_to_compress" << std::endl;
        return 0;
    }

    std::string inp(argv[1]);

    if (inp == "-f") {
        bm _x1("File reading time");

        std::ifstream in(argv[2], std::ios::in | std::ios::binary);

        if (!in)
            return 1;

        std::string data;
        in.seekg(0, std::ios::end);
        data.resize(in.tellg());
        in.seekg(0, std::ios::beg);
        in.read(&data[0], data.size());
        in.close();

        inp.swap(data);

    } else if (inp == "-h" || inp == "--help") {
        std::cout << "Usage: " << argv[0] << " 'string to compress'" << std::endl
                  << "       " << argv[0] << " -f file_to_compress" << std::endl;
        return 0;
    }

    std::string out;

    {
        bm _x2("Compression time");
        lz77::compress_t compress;
        out = compress.feed(inp);
    }

    std::cout << "Compressed size:   " << out.size() << std::endl;
    std::cout << "Uncompressed size: " << inp.size() << std::endl;

    lz77::decompress_t decompress;

    {
        bm _x2("Decompression time");
        std::string extra;

        if (!decompress.feed(out, extra) || extra.size() > 0) {
            std::cout << "Sanity error: failed to decompress whole buffer." << std::endl;
            return 1;
        }
    }

    if (decompress.result() != inp) {
        std::cout << "Compression-decompression equivalence test failed!" << std::endl;

        if (decompress.result().size() < 1024)
            std::cout << "  " << decompress.result() << std::endl;

        return 1;
    }

    return 0;
}

