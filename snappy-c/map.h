#include <stddef.h>
char *mapfile(char *file, int oflags, size_t *size);
char *mapfile_flag(char *file, int oflags, size_t *size, int flag);
void unmap_file(char *map, size_t size);
