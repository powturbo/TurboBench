// minlz.h
#ifndef MINLZ_H
#define MINLZ_H

#include <stdint.h>
#include <string.h>

// Error codes
#define DECODE_ERR_CODE_CORRUPT -1


// int_minlz_decode decodes src into dst. It assumes that the varint-encoded
// length of the decompressed bytes has already been read, and that dst_len
// equals that length.
//
// Returns 0 on success or DECODE_ERR_CODE_CORRUPT on failure.
static inline int int_minlz_decode(uint8_t* dst, size_t dst_len,
                                  const uint8_t* src, size_t src_len) {
    size_t d = 0, s = 0;
    int offset = 1;

    // Constants
    enum {
        TAG_LITERAL = 0x00,
        TAG_COPY1   = 0x01,
        TAG_COPY2   = 0x02,

        MIN_COPY2_OFFSET     = 64,
        MIN_COPY3_OFFSET     = 65536,
    };

    // Helper functions for reading multi-byte values (little-endian)
    #define LOAD8(b, i)  ((b)[i])
    #define LOAD16(b, i) ((uint16_t)(b)[i] | ((uint16_t)(b)[(i)+1] << 8))
    #define LOAD32(b, i) ((uint32_t)(b)[i] | ((uint32_t)(b)[(i)+1] << 8) | \
                         ((uint32_t)(b)[(i)+2] << 16) | ((uint32_t)(b)[(i)+3] << 24))
    #define STORE32(b, i, v) do { \
        (b)[i] = (v) & 0xFF; \
        (b)[(i)+1] = ((v) >> 8) & 0xFF; \
        (b)[(i)+2] = ((v) >> 16) & 0xFF; \
        (b)[(i)+3] = ((v) >> 24) & 0xFF; \
    } while(0)

    // Little Endian, unaligned platforms:
    // #define LOAD16(b, i) *(uint16_t*)(b + i)
    // #define LOAD32(b, i) *(uint32_t*)(b + i)
    // #define STORE32(b, i, v) ((uint32_t*)(b + i))[0] = v

  
    // Main decoding loop for when we have enough bytes
    while (s < src_len - 11) {
        int length;
        switch (LOAD8(src, s) & 0x03) {
        case TAG_LITERAL: {
            uint8_t v = LOAD8(src, s);
            uint8_t x = v >> 3;
            if (x < 29) {
                s++;
                length = x + 1;
            } else if (x == 29) {
                length = 30 + LOAD8(src, s + 1);
                s += 2;
            } else if (x == 30) {
                length = 30 + LOAD16(src, s + 1);
                s += 3;
            } else { // x == 31
                length = 30 + (LOAD32(src, s) >> 8);
                s += 4;
            }

            if (v & 4) {
                goto do_copy;
            }
            if (length > (int)(dst_len - d) || length > (int)(src_len - s)) {
                return DECODE_ERR_CODE_CORRUPT;
            }

            memcpy(dst + d, src + s, length);
            d += length;
            s += length;
            continue;
        }

        case TAG_COPY1:
            length = (LOAD8(src, s) >> 2) & 15;
            offset = (LOAD16(src, s) >> 6) + 1;
            if (length == 15) {
                length = LOAD8(src, s + 2) + 18;
                s += 3;
            } else {
                length += 4;
                s += 2;
            }
            break;

        case TAG_COPY2:
            length = LOAD8(src, s) >> 2;
            offset = LOAD16(src, s + 1);
            if (length <= 60) {
                length += 4;
                s += 3;
            } else {
                switch (length) {
                case 61:
                    length = LOAD8(src, s + 3) + 64;
                    s += 4;
                    break;
                case 62:
                    length = LOAD16(src, s + 3) + 64;
                    s += 5;
                    break;
                case 63:
                    length = (LOAD32(src, s + 2) >> 8) + 64;
                    s += 6;
                    break;
                }
            }
            offset += MIN_COPY2_OFFSET;
            break;

        case 0x3: {
            uint32_t val = LOAD32(src, s);
            int is_copy3 = (val & 4) != 0;
            int lit_len = (val >> 3) & 3;

            if (!is_copy3) {
                length = 4 + ((val >> 5) & 7);
                offset = ((val >> 8) & 65535) + MIN_COPY2_OFFSET;
                s += 3;
                lit_len++;
            } else {
                uint32_t length_tmp = (val >> 5) & 63;
                offset = (val >> 11) + MIN_COPY3_OFFSET;
                if (length_tmp < 61) {
                    length = length_tmp + 4;
                    s += 4;
                } else if (length_tmp == 61) {
                    length = LOAD8(src, s + 4) + 64;
                    s += 5;
                } else if (length_tmp == 62) {
                    length = LOAD16(src, s + 4) + 64;
                    s += 6;
                } else { // 63
                    length = (LOAD32(src, s + 3) >> 8) + 64;
                    s += 7;
                }
            }

            if (lit_len > 0) {
                if (dst_len - d < 4) {
                    return DECODE_ERR_CODE_CORRUPT;
                }
                STORE32(dst, d, LOAD32(src, s));
                s += lit_len;
                d += lit_len;
            }
            break;
        }
        }

    do_copy:
        if (d < (size_t)offset || length > (int)(dst_len - d)) {
            return DECODE_ERR_CODE_CORRUPT;
        }

        if (offset > length) {
            memcpy(dst + d, dst + d - offset, length);
        } else {
            for (int i = 0; i < length; i++) {
                dst[d + i] = dst[d + i - offset];
            }
        }
        d += length;
    }

    // Handle remaining bytes with extra checks
    while (s < src_len) {
        int length;
        switch (LOAD8(src, s) & 0x03) {
        case TAG_LITERAL: {
            uint8_t v = LOAD8(src, s);
            uint8_t x = v >> 3;
            if (x < 29) {
                s++;
                length = x + 1;
            } else if (x == 29) {
                s += 2;
                if (s > src_len) return DECODE_ERR_CODE_CORRUPT;
                length = LOAD8(src, s - 1) + 30;
            } else if (x == 30) {
                s += 3;
                if (s > src_len) return DECODE_ERR_CODE_CORRUPT;
                length = LOAD16(src, s - 2) + 30;
            } else { // x == 31
                s += 4;
                if (s > src_len) return DECODE_ERR_CODE_CORRUPT;
                length = (LOAD32(src, s - 4) >> 8) + 30;
            }

            if (v & 4) {
                goto do_copy2;
            }
            if (length > (int)(dst_len - d) || length > (int)(src_len - s)) {
                return DECODE_ERR_CODE_CORRUPT;
            }

            memcpy(dst + d, src + s, length);
            d += length;
            s += length;
            continue;
        }

        case TAG_COPY1:
            s += 2;
            if (s > src_len) return DECODE_ERR_CODE_CORRUPT;
            length = (LOAD8(src, s - 2) >> 2) & 15;
            offset = (LOAD16(src, s - 2) >> 6) + 1;
            if (length == 15) {
                s++;
                if (s > src_len) return DECODE_ERR_CODE_CORRUPT;
                length = LOAD8(src, s - 1) + 18;
            } else {
                length += 4;
            }
            break;

        case TAG_COPY2:
            s += 3;
            if (s > src_len) return DECODE_ERR_CODE_CORRUPT;
            length = LOAD8(src, s - 3) >> 2;
            offset = LOAD16(src, s - 2);
            if (length <= 60) {
                length += 4;
            } else {
                switch (length) {
                case 61:
                    s++;
                    if (s > src_len) return DECODE_ERR_CODE_CORRUPT;
                    length = LOAD8(src, s - 1) + 64;
                    break;
                case 62:
                    s += 2;
                    if (s > src_len) return DECODE_ERR_CODE_CORRUPT;
                    length = LOAD16(src, s - 2) + 64;
                    break;
                case 63:
                    s += 3;
                    if (s > src_len) return DECODE_ERR_CODE_CORRUPT;
                    length = (LOAD32(src, s - 3) >> 8) + 64;
                    break;
                }
            }
            offset += MIN_COPY2_OFFSET;
            break;

        case 0x3: {
            s += 4;
            if (s > src_len) return DECODE_ERR_CODE_CORRUPT;
            uint32_t val = LOAD32(src, s - 4);
            int is_copy3 = (val & 4) != 0;
            int lit_len = (val >> 3) & 3;

            if (!is_copy3) {
                length = 4 + ((val >> 5) & 7);
                offset = ((val >> 8) & 65535) + MIN_COPY2_OFFSET;
                s--;
                lit_len++;
            } else {
                uint32_t length_tmp = (val >> 5) & 63;
                offset = (val >> 11) + MIN_COPY3_OFFSET;
                if (length_tmp >= 61) {
                    switch (length_tmp) {
                    case 61:
                        s++;
                        if (s > src_len) return DECODE_ERR_CODE_CORRUPT;
                        length = LOAD8(src, s - 1) + 64;
                        break;
                    case 62:
                        s += 2;
                        if (s > src_len) return DECODE_ERR_CODE_CORRUPT;
                        length = LOAD16(src, s - 2) + 64;
                        break;
                    case 63:
                        s += 3;
                        if (s > src_len) return DECODE_ERR_CODE_CORRUPT;
                        length = (LOAD32(src, s - 3) >> 8) + 64;
                        break;
                    }
                } else {
                    length = length_tmp + 4;
                }
            }

            if (lit_len > 0) {
                if (lit_len > (int)(dst_len - d) || s + lit_len > src_len) {
                    return DECODE_ERR_CODE_CORRUPT;
                }
                memcpy(dst + d, src + s, lit_len);
                d += lit_len;
                s += lit_len;
            }
            break;
        }
        }

    do_copy2:
        if (offset <= 0 || d < (size_t)offset || length > (int)(dst_len - d)) {
            return DECODE_ERR_CODE_CORRUPT;
        }

        if (offset > length) {
            memcpy(dst + d, dst + d - offset, length);
        } else {
            for (int i = 0; i < length; i++) {
                dst[d + i] = dst[d + i - offset];
            }
        }
        d += length;
    }

    if (d != dst_len) {
        return DECODE_ERR_CODE_CORRUPT;
    }
    return 0;
}

