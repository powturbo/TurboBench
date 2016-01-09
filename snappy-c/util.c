#include <string.h>
#include <stdlib.h>
#include "util.h"

void *xmalloc(size_t size)
{
	void *ptr = malloc(size);
	if (!ptr) {
		fprintf(stderr, "out of memory\n");
		exit(1);
	}
	return ptr;
}

char *basen(char *s)
{
	char *p = strrchr(s, '/');
	if (p) 
		return p + 1;
	return s;
}

int compare(char *a, char *b, size_t size)
{
    size_t i;
	for (i = 0; i < size; i++)
		if (a[i] != b[i])
			return i;
	return -1;
}
