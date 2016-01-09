#define _GNU_SOURCE 1
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "map.h"
#include "snappy.h"
#include "util.h"
#include "compat.h"

#define N 10

#ifdef SIMPLE_PMU
#include "cycles.h"
#define COUNT() unhalted_core()
#else
typedef unsigned long long counter_t;
#define COUNT() __builtin_ia32_rdtsc()
#define sync_core() asm volatile("lfence" ::: "memory")
#endif

#define memeat(x) asm volatile("" :: "r" (x) : "memory")

#define BENCH(name, names, fn, arg)					\
	counter_t a, b, total_comp = 0, total_uncomp = 0;		\
	size_t orig_outlen = outlen;					\
	for (i = 0; i < N + 1; i++) {					\
	    outlen = orig_outlen;					\
	    sync_core();						\
            a = COUNT();						\
	    err = c_##name(map, size, out, &outlen, arg);		\
	    b = COUNT();						\
	    if (i > 0) 							\
		    total_comp += b - a;				\
	    sync_core();						\
	    if (err)							\
		    printf("%s: compression of %s failed: %d\n", names, fn, err); \
	    sync_core();						\
	    a = COUNT();						\
	    err = d_##name(out, outlen, buf2, size, arg);		\
	    b = COUNT();						\
            sync_core();						\
	    if (i > 0)							\
		    total_uncomp += b - a;				\
            if (err)							\
		    printf("%s: uncompression of %s failed: %d\n", names, fn, err); \
	    int o = compare(buf2, map, size); 				\
	    if (o >= 0)							\
		    printf("%s: final comparision failed at %d of %lu\n", names, o, (unsigned long)size); \
       }								\
       printf("%-6s: %s: %lu b: ratio %.02f: comp %3.02f uncomp %2.02f c/b\n", \
	      names, basen(fn), (unsigned long)size,			\
	      (double)outlen / size,					\
	      (double)(total_comp / N) / size,				\
	      (double)(total_uncomp / N) / size);		       


#include "glue.c"

int main(int ac, char **av)
{
	int snappy_only = 0;

	if (av[1] && !strcmp(av[1], "-s")) {
		snappy_only = 1;
		av++;
	}

#ifdef SIMPLE_PMU
	pin_cpu(NULL);
	if(perfmon_available() == 0) {
		printf("no perfmon support\n");
		exit(1);
	}
#endif

	while (*++av) { 
		size_t size;
		char *map = mapfile(*av, O_RDONLY, &size);
		if (!map)
			err(*av);
		
		int i, v = 0;
		for (i = 0; i < size; i += 4096)
			v = ((volatile char *)map)[i];
		memeat(v);

#ifdef COMP
		test_lz4(map, size, *av);
#endif

		test_snappy(map, size, *av);

		if (snappy_only)
			goto unmap;

#ifdef COMP		
		test_lzo(map, size, *av);
		test_zlib(map, size, *av, 1);
		test_zlib(map, size, *av, 3);
		//test_zlib(map, size, *av, 5);
		test_lzf(map, size, *av);
		test_quicklz(map, size, *av);
		test_fastlz(map, size, *av);
#endif		

#ifdef SNAPREF
		test_snapref(map, size, *av);
#endif

unmap:
		unmap_file(map, size);
		
	}
	return 0;
}