// minlz_decode_block decodes src into dst.
// dst must have capacity for the decompressed block, up to 8MB.
static inline int minlz_decode_block(uint8_t* dst, size_t dst_cap,
                                 const uint8_t* src, size_t src_len) {
    const size_t maxBlockSize = 8 << 20; // 8MB

    // Check if source is empty
    if (src_len == 0) {
        return DECODE_ERR_CODE_CORRUPT;
    }

    // Check if first byte is 0
    if (src[0] != 0) {
        return DECODE_ERR_CODE_CORRUPT;
    }

    // If 0 is the only byte, return size 0
    if (src_len == 1 && src[0] == 0) {
        return 0;
    }

    // Skip the first byte
    src++;
    src_len--;

    // Read the expected length of uncompressed data (uvarint encoded)
    size_t wantSize = 0;
    for (uint32_t i = 0; i < 10; i++) { // Limit to 10 bytes (70 bits max)
        if (src_len == 0) {
            return DECODE_ERR_CODE_CORRUPT;
        }
        uint8_t v = src[0];
        wantSize |= (size_t)(v & 0x7f) << (7 * i);
        src++;
        src_len--;
        if ((v & 0x80) == 0) {
            break;
        }
        if (i == 9 && (v & 0x80) != 0) {
            // Value exceeds 64 bits
            return DECODE_ERR_CODE_CORRUPT;
        }
    }

    // Check if size exceeds maximum block size
    if (wantSize > maxBlockSize) {
        return DECODE_ERR_CODE_CORRUPT;
    }

    // If size is 0, return 0
    if (wantSize == 0) {
        return 0;
    }

    // Validate decompressed size and destination capacity
    if (wantSize < src_len || wantSize > dst_cap) {
        return DECODE_ERR_CODE_CORRUPT;
    }

    // Perform the actual decoding
    if (int_minlz_decode(dst, wantSize, src, src_len) == 0) {
        return (int)wantSize;
    }

    return DECODE_ERR_CODE_CORRUPT;
}

#endif // MINLZ_H
