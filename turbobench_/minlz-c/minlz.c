#include <stddef.h>
#include "../conf.h"
#include "minlzc.h"
#undef MINLZ_H
#include "minlzd.h"

size_t minlz_compress(unsigned char *in, size_t inlen, unsigned char *out, size_t outsize, int lev) {
  unsigned char *ip = in, *in_ = in+inlen, *op = out, *out_ = out+inlen; 
  unsigned iplen;
  size_t osize = outsize;
  for(*op++ = 0; ip < in_; ip += iplen) {
    iplen = in_ - ip > MINLZ_MAX_BLOCK_SIZE? MINLZ_MAX_BLOCK_SIZE :  in_ - ip;
    int oplen = minlz_encode_minlz_block(op+4, osize, ip, iplen, lev); 
    if(oplen <= 0 || oplen >= MINLZ_MAX_BLOCK_SIZE) { oplen = MINLZ_MAX_BLOCK_SIZE; memcpy(op+4, ip, MINLZ_MAX_BLOCK_SIZE); }; 
    ctou32(op) = oplen; op += 4 + oplen;   osize -=  4 + oplen;   if(op >= out_) goto e;
  }
  if(op >= out_) { e: *out = 0xff; memcpy(out+1,in,inlen); op = out_+1; }
  return op - out;
}

size_t minlz_decompress(unsigned char *in, size_t inlen, unsigned char *out, size_t outlen) {
  unsigned char *op = out, *out_ = out+outlen, *ip = in;
  unsigned oplen;
  if(*ip++ == 0xff) { memcpy(out,ip,outlen); return inlen+1; }
  for(; op < out_; op += oplen) {
    oplen = out_ - op > MINLZ_MAX_BLOCK_SIZE? MINLZ_MAX_BLOCK_SIZE :  out_ - op;
    int iplen = ctou32(ip);
    if(iplen == MINLZ_MAX_BLOCK_SIZE) { memcpy(op, ip+4, MINLZ_MAX_BLOCK_SIZE); } 
    else { int rc = minlz_decode_block(op, oplen, ip+4, iplen);
      if(rc < 0) return 0;
    } 
    ip += 4 + iplen;
  }
  return ip - in;
}

