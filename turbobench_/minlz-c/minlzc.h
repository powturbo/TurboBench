#ifndef MINLZ_H
#define MINLZ_H

#include <stdint.h>
#include <string.h>

// ### Constants

// **Exported Constants**
#define MINLZ_TAG_LITERAL 0x00
#define MINLZ_TAG_COPY1 0x01
#define MINLZ_TAG_COPY2 0x02
#define MINLZ_TAG_COPY3 0x07
#define MINLZ_TAG_COPY2_FUSED 0x03
#define MINLZ_MAX_COPY1_OFFSET 1024
#define MINLZ_MIN_COPY2_OFFSET 64
#define MINLZ_MAX_COPY2_OFFSET (MINLZ_MIN_COPY2_OFFSET + 65535) // 2MiB
#define MINLZ_MAX_BLOCK_SIZE (8 << 20)

// **Internal Constants**
#define _MINLZ_TAG_REPEAT 0x04
#define _MINLZ_COPY_LIT_BITS 2
#define _MINLZ_COPY2_LIT_MAX_LEN (7 + 4) // max length
#define _MINLZ_MAX_COPY3_LITS ((1 << _MINLZ_COPY_LIT_BITS) - 1)

// **Error Codes**
#define MINLZ_ERR_BLOCK_TOO_LARGE -1
#define MINLZ_ERR_DST_TOO_SMALL -2

// ### Helper Functions

/** Load 8-bit value from buffer at index i */
static inline uint8_t _minlz_load8(const uint8_t* b, int i) {
    return b[i];
}

/** Load 16-bit value from buffer at index i (little-endian) */
static inline uint16_t _minlz_load16(const uint8_t* b, int i) {
    return (uint16_t)b[i] | ((uint16_t)b[i+1] << 8);
}

/** Load 32-bit value from buffer at index i (little-endian) */
static inline uint32_t _minlz_load32(const uint8_t* b, int i) {
    return (uint32_t)b[i] | ((uint32_t)b[i+1] << 8) | ((uint32_t)b[i+2] << 16) | ((uint32_t)b[i+3] << 24);
}

/** Load 64-bit value from buffer at index i (little-endian) */
static inline uint64_t _minlz_load64(const uint8_t* b, int i) {
    return (uint64_t)_minlz_load32(b, i) | ((uint64_t)_minlz_load32(b, i+4) << 32);
}

/** Store 8-bit value v into buffer at index idx */
static inline void _minlz_store8(uint8_t* b, int idx, uint8_t v) {
    b[idx] = v;
}

/** Store 16-bit value v into buffer at index idx (little-endian) */
static inline void _minlz_store16(uint8_t* b, int idx, uint16_t v) {
    b[idx] = (uint8_t)v;
    b[idx+1] = (uint8_t)(v >> 8);
}

/** Store 32-bit value v into buffer at index idx (little-endian) */
static inline void _minlz_store32(uint8_t* b, int idx, uint32_t v) {
    b[idx] = (uint8_t)v;
    b[idx+1] = (uint8_t)(v >> 8);
    b[idx+2] = (uint8_t)(v >> 16);
    b[idx+3] = (uint8_t)(v >> 24);
}

// ### Emit Functions

