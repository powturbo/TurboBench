#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "map.h"
#include "util.h"

int seed;
int bits = 1;
int fuzz = 20;


void fuzzbuf(char *file, size_t size)
{
	int i;
	for (i = 0; i < fuzz; i++) {
		unsigned r = rand();

		int mode = r & 3;
		r >>= 2;
		int offset = r % (size*8);

		int w;
		switch (mode) { 
		case 0:
		case 1: 
			for (w = offset; w < offset + bits; w++)
				file[w / 8] ^= (1U << (w%8));
			break;
		case 2: 
			for (w = offset; w < offset + bits; w++)
				file[w / 8] |= (1U << (w%8));
			break;
		case 3:  {
			unsigned val = rand();
			for (w = offset; w < offset + bits; w++)
				file[w / 8] = (file[w / 8] & ~(1U << (w%8))) |
					      ((val >> (w-offset)) << (w%8));
			break;
		}
		}
	}
}

int runs1 = 5000;
int runs2 = 100;
	
#define BENCH(name,names,fn,arg) \
	int k;			 \
	size_t orig_outlen = outlen;		\
	for (k = 0; k < runs1; k++) {					\
		outlen = orig_outlen;					\
		err = c_##name(map, size, out, &outlen, arg);		\
		if (err)						\
			printf("%s: compression of %s failed: %d\n",	\
			       names, fn, err);				\
		for (i = 0; i < runs2; i++) {				\
			fuzzbuf(out, outlen);				\
			d_##name(out, outlen, buf2, size, arg);		\
		}							\
	}

	
#include "glue.c"

       
void fuzzfile(char *fn)
{
	size_t size;

	char *map = mapfile_flag(fn, O_RDONLY, &size, MAP_PRIVATE);
	if (!map) {
		perror(fn);
		return;
	}

#ifdef SNAPREF
	printf("snappy-ref\n");
	test_snapref(map, size, fn);
#endif

	printf("snappy\n");
	test_snappy(map, size, fn);

#ifdef COMP
	printf("lzo\n");
	test_lzo(map, size, fn);
	printf("zlib1\n");
	test_zlib(map, size, fn, 1);
	printf("zlib3\n");
	test_zlib(map, size, fn, 3);
#endif

	unmap_file(map, size);
}

void usage(void)
{
	printf("usage\n");
	exit(1);
}

int main(int ac, char **av)
{
	//seed = time(0);
	seed = 1312816253;
	printf("seed %u\n", seed);
	srand(seed);

	

	while (*++av)
		fuzzfile(*av);

	return 0;
}
