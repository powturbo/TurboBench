#include <stdlib.h>
#include "snappy.h"
#include "map.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "util.h"

int main(int ac, char **av)
{
	int failed = 0, verbose = 0;
	struct snappy_env env;
	snappy_init_env(&env);

	if (av[1] && !strcmp(av[1], "-v")) {
		verbose++;
		av++;
	}

	while (*++av) { 
		size_t size;
		char *map = mapfile(*av, O_RDONLY, &size);
		if (!map) {
			if (size > 0) {
				perror(*av);
				failed = 1;
			}
			continue;
		}

		size_t outlen;
		int err;       
		char *out = xmalloc(snappy_max_compressed_length(size));
		char *buf2 = xmalloc(size);

		err = snappy_compress(&env, map, size, out, &outlen);		
		if (err) {
			failed = 1;
			printf("compression of %s failed: %d\n", *av, err);
			goto next;
		}
		err = snappy_uncompress(out, outlen, buf2);

		if (err) {
			failed = 1;
			printf("uncompression of %s failed: %d\n", *av, err);
			goto next;
		}
		if (memcmp(buf2, map, size)) {
			int o = compare(buf2, map, size);			
			if (o >= 0) {
				failed = 1;
				printf("final comparision of %s failed at %d of %lu\n", 
				       *av, o, (unsigned long)size);
			}
		} else {
			if (verbose)
				printf("%s OK!\n", *av);
		}

	next:
		unmap_file(map, size);
		free(out);
		free(buf2);
	}
	return failed;
}
