/*
 * ACEAPEX decode example
 * Compile: g++ -O3 -std=c++17 -o decode_example decode_example.cpp -lpthread -lzstd
 * Usage:   ./decode_example input.acpx output
 */
#include "../include/aceapex_decode.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    if (argc < 3) {
        printf("Usage: %s <input.acpx> <output>\n", argv[0]);
        return 1;
    }
    FILE* f = fopen(argv[1], "rb");
    if (!f) { perror("open"); return 1; }
    fseek(f, 0, SEEK_END);
    size_t in_sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t* in = (uint8_t*)malloc(in_sz);
    fread(in, 1, in_sz, f);
    fclose(f);

    size_t out_sz = aceapex_decompress_bound(in, in_sz);
    if (!out_sz) { fprintf(stderr, "Not a valid .acpx file\n"); return 1; }
    printf("Compressed:   %zu bytes\n", in_sz);
    printf("Original:     %zu bytes\n", out_sz);
    printf("Ratio:        %.3fx\n", (double)out_sz / in_sz);

    uint8_t* out = (uint8_t*)malloc(out_sz);
    int ok = aceapex_decompress(in, in_sz, out, out_sz, 8);
    printf("Decompress:   %s\n", ok ? "OK" : "FAIL");

    if (ok) {
        FILE* fo = fopen(argv[2], "wb");
        fwrite(out, 1, out_sz, fo);
        fclose(fo);
    }
    free(in); free(out);
    return !ok;
}
