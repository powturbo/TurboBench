#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include "map.h"

#define roundup(x,y) (((x) + (y) - 1) & ~((y) - 1))

char *mapfile_flag(char *file, int oflags, size_t *size, int flag)
{
	int fd = open(file, oflags, 0644);
	if (fd < 0) 
		return NULL;
	
	struct stat st;
	if (fstat(fd, &st) >= 0 && (*size = st.st_size) > 0) {
		size_t ps = sysconf(_SC_PAGE_SIZE);
		*size =  roundup(st.st_size, ps);
		int prot = PROT_READ;
		if ((oflags & O_WRONLY) || (flag & MAP_PRIVATE))
			prot |= PROT_WRITE;
		char *map = mmap(NULL, *size, prot,
				 flag,
				 fd, 0);
		close(fd);
		if (map == (char *)-1)
			return NULL;
		*size = st.st_size;
		return map;		
	} 
	close(fd);
	return NULL;		
}

char *mapfile(char *file, int oflags, size_t *size)
{
	return mapfile_flag(file, oflags, size, MAP_SHARED);
}

void unmap_file(char *map, size_t size)
{
	size_t ps = sysconf(_SC_PAGE_SIZE);
	munmap(map, roundup(size, ps));
}
