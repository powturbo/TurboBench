#include <stdio.h>
#define QLZ_COMPRESSION_LEVEL 1
#define QLZ_STREAMING_BUFFER 0
#include "src/quicklz.c"
//#include "../quicklz/src/quicklz.c"

size_t qlz_compress1(const void *source, char *destination, size_t size, char *scratch)
{
  return (size_t)qlz_compress((const unsigned char *)source, (unsigned char *)destination, (unsigned int)size, (unsigned char*)scratch);
}

size_t qlz_decompress1(const char *source, void *destination, char *scratch)
{
  return qlz_decompress(source, destination, scratch);
}

int qlz_get_setting1(int setting) { return qlz_get_setting(setting); }

