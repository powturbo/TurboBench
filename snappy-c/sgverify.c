/* Test sg interfaces */

#ifdef SG

#include <assert.h>
#include <sys/uio.h>
#include <stdlib.h>
#include "snappy.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "util.h"
#include <sys/stat.h>
#include "map.h"

#define REPEAT 20
#define N 5000

size_t sum_iov(struct iovec *iov, int n)
{
	size_t len = 0;
	int i;
	for (i = 0; i < n; i++)
		len += iov[i].iov_len;
	return len;
}

static int rnd_seq;

int rand_seq(void)
{	
	++rnd_seq;
	return rand();
}

void *iov_to_buf(struct iovec *iov, int n, size_t *len)
{
	*len = sum_iov(iov, n);
	void *p = malloc(*len);
	int i;
	unsigned offset = 0;
	for (i = 0; i < n; i++) {
		memcpy(p + offset, iov[i].iov_base, iov[i].iov_len);
		offset += iov[i].iov_len;
	}
	return p;
}

void test_read(char *buf, int len)
{
	int i;
	for (i = 0; i < len; i++)
		asm volatile("" :: "r" (buf[i]) : "memory");
}

void print_mismatch(char *a, char *b, int size)
{
	int j;
	int printed = 0;
	for (j = 0; j < size; j++)
		if (a[j] != b[j]) {
			printf("%d: %x vs %x\n",
			       j, 
			       ((unsigned char *)a)[j],
			       ((unsigned char *)b)[j]);
			if (printed++ > 10)
				break;
		}
}

int main(int ac, char **av)
{
	struct snappy_env env;
	snappy_init_env_sg(&env, true);

	srand(99);
#if 0
	int j;
	for (j = 0; j < 60016; j++)
		rand_seq();
#endif

	while (*++av) { 
		size_t st_size;
		char *map = mapfile(*av, O_RDONLY, &st_size);
		if (!map) {
			perror(*av);
			continue;
		}

		int k;
		for (k = 0; k < REPEAT; k++) { 
			struct iovec in_iov[N];
			int iv = 0;
			size_t size = st_size;
			size_t offset = 0;
			int err;

			unsigned rnd_seq_start = rnd_seq;

			while (size > 0 && iv < N - 1) {
				size_t len = rand_seq() % size + 1;

				if (len > size)
					len = size;
				in_iov[iv].iov_base = malloc(len);
				in_iov[iv].iov_len = len;
				memcpy(in_iov[iv].iov_base, 
				       map + offset,
				       len);
				size -= len;
				offset += len;
				iv++;
			}
			in_iov[iv].iov_base = malloc(size);
			in_iov[iv].iov_len = size;
			iv++;

			int j;
			for (j = 0; j < iv; j++)
				test_read(in_iov[j].iov_base, in_iov[j].iov_len); 
			
			assert (sum_iov(in_iov, iv) == st_size);		
				       		
			struct iovec out_iov[N];
			int ov = 0;
			size = snappy_max_compressed_length(st_size);
			while (size > 0 && ov < N - 1) {
				size_t len = rand_seq() % size + 1;
				if (len > size)
					len = size;
				out_iov[ov].iov_len = len;
				out_iov[ov].iov_base = malloc(len);
				size -= len;
				ov++;
			}
			out_iov[ov].iov_base = malloc(size);
			out_iov[ov].iov_len = size;
			ov++;
			
			assert (sum_iov(out_iov, ov) == 
				snappy_max_compressed_length(st_size));

			size_t outlen_linear;
			char *out_linear = malloc(snappy_max_compressed_length(st_size));
			err = snappy_compress(&env, map, st_size, out_linear, &outlen_linear);
			if (err < 0)
				printf("linear compression of %s failed: %s\n", *av, strerror(-err));

			size_t outlen;
		
			err = snappy_compress_iov(&env, in_iov, iv, st_size, 
						      out_iov, &ov, &outlen);
			if (err < 0) 
				printf("compression of %s failed: %s\n", *av, strerror(-err));

			assert(outlen == outlen_linear);

			int w;
			offset = 0;
			for (w = 0; w < ov; w++) {
				if (memcmp(out_iov[w].iov_base, out_linear + offset, out_iov[w].iov_len)) {
					printf("compressed sg %d does not match\n", w);
					print_mismatch(out_iov[w].iov_base, out_linear + offset, 
							out_iov[w].iov_len);
					break;
				}
				offset += out_iov[w].iov_len;
			}

			char *obuf = malloc(st_size);

			err = snappy_uncompress_iov(out_iov, ov, outlen, obuf);
			if (err < 0)
				printf("uncompression of %s failed: %s\n", *av, strerror(-err));
		
			if (memcmp(obuf, map, st_size)) {
				printf("comparison of %s failed, olen %zu, orig %zu, rnd_seq %d\n", *av,
				       outlen, st_size, rnd_seq_start);
				print_mismatch(obuf, map, st_size);
			}
			
			for (w = 0; w < iv; w++) 
				free(in_iov[w].iov_base);
			for (w = 0; w < ov; w++)
				free(out_iov[w].iov_base);
			free(obuf);
			free(out_linear);
		}

		unmap_file(map, st_size);
	}
	return 0;
}	
	
#else
int main(void) { return 0; } 	
#endif
