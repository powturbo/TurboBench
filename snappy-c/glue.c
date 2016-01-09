#include <stdlib.h>
#include "snappy.h"

#ifdef COMP
#include "../comp/zconf.h"
#include "../comp/zlib.h"
#ifdef KERNEL_LZO
#include "../comp/lzo/lzo.h"
#endif
#include "../comp/lzf.h"
#include "../comp/quicklz.h"
#include "../comp/fastlz.h"
#ifndef KERNEL_LZO
#include "../comp/minilzo-2.06/minilzo.h"
#endif
#include "../comp/lz4/lz4.h"
#endif

static inline int c_snappy(char *map, size_t size, char *out, size_t *outlen, void *a)
{
	return snappy_compress((struct snappy_env *)a, map, size, out, outlen);
}

static inline int d_snappy(char *out, size_t outlen, char *buf2, size_t size, void *a)
{
	return snappy_uncompress(out, outlen, buf2);
}

void test_snappy(char *map, size_t size, char *fn)
{
	int i;
	size_t outlen = snappy_max_compressed_length(size);
	int err;       
	char *out = xmalloc(outlen);
	char *buf2 = xmalloc(size);

	struct snappy_env env;
	snappy_init_env(&env);

	BENCH(snappy, "snappy", fn, &env);

	free(out);
	free(buf2);
}

#ifdef COMP

static inline int c_lzo(char *map, size_t size, char *out, size_t *outlen, void *a)
{
	return lzo1x_1_compress((u8*)map, size, (u8*)out, outlen, a);
}

static inline int d_lzo(char *out, size_t outlen, char *buf2, size_t size, void *a)
{
        return lzo1x_decompress_safe((u8*)out, outlen, (u8*)buf2, &size
#ifndef KERNEL_LZO
				     ,NULL
#endif
		);
}

void test_lzo(char *map, size_t size, char *fn)
{
	int i;
	int err;       
#ifdef KERNEL_LZO
	size_t outlen = lzo1x_worst_compress(size);
#else
	size_t outlen = size * 2;
#endif
	char *out = xmalloc(outlen);
	char *buf2 = xmalloc(size);

	char lzo_wmem[LZO1X_MEM_COMPRESS];
	memset(lzo_wmem, 0, LZO1X_MEM_COMPRESS);

	BENCH(lzo, "lzo", fn, lzo_wmem);

	free(out);
	free(buf2);
}

struct state {
	struct z_stream_s comp;
	struct z_stream_s de;
};

static inline int c_zlib(char *map, size_t size, char *out, size_t *outlen, void *a)
{
	struct state *s = a;
	int ret;
	s->comp.next_in = (unsigned char *)map;
	s->comp.avail_in = size;
	s->comp.total_in = 0;
	s->comp.next_out = (unsigned char *)out;
	s->comp.avail_out = *outlen;
	s->comp.total_out = 0;
	ret = zlib_deflate(&s->comp, Z_FINISH);
	if (ret != Z_STREAM_END)
		return ret;

	ret = zlib_deflateReset(&s->comp);
	if (ret != Z_OK) 
		return ret;

	*outlen = *outlen - s->comp.avail_out;
	return 0;
}

static inline int d_zlib(char *out, size_t outlen, char *buf2, size_t size, void *a)
{
	struct state *s = a;
	int ret;

	s->de.next_in = (unsigned char *)out;
	s->de.avail_in = outlen;
	s->de.next_out = (unsigned char *)buf2;
	s->de.avail_out = size;
	s->de.total_in = 0;
	s->de.total_out = 0;
	ret = zlib_inflate(&s->de, Z_FINISH);
	if (ret != Z_STREAM_END)
		return ret;
	ret = zlib_inflateReset(&s->de);
	if (ret != Z_OK) 
		return ret;
	return 0;
}

