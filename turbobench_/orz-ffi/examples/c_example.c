/*
 * Minimal example using the orz C bindings.
 *
 * Build (after cargo build --release):
 *
 *   cc -O2 examples/c_example.c \
 *      -I include \
 *      -L target/release \
 *      -lorz -lpthread -ldl -lm \
 *      -o c_example
 *
 * On macOS you may also need:
 *   -Wl,-rpath,target/release
 * and the library name is liborz.dylib.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "orz.h"

static void print_hex(const uint8_t *p, size_t n) {
    for (size_t i = 0; i < n; i++)
        printf("%02x", p[i]);
    printf("\n");
}

int main(void) {
    const char *msg = "Hello from the orz C bindings!  "
                      "orz is a fast ROLZ compressor written in Rust.";
    size_t msg_len = strlen(msg);

    OrzBuffer compressed = {0};
    int rc = orz_encode((const uint8_t *)msg, msg_len, /*level=*/1, &compressed);
    if (rc != ORZ_OK) {
        fprintf(stderr, "encode failed: %d\n", rc);
        return 1;
    }

    printf("original size : %zu\n", msg_len);
    printf("compressed size: %zu\n", compressed.len);
    printf("compressed hex : ");
    print_hex(compressed.data, compressed.len > 32 ? 32 : compressed.len);

    OrzBuffer decompressed = {0};
    rc = orz_decode(compressed.data, compressed.len, &decompressed);
    if (rc != ORZ_OK) {
        fprintf(stderr, "decode failed: %d\n", rc);
        orz_buffer_free(&compressed);
        return 1;
    }

    if (decompressed.len != msg_len ||
        memcmp(decompressed.data, msg, msg_len) != 0) {
        fprintf(stderr, "round-trip mismatch!\n");
        orz_buffer_free(&compressed);
        orz_buffer_free(&decompressed);
        return 1;
    }

    printf("round-trip OK: \"%.*s\"\n",
           (int)decompressed.len, (const char *)decompressed.data);

    orz_buffer_free(&compressed);
    orz_buffer_free(&decompressed);
    return 0;
}
