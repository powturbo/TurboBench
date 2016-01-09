#include <iostream>
#include "lz77.h"

#include <stdio.h>


int main(int argc, char** argv) {

    bool compress = false;
    bool decompress = false;
    bool fastmode = false;
    bool smallmode = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);

        if (arg == "-c")
            compress = true;
        else if (arg == "-d")
            decompress = true;
        else if (arg == "-1")
            fastmode = true;
        else if (arg == "-2")
            smallmode = true;
    }

    const size_t BUFSIZE = (smallmode || decompress ? 100*1024 : 10*1024*1024);

    if (compress) {

        std::string buff;

        size_t searchlen = (fastmode ? 1 : lz77::DEFAULT_SEARCHLEN);
        size_t blocksize = (smallmode ? 4096 : lz77::DEFAULT_BLOCKSIZE);
        
        lz77::compress_t compress(searchlen, blocksize);

        while (1) {
            buff.resize(BUFSIZE);
            size_t i = ::fread((void*)buff.data(), 1, buff.size(), stdin);
            buff.resize(i);

            if (i > 0) {
                std::string out = compress.feed(buff);
                ::fwrite(out.data(), 1, out.size(), stdout);
            }

            if (i != BUFSIZE)
                break;
        }
    
    } else if (decompress) {

        std::string buff;
        buff.resize(BUFSIZE);
        size_t buff_size = 0;

        lz77::decompress_t decompress;
        std::string extra;

        while (1) {
            buff_size = ::fread((void*)buff.data(), 1, buff.size(), stdin);
            
            if (buff_size == 0)
                break;

            std::string* what = &buff;
            size_t what_size = buff_size;
            
            while (what_size > 0) {

                const unsigned char* whatd = (const unsigned char*)what->data();
                bool done = decompress.feed(whatd, whatd + what_size, extra);

                if (!done)
                    break;

                const std::string& result = decompress.result();
                ::fwrite(result.data(), 1, result.size(), stdout);

                what = &extra;
                what_size = extra.size();
            }

            if (buff_size != BUFSIZE)
                break;
        }

    } else {
        fprintf(stderr, "Usage: %s [-1|-2] {-c|-d}, where -c is compression and -d is decompression.\n"
                "  Input is stdin and and output is stdout.\n"
                "  Add '-1' when compressing to enable fast and bad compression.\n"
                "  Add '-2' when compressing to enable a compression mode for small files.\n", argv[0]);
        return 1;
    }

    return 0;
}
