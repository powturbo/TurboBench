/* zlib-turbo for TurboBench: in a file of its own, as zlib-turbo is built with Z_PREFIX to link beside zlib,
   and its zlib.h cannot be included with zlib's in plugin.cc */
#include <string.h>
#include "../zlib-turbo/zlib.h"
#include "../zlib-turbo/zturbo.h"

/* zlib format, as compress2(); levels 4-9 search on threads of their own, with the output of one thread */
unsigned zlib_turbo_compress(const unsigned char *in, unsigned inlen, unsigned char *out, unsigned outsize, int lev, int threads) {
  z_stream strm;
  unsigned outlen;
  int rc;
  memset(&strm, 0, sizeof(strm));
  if(deflateInit(&strm, lev) != Z_OK) return 0;
  if(threads > 1) deflateSetThreads(&strm, threads);   /* without threads in the build, the stream stays on one */
  strm.next_in  = (Bytef *)in;  strm.avail_in  = inlen;
  strm.next_out = out;          strm.avail_out = outsize;
  rc = deflate(&strm, Z_FINISH);
  outlen = (unsigned)strm.total_out;
  deflateEnd(&strm);
  return rc == Z_STREAM_END ? outlen : 0;
}

unsigned zlib_turbo_decompress(const unsigned char *in, unsigned inlen, unsigned char *out, unsigned outlen) {
  uLongf n = outlen;
  return uncompress(out, &n, in, inlen) == Z_OK ? (unsigned)n : 0;
}

const char *zlib_turbo_version(void) { return zlibTurboVersion(); }