/** Writes a literal chunk to dst and returns the number of bytes written */
static int _minlz_emit_literal(uint8_t* dst, const uint8_t* lit, size_t len) {
    if (len == 0) {
        return 0;
    }
    size_t n = len - 1;
    int i;
    if (n < 29) {
        _minlz_store8(dst, 0, (uint8_t)(n << 3 | MINLZ_TAG_LITERAL));
        i = 1;
    } else if (n < (1 << 8) + 29) {
        _minlz_store8(dst, 1, (uint8_t)(n - 29));
        _minlz_store8(dst, 0, (uint8_t)(29 << 3 | MINLZ_TAG_LITERAL));
        i = 2;
    } else if (n < (1 << 16) + 29) {
        n -= 29;
        dst[2] = (uint8_t)(n >> 8);
        dst[1] = (uint8_t)n;
        dst[0] = (uint8_t)(30 << 3 | MINLZ_TAG_LITERAL);
        i = 3;
    } else if (n < (1 << 24) + 29) {
        n -= 29;
        dst[3] = (uint8_t)(n >> 16);
        dst[2] = (uint8_t)(n >> 8);
        dst[1] = (uint8_t)n;
        dst[0] = (uint8_t)(31 << 3 | MINLZ_TAG_LITERAL);
        i = 4;
    } else {
        return 0; // Literal block too long
    }
    memcpy(dst + i, lit, len);
    return i + (int)len;
}

/** Writes a repeat chunk to dst and returns the number of bytes written */
static int _minlz_emit_repeat(uint8_t* dst, int length) {
    if (length < 30) {
        _minlz_store8(dst, 0, (uint8_t)((length - 1) << 3 | _MINLZ_TAG_REPEAT));
        return 1;
    }
    length -= 30;
    if (length < 256) {
        _minlz_store8(dst, 1, (uint8_t)length);
        _minlz_store8(dst, 0, (uint8_t)(29 << 3 | _MINLZ_TAG_REPEAT));
        return 2;
    }
    if (length < 65536) {
        dst[2] = (uint8_t)(length >> 8);
        dst[1] = (uint8_t)length;
        dst[0] = (uint8_t)(30 << 3 | _MINLZ_TAG_REPEAT);
        return 3;
    }
    dst[3] = (uint8_t)(length >> 16);
    dst[2] = (uint8_t)(length >> 8);
    dst[1] = (uint8_t)length;
    dst[0] = (uint8_t)(31 << 3 | _MINLZ_TAG_REPEAT);
    return 4;
}

/** Encodes a copy operation with a 24-bit offset */
static int _minlz_encode_copy3(uint8_t* dst, int offset, int length, int lits) {
    length -= 4;
    uint32_t encoded = (uint32_t)(offset - 65536) << 11 | MINLZ_TAG_COPY3 | (uint32_t)(lits << 3);
    if (length <= 60) {
        encoded |= (uint32_t)(length << 5);
        _minlz_store32(dst, 0, encoded);
        return 4;
    }
    length -= 60;
    if (length < 256) {
        _minlz_store8(dst, 4, (uint8_t)length);
        encoded |= (61 << 5);
        _minlz_store32(dst, 0, encoded);
        return 5;
    }
    if (length < 65536) {
        encoded |= (62 << 5);
        _minlz_store16(dst, 4, (uint16_t)length);
        _minlz_store32(dst, 0, encoded);
        return 6;
    }
    encoded |= (63 << 5);
    dst[6] = (uint8_t)(length >> 16);
    dst[5] = (uint8_t)(length >> 8);
    dst[4] = (uint8_t)length;
    _minlz_store32(dst, 0, encoded);
    return 7;
}

