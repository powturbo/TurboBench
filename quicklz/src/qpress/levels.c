/*
Very dirty method to offer runtime selection of QuickLZ compression level

    for each compression level L in (1, 2, 3) do
    {
       redefine publics+privates of quicklz.c by appending compression level to their names
       define QLZ_COMPRESSION_LEVEL to L
       include quicklz.c
       undefine publics+privates
    }
*/

#include <string.h>

#define QLZ_STREAMING_BUFFER 0
//#define QLZ_STREAMING_BUFFER 100000
//#define QLZ_STREAMING_BUFFER 1000000

#define QLZ_MEMORY_SAFE

// ---------------------------------------
#define fast_read FAST_READ_1
#define fast_write FAST_WRITE_1
#define hash_func HASH_FUNC_1
#define memcpy_up MEMCPY_UP_1
#define qlz_compress_core QLZ_COMPRESS_CORE_1
#define qlz_decompress_core QLZ_DECOMPRESS_CORE_1
#define qlz_decompress QLZ_DECOMPRESS_1
#define qlz_compress QLZ_COMPRESS_1
#define qlz_size_decompressed QLZ_SIZE_DECOMPRESSED_1
#define qlz_size_compressed QLZ_SIZE_COMPRESSED_1
#define qlz_get_setting QLZ_GET_SETTING_1
#define update_hash_upto UPDATE_HASH_UPTO_1
#define update_hash UPDATE_HASH_1
#define qlz_hash_entry QLZ_HASH_ENTRY_1
#define reset_state RESET_STATE_1
#define qlz_hash_compress QLZ_HASH_COMPRESS_1
#define qlz_hash_decompress QLZ_HASH_DECOMPRESS_1

#define QLZ_COMPRESSION_LEVEL 1

#include "quicklz.c"

size_t QLZ_SCRATCH_COMPRESS_1(void)
{
    return QLZ_SCRATCH_COMPRESS;
}
size_t QLZ_SCRATCH_DECOMPRESS_1(void)    
{
    return QLZ_SCRATCH_DECOMPRESS;
}

#undef X86X64
#undef QLZ_HEADER
#undef QLZ_SCRATCH_COMPRESS
#undef QLZ_SCRATCH_DECOMPRESS
#undef fast_read
#undef fast_write
#undef hash_func
#undef memcpy_up
#undef qlz_compress_core
#undef qlz_decompress_core
#undef qlz_decompress
#undef qlz_compress
#undef qlz_size_decompressed
#undef qlz_size_compressed
#undef qlz_get_setting
#undef QLZ_COMPRESSION_LEVEL
#undef QLZ_POINTERS
#undef QLZ_HASH_VALUES
#undef update_hash_upto 
#undef update_hash 
#undef qlz_hash_entry
#undef reset_state
#undef qlz_hash_compress
#undef qlz_hash_decompress

// ---------------------------------------
#define fast_read FAST_READ_2
#define fast_write FAST_WRITE_2
#define hash_func HASH_FUNC_2
#define memcpy_up MEMCPY_UP_2
#define qlz_compress_core QLZ_COMPRESS_CORE_2
#define qlz_decompress_core QLZ_DECOMPRESS_CORE_2
#define qlz_decompress QLZ_DECOMPRESS_2
#define qlz_compress QLZ_COMPRESS_2
#define qlz_size_decompressed QLZ_SIZE_DECOMPRESSED_2
#define qlz_size_compressed QLZ_SIZE_COMPRESSED_2
#define qlz_get_setting QLZ_GET_SETTING_2
#define update_hash_upto UPDATE_HASH_UPTO_2
#define update_hash UPDATE_HASH_2
#define qlz_hash_entry QLZ_HASH_ENTRY_2
#define reset_state RESET_STATE_2
#define qlz_hash_compress QLZ_HASH_COMPRESS_2
#define qlz_hash_decompress QLZ_HASH_DECOMPRESS_2

#define QLZ_COMPRESSION_LEVEL 2

#include "quicklz.c"

size_t QLZ_SCRATCH_COMPRESS_2(void)
{
    return QLZ_SCRATCH_COMPRESS;
}
size_t QLZ_SCRATCH_DECOMPRESS_2(void)
{
    return QLZ_SCRATCH_DECOMPRESS;
}

