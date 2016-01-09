/*
Ascynchroneous non-buffered file I/O
Copyright Lasse Mikkel Reinhold 2006-2008
GPL-1 and GPL-2 licensed.
*/
#ifndef AIO_HEADER
#define AIO_HEADER

#define AIO_MAX_SECTOR_SIZE (64*1024)

#include <string.h>

size_t aread(void *dst, size_t size);
size_t awrite(const void *src, size_t size);
bool aopen_read(const char *file);
bool aopen_write(const char *file);
void aclose_read(void);
bool aclose_write(void);
unsigned long long awritten(void);
bool aio_init(size_t chunk_size, bool buffered_io);
size_t try_awrite(const void *Str, size_t Count);
size_t try_aread(void *DstBuf, size_t Count);
void fwrite64(unsigned long long i);
void fwrite32(unsigned int i);
unsigned int fread32(void);
unsigned long long fread64(void);
void adelete_write(void);

#endif