/** Writes a copy chunk to dst and returns the number of bytes written */
static int _minlz_emit_copy(uint8_t* dst, int offset, int length) {
    if (offset > MINLZ_MAX_COPY2_OFFSET) {
        return _minlz_encode_copy3(dst, offset, length, 0);
    }
    if (offset <= MINLZ_MAX_COPY1_OFFSET) {
        offset--;
        if (length < 15 + 4) {
            uint16_t x = (uint16_t)(offset << 6) | (uint16_t)((length - 4) << 2) | MINLZ_TAG_COPY1;
            _minlz_store16(dst, 0, x);
            return 2;
        }
        if (length < 256 + 18) {
            uint16_t x = (uint16_t)(offset << 6) | (uint16_t)(15 << 2) | MINLZ_TAG_COPY1;
            _minlz_store16(dst, 0, x);
            _minlz_store8(dst, 2, (uint8_t)(length - 18));
            return 3;
        }
        uint16_t x = (uint16_t)(offset << 6) | (uint16_t)(14 << 2) | MINLZ_TAG_COPY1;
        _minlz_store16(dst, 0, x);
        int n = _minlz_emit_repeat(dst + 2, length - 18);
        return 2 + n;
    }
    length -= 4;
    offset -= MINLZ_MIN_COPY2_OFFSET;
    _minlz_store16(dst, 1, (uint16_t)offset);
    if (length <= 60) {
        _minlz_store8(dst, 0, (uint8_t)(length << 2 | MINLZ_TAG_COPY2));
        return 3;
    }
    length -= 60;
    if (length < 256) {
        _minlz_store8(dst, 3, (uint8_t)length);
        _minlz_store8(dst, 0, (uint8_t)(61 << 2 | MINLZ_TAG_COPY2));
        return 4;
    }
    if (length < 65536) {
        dst[4] = (uint8_t)(length >> 8);
        dst[3] = (uint8_t)length;
        dst[0] = (uint8_t)(62 << 2 | MINLZ_TAG_COPY2);
        return 5;
    }
    dst[5] = (uint8_t)(length >> 16);
    dst[4] = (uint8_t)(length >> 8);
    dst[3] = (uint8_t)length;
    dst[0] = (uint8_t)(63 << 2 | MINLZ_TAG_COPY2);
    return 6;
}

/** Emits a 2-byte offset copy with literals */
static int _minlz_emit_copy_lits2(uint8_t* dst, const uint8_t* lits, size_t lits_len, int offset, int length) {
    offset -= MINLZ_MIN_COPY2_OFFSET;
    length -= 4;
    const int copy2_lit_max_len_raw = _MINLZ_COPY2_LIT_MAX_LEN - 4;
    if (length > copy2_lit_max_len_raw) {
        _minlz_store16(dst, 1, (uint16_t)offset);
        _minlz_store8(dst, 0, MINLZ_TAG_COPY2_FUSED | (uint8_t)(copy2_lit_max_len_raw << 5) | (uint8_t)((lits_len - 1) << 3));
        memcpy(dst + 3, lits, lits_len);
        int n = 3 + (int)lits_len;
        return n + _minlz_emit_repeat(dst + n, length - copy2_lit_max_len_raw);
    }
    _minlz_store16(dst, 1, (uint16_t)offset);
    _minlz_store8(dst, 0, MINLZ_TAG_COPY2_FUSED | (uint8_t)(length << 5) | (uint8_t)((lits_len - 1) << 3));
    memcpy(dst + 3, lits, lits_len);
    return 3 + (int)lits_len;
}

/** Emits a 3-byte offset copy with literals */
static int _minlz_emit_copy_lits3(uint8_t* dst, const uint8_t* lits, size_t lits_len, int offset, int length) {
    int n = _minlz_encode_copy3(dst, offset, length, (int)lits_len);
    memcpy(dst + n, lits, lits_len);
    return n + (int)lits_len;
}

// ### Utility Functions

/** Returns the maximum encoded length for a given source length */
static inline int minlz_max_encoded_len(int src_len) {
    if (src_len > MINLZ_MAX_BLOCK_SIZE) {
        return -1;
    }
    if (src_len == 0) {
        return 1;
    }
    return src_len + 2;
}

/** Encodes a uint64_t as a varint and returns the number of bytes written */
static int _minlz_put_uvarint(uint8_t* dst, uint64_t x) {
    int i = 0;
    while (x >= 0x80) {
        dst[i] = (uint8_t)(x | 0x80);
        x >>= 7;
        i++;
    }
    dst[i] = (uint8_t)x;
    return i + 1;
}

/** Encodes src as uncompressed data into dst */
static int _minlz_encode_uncompressed(uint8_t* dst, const uint8_t* src, size_t src_len) {
    if (src_len == 0) {
        dst[0] = 0;
        return 1;
    }
    dst[0] = 0;
    dst[1] = 0;
    memcpy(dst + 2, src, src_len);
    return 2 + (int)src_len;
}