void test_zlib(char *map, size_t size, char *fn, int level)
{
	char name[30];
	int i;
	int err;       
	size_t outlen = size * 2; /* XXX? */
	char *out = xmalloc(outlen);
	char *buf2 = xmalloc(size);
	struct state state;
	memset(&state, 0, sizeof state);

	state.comp.workspace = calloc(zlib_deflate_workspacesize(MAX_WBITS, MAX_MEM_LEVEL), 1);
	
	err = zlib_deflateInit(&state.comp, level);
	if (err != Z_OK) {
		printf("zlib_deflateinit failed: %d\n", err);
		exit(1);
	}

	state.de.workspace = calloc(zlib_inflate_workspacesize(), 1);
	err = zlib_inflateInit(&state.de);
	if (err != Z_OK) {
		printf("zlib_inflateinit failed: %d\n", err);
		exit(1);
	}
	
	sprintf(name, "zlib%d", level);	
	BENCH(zlib, name, fn, &state);

	zlib_deflateEnd(&state.comp);
	zlib_inflateEnd(&state.de);

	free(state.de.workspace);
	free(state.comp.workspace);

	free(out);
	free(buf2);
}


static inline int c_lzf(char *map, size_t size, char *out, size_t *outlen, void *a)
{
	size_t n;
	n = lzf_compress(map, size, out, *outlen);
	if (n == 0)
		return -1;
	*outlen = n;
	return 0;
}

static inline int d_lzf(char *out, size_t outlen, char *buf2, size_t size, void *a)
{
	if (lzf_decompress(out, outlen, buf2, size) == 0)
		return -1;
	return 0;
}

void test_lzf(char *map, size_t size, char *fn)
{
	int i;
	int err;       
	size_t outlen = size * 2;
	char *out = xmalloc(outlen);
	char *buf2 = xmalloc(size);

	BENCH(lzf, "lzf", fn, NULL);

	free(out);
	free(buf2);
}

static inline int c_quicklz(char *map, size_t size, char *out, size_t *outlen, void *a)
{
	if (qlz_compress(map, out, size, a) == 0)
		return -1;

	*outlen = qlz_size_compressed(out);
	return 0;
}

static inline int d_quicklz(char *out, size_t outlen, char *buf2, size_t size, void *a)
{
	if (qlz_decompress(out, buf2, a) == 0)
		return -1;
	return 0;
}

void test_quicklz(char *map, size_t size, char *fn)
{
	int i;
	int err;       
	size_t outlen = size * 2;
	char *out = xmalloc(outlen);
	char *buf2 = xmalloc(size);
	qlz_state_compress state;
	memset(&state, 0, sizeof(qlz_state_compress));

	BENCH(quicklz, "qlz", fn, &state);

	free(out);
	free(buf2);
}


static inline int c_fastlz(char *map, size_t size, char *out, size_t *outlen, void *a)
{
	*outlen = fastlz_compress(map, size, out);
	return 0;
}

static inline int d_fastlz(char *out, size_t outlen, char *buf2, size_t size, void *a)
{
	if (fastlz_decompress(out, outlen, buf2, size) == 0)
		return -1;
	return 0;
}

void test_fastlz(char *map, size_t size, char *fn)
{
	int i;
	int err;       
	size_t outlen = size * 2;
	char *out = xmalloc(outlen);
	char *buf2 = xmalloc(size);

	BENCH(fastlz, "fastlz", fn, NULL);

	free(out);
	free(buf2);
}

static inline int c_lz4(char *map, size_t size, char *out, size_t *outlen, void *a)
{
	*outlen = LZ4_compress(map, out, size);
	return 0;
}

static inline int d_lz4(char *out, size_t outlen, char *buf2, size_t size, void *a)
{
	if (LZ4_uncompress(out, buf2, outlen) < 0)
		return -1;
	return 0;
}

void test_lz4(char *map, size_t size, char *fn)
{
	int i;
	int err;       
	size_t outlen = size * 2;
	char *out = xmalloc(outlen);
	char *buf2 = xmalloc(size);

	BENCH(fastlz, "lz4", fn, NULL);

	free(out);
	free(buf2);
}


#endif

#ifdef SNAPREF

#include "snappy-c.h"

static inline int c_snapref(char *map, size_t size, char *out, size_t *outlen, void *a)
{
	return snappyc_compress(map, size, out, outlen);
}

static inline int d_snapref(char *out, size_t outlen, char *buf2, size_t size, void *a)
{
	return snappyc_uncompress(out, outlen, buf2, &size);
}

void test_snapref(char *map, size_t size, char *fn)
{
	int i;
	size_t outlen = snappy_max_compressed_length(size);
	int err;       
	char *out = xmalloc(outlen);
	char *buf2 = xmalloc(size);

	BENCH(snappy, "snapref", fn, NULL);

	free(out);
	free(buf2);
}
#endif
