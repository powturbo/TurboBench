#define FREEARC_STANDALONE_TORNADO
#define  MyAlloc zMyAlloc
#define  MyFree  zMyFree
#define  MidAlloc zMidAlloc
#define  MidFree  zMidFree
#define  BigAlloc zBigAlloc
#define  BigFree  zBigFree

#include "Common.cpp"
#include "../tornado/src/Tornado.cpp"

#include "tormem.h"

int compress_all_at_once = 0;

struct bufio { unsigned char *rdp,*rde,*wrp,*wre; };

void bufioini(struct bufio *b, unsigned char *in, int inlen, unsigned char *out, int outsize) {
  b->rdp = in; b->rde = in+inlen; b->wrp = out; b->wre = out + outsize;
}

#define wrptr(b) (b)->wrp

int ReadWriteMem (const char *what, void *buf, int size, void *_bufio)
{
  bufio *bufio = (struct bufio *)_bufio;
  if (strequ(what,"read")) {
    if(bufio->rdp + size > bufio->rde) size = bufio->rde - bufio->rdp;
    memcpy(buf, bufio->rdp, size);
    bufio->rdp += size;
    return size;
  } else if (strequ(what,"write")) {
    memcpy (bufio->wrp, buf, size);
    bufio->wrp  += size;
    return size;
  }
  return FREEARC_OK;
}

unsigned torcompress(unsigned char method, unsigned char* in, unsigned char* out, unsigned size)
{
  struct bufio bufio; bufioini(&bufio, in, size, out, size);
  PackMethod m = std_Tornado_method[method];
  int rc = tor_compress (m, ReadWriteMem, &bufio, NULL, -1);
  return rc?0:wrptr(&bufio) - out;
}

unsigned tordecompress(unsigned char* in, unsigned char* out, unsigned inlen, unsigned outsize)
{
  struct bufio bufio; bufioini(&bufio, in, inlen, out, outsize);
  int rc = tor_decompress(ReadWriteMem, &bufio, NULL, -1);
  return rc?0:wrptr(&bufio) - out;
}

#define CHUNK_SIZE (1024*1024)
#define EOB_CODE 0x100
int torhenc(unsigned char *in, int inlen, unsigned char *out, int outsize) {
  struct bufio bufio; bufioini(&bufio, in, inlen, out, outsize);
  HuffmanEncoder<EOB_CODE> huff(ReadWriteMem, &bufio, CHUNK_SIZE, 4*CHUNK_SIZE, EOB_CODE+1);
  unsigned char *p,*iep = in+inlen;
  for(p = in; p < iep; ) { unsigned char *ep = p + CHUNK_SIZE; if(ep>iep) ep = iep;
    for(; p < ep; p++) huff.encode(*p); 
    huff.flush();
  } huff.finish();
  return wrptr(&bufio) - out;
}
 
int torhdec(unsigned char *in, int inlen, unsigned char *out, int outlen ) {
  struct bufio bufio; bufioini(&bufio, in, inlen, out, outlen);
  HuffmanDecoder<EOB_CODE> huff(ReadWriteMem, &bufio, CHUNK_SIZE, EOB_CODE+1);
  for(int i=0; i<outlen; i++) out[i] = huff.decode();
  return wrptr(&bufio) - out;
}