/** Hash function for match finding */
/*static inline uint32_t _minlz_hash_value(uint64_t u, uint8_t h) {
    const uint64_t prime6bytes = 227718039650203ULL;
    return (uint32_t)(((u << (64 - 48)) * prime6bytes) >> ((64 - h) & 63));
}*/

static const uint64_t prime6bytes = 227718039650203ULL;
static inline uint32_t _minlz_hash_value(uint64_t u, uint8_t h) { return (uint32_t)((((u  << (64-48)) * prime6bytes)) >> (64-h)) ; }


/** Internal function to perform the actual compression */
static int _minlz_encode_minlz_block(uint8_t* dst, size_t dst_len, const uint8_t* src, size_t src_len) {
    const int table_bits = 15;
    const size_t max_table_size = 1 << table_bits;
    const int skip_log = 6;
    const int input_margin = 8;
    const int max_copy3_offset = (2 << 20) + 65535;

    uint32_t table[32768] = {0};

    int s_limit = (int)src_len - input_margin;
    int dst_limit = (int)src_len - ((int)src_len >> 5) - 6;

    int next_emit = 0;
    int s = 1;
    uint64_t cv = _minlz_load64(src, s);
    int repeat = 1;
    int d = 0;

    while (1) {
        int candidate = 0;
        while (1) {
            int next_s = s + ((s - next_emit) >> skip_log) + 4;
            if (next_s > s_limit) {
                goto emit_remainder;
            }
            int min_src_pos = s - max_copy3_offset;
            uint32_t hash0 = _minlz_hash_value(cv, table_bits);
            uint32_t hash1 = _minlz_hash_value(cv >> 8, table_bits);
            candidate = (int)table[hash0];
            int candidate2 = (int)table[hash1];
            table[hash0] = (uint32_t)s;
            table[hash1] = (uint32_t)(s + 1);
            uint32_t hash2 = _minlz_hash_value(cv >> 16, table_bits);

            const int check_rep = 1;
            if ((uint32_t)(cv >> (check_rep * 8)) == _minlz_load32(src, s - repeat + check_rep)) {
                int base = s + check_rep;
                int i = base - repeat;
                while (base > next_emit && i > 0 && src[i - 1] == src[base - 1]) {
                    i--;
                    base--;
                }
                if (d + (base - next_emit) > dst_limit) {
                    return 0;
                }
                d += _minlz_emit_literal(dst + d, src + next_emit, (size_t)(base - next_emit));
                candidate = s - repeat + 4 + check_rep;
                s += 4 + check_rep;
                while (s <= s_limit) {
                    uint64_t diff = _minlz_load64(src, s) ^ _minlz_load64(src, candidate);
                    if (diff != 0) {
                        s += __builtin_ctzll(diff) >> 3;
                        break;
                    }
                    s += 8;
                    candidate += 8;
                }
                d += _minlz_emit_repeat(dst + d, s - base);
                next_emit = s;
                if (s >= s_limit) {
                    goto emit_remainder;
                }
                cv = _minlz_load64(src, s);
                continue;
            }

            if (candidate >= min_src_pos && (uint32_t)cv == _minlz_load32(src, candidate)) {
                break;
            }
            candidate = (int)table[hash2];
            if (candidate2 >= min_src_pos && (uint32_t)(cv >> 8) == _minlz_load32(src, candidate2)) {
                table[hash2] = (uint32_t)(s + 2);
                candidate = candidate2;
                s++;
                break;
            }
            table[hash2] = (uint32_t)(s + 2);
            if (candidate >= min_src_pos && (uint32_t)(cv >> 16) == _minlz_load32(src, candidate)) {
                s += 2;
                break;
            }
            cv = _minlz_load64(src, next_s);
            s = next_s;
        }

        while (candidate > 0 && s > next_emit && src[candidate - 1] == src[s - 1]) {
            candidate--;
            s--;
        }

        int base = s;
        repeat = base - candidate;

        s += 4;
        candidate += 4;
        while (s <= (int)src_len - 8) {
            uint64_t diff = _minlz_load64(src, s) ^ _minlz_load64(src, candidate);
            if (diff != 0) {
                s += __builtin_ctzll(diff) >> 3;
                break;
            }
            s += 8;
            candidate += 8;
        }
        int length = s - base;

        if (next_emit != base) {
            if ((base - next_emit) > _MINLZ_MAX_COPY3_LITS || repeat < MINLZ_MIN_COPY2_OFFSET) {
                if (d + (s - next_emit) > dst_limit) {
                    return 0;
                }
                d += _minlz_emit_literal(dst + d, src + next_emit, (size_t)(base - next_emit));
                d += _minlz_emit_copy(dst + d, repeat, length);
            } else if (repeat <= MINLZ_MAX_COPY2_OFFSET) {
                d += _minlz_emit_copy_lits2(dst + d, src + next_emit, (size_t)(base - next_emit), repeat, length);
            } else {
                d += _minlz_emit_copy_lits3(dst + d, src + next_emit, (size_t)(base - next_emit), repeat, length);
            }
        } else {
            d += _minlz_emit_copy(dst + d, repeat, length);
        }

        while (1) {
            next_emit = s;
            if (s >= s_limit) {
                goto emit_remainder;
            }
            uint64_t x = _minlz_load64(src, s - 2);
            if (d > dst_limit) {
                return 0;
            }
            uint32_t m2_hash = _minlz_hash_value(x, table_bits);
            x >>= 16;
            uint32_t curr_hash = _minlz_hash_value(x, table_bits);
            candidate = (int)table[curr_hash];
            table[m2_hash] = (uint32_t)(s - 2);
            table[curr_hash] = (uint32_t)s;
            if (s - candidate > max_copy3_offset || (uint32_t)x != _minlz_load32(src, candidate)) {
                cv = _minlz_load64(src, s + 1);
                s++;
                break;
            }
            repeat = s - candidate;
            base = s;
            s += 4;
            candidate += 4;
            while (s <= (int)src_len - 8) {
                uint64_t diff = _minlz_load64(src, s) ^ _minlz_load64(src, candidate);
                if (diff != 0) {
                    s += __builtin_ctzll(diff) >> 3;
                    break;
                }
                s += 8;
                candidate += 8;
            }
            d += _minlz_emit_copy(dst + d, repeat, s - base);
        }
    }

emit_remainder:
    if (next_emit < (int)src_len) {
        if (d + ((int)src_len - next_emit) > dst_limit) {
            return 0;
        }
        d += _minlz_emit_literal(dst + d, src + next_emit, (size_t)((int)src_len - next_emit));
    }
    return d;
}

// ### Main Encoding Functions

/** Encodes a source buffer into a destination buffer */
int minlz_encode_minlz_block(uint8_t* dst, size_t dst_len, const uint8_t* src, size_t src_len, int level) {
    int n = minlz_max_encoded_len((int)src_len);
    if (n < 0) {
        return MINLZ_ERR_BLOCK_TOO_LARGE;
    }
    if (dst_len < (size_t)n) {
        return MINLZ_ERR_DST_TOO_SMALL;
    }

    const size_t min_non_literal_block_size = 16;
    if (src_len < min_non_literal_block_size) {
        return _minlz_encode_uncompressed(dst, src, src_len);
    }

    dst[0] = 0;
    int d = 1;
    d += _minlz_put_uvarint(dst + d, (uint64_t)src_len);
    int encoded = _minlz_encode_minlz_block(dst + d, dst_len - d, src, src_len);
    if (encoded > 0) {
        return d + encoded;
    }
    return _minlz_encode_uncompressed(dst, src, src_len);
}


#endif // MINLZ_H

