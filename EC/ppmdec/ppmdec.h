/****************************************************************************
 *  This file is part of the ao0ac project a derivative of PPMd project     *
 *  PPMd written and distributed to public domain by Dmitry Shkarin 1997,   *
 *  1999-2001, 2006                                                         *
 *  Converted to an order 0 adaptive arithmetic code by Kennon Conrad, 2015 *
 ****************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif
unsigned ppmdenc(unsigned char *DecodedFile, unsigned inlen, unsigned char* EncodedFile);
unsigned ppmddec(unsigned char *EncodedFile, unsigned inlen, unsigned char *DecodedFile, unsigned outlen);
#ifdef __cplusplus
}
#endif