#undef X86X64
#undef QLZ_HEADER
#undef QLZ_SCRATCH_COMPRESS
#undef QLZ_SCRATCH_DECOMPRESS
#undef fast_read
#undef fast_write
#undef hash_func
#undef memcpy_up
#undef qlz_compress_core
#undef qlz_decompress_core
#undef qlz_decompress
#undef qlz_compress
#undef qlz_size_decompressed
#undef qlz_size_compressed
#undef qlz_get_setting
#undef QLZ_COMPRESSION_LEVEL
#undef QLZ_POINTERS
#undef QLZ_HASH_VALUES
#undef update_hash_upto 
#undef update_hash 
#undef qlz_hash_entry
#undef reset_state
#undef qlz_hash_compress
#undef qlz_hash_decompress

// ---------------------------------------
#define fast_read FAST_READ_3
#define fast_write FAST_WRITE_3
#define hash_func HASH_FUNC_3
#define memcpy_up MEMCPY_UP_3
#define qlz_compress_core QLZ_COMPRESS_CORE_3
#define qlz_decompress_core QLZ_DECOMPRESS_CORE_3
#define qlz_decompress QLZ_DECOMPRESS_3
#define qlz_compress QLZ_COMPRESS_3
#define qlz_size_decompressed QLZ_SIZE_DECOMPRESSED_3
#define qlz_size_compressed QLZ_SIZE_COMPRESSED_3
#define qlz_get_setting QLZ_GET_SETTING_3
#define update_hash_upto UPDATE_HASH_UPTO_3
#define update_hash UPDATE_HASH_3
#define qlz_hash_entry QLZ_HASH_ENTRY_3
#define reset_state RESET_STATE_3
#define qlz_hash_compress QLZ_HASH_COMPRESS_3
#define qlz_hash_decompress QLZ_HASH_DECOMPRESS_3

#define QLZ_COMPRESSION_LEVEL 3

#include "quicklz.c"

size_t QLZ_SCRATCH_COMPRESS_3(void)
{
    return QLZ_SCRATCH_COMPRESS;
}
size_t QLZ_SCRATCH_DECOMPRESS_3(void)
{
    return QLZ_SCRATCH_DECOMPRESS;
}

size_t QLZ_SIZE_COMPRESSED(char *source)
{
    return QLZ_SIZE_COMPRESSED_3(source);
}

size_t QLZ_SIZE_DECOMPRESSED(char *source)
{
    return QLZ_SIZE_DECOMPRESSED_3(source);
}

#undef QLZ_HEADER
#undef QLZ_SCRATCH_COMPRESS
#undef QLZ_SCRATCH_DECOMPRESS
#undef fast_read
#undef fast_write
#undef hash_func
#undef memcpy_up
#undef qlz_compress_core
#undef qlz_decompress_core
#undef qlz_decompress
#undef qlz_compress
#undef qlz_size_decompressed
#undef qlz_size_compressed
#undef qlz_get_setting
#undef QLZ_COMPRESSION_LEVEL
#undef QLZ_POINTERS
#undef QLZ_HASH_VALUES
#undef update_hash_upto 
#undef update_hash 
#undef qlz_hash_entry
#undef reset_state
#undef qlz_hash_compress
#undef qlz_hash_decompress

static size_t max2(size_t a, size_t b) 
{
    return a > b ? a : b;
}

size_t QLZ_SCRATCH_COMPRESS(void)
{
    return max2(QLZ_SCRATCH_COMPRESS_1(), max2(QLZ_SCRATCH_COMPRESS_2(), QLZ_SCRATCH_COMPRESS_3()));
}

size_t QLZ_SCRATCH_DECOMPRESS(void)
{
    return max2(QLZ_SCRATCH_DECOMPRESS_1(), max2(QLZ_SCRATCH_DECOMPRESS_2(), QLZ_SCRATCH_DECOMPRESS_3()));
}


size_t QLZ_COMPRESS(const void *source, char *destination, size_t size, unsigned int level, char *scratch)
{
    if(level == 1)
        return QLZ_COMPRESS_1(source, destination, size, scratch);
    else if(level == 2)
        return QLZ_COMPRESS_2(source, destination, size, scratch);
    else if(level == 3)
        return QLZ_COMPRESS_3(source, destination, size, scratch);
    else
        return 0;
}
size_t QLZ_DECOMPRESS(const char *source, void *destination, char *scratch)
{
    int level = ((source[0] >> 2) & 3);

    if(level == 1)
        return QLZ_DECOMPRESS_1(source, destination, scratch);
    else if(level == 2)
        return QLZ_DECOMPRESS_2(source, destination, scratch);
    else if(level == 3)
        return QLZ_DECOMPRESS_3(source, destination, scratch);
    else
        return 0;
}

