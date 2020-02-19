/*
 *  Copyright (C) 2017, Konstantinos Agiannis
 *
 *  Permission to use, copy, modify, and/or distribute this software for any
 *  purpose with or without fee is hereby granted, provided that the above
 *  copyright notice and this permission notice appear in all copies.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#if defined (__cplusplus)
extern "C" {
#endif

//encode bytes,return bytes written
//on error return -1
//size < 64Kb
int FPC_compress_block(void *output,const void *in,int size,int sym_num);
//on error return -1
int FPC_decompress_block(void * output,int out_size,const void *input,int in_size,int sym_num);

//return compressed size
//bsize < 64KB
//if bsize == 0 then adaptive
size_t FPC_compress(void * output,void * input,size_t inlen,int bsize);

//return uncompressed bytes
size_t FPC_decompress(void * output,size_t max_output,void * input,size_t inlen);

#define FPC_MAX_OUTPUT(S,B) ((S) + 256 + (B == 0 ? (((S)/(32*1024))+1)*4:((S)/(B)+1)*4))//TODO

#if defined (__cplusplus)
}
#endif
