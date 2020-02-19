/**
    Copyright (C) powturbo 2013-2020
    GPL v2 License
  
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

    - homepage : https://sites.google.com/site/powturbo/
    - github   : https://github.com/powturbo
    - twitter  : https://twitter.com/powturbo
    - email    : powturbo [_AT_] gmail [_DOT_] com
**/
//	    TurboBench: plugins.h - settings 
#define E_ANS  0x1
#define E_HUF  0x2

struct plugs { 
  int   id; 
  char  *s;
  int   codec; 
  char  *name,*lev; 
  unsigned flag,blksize; 
};

  #ifdef __cplusplus
extern "C" {
  #endif
extern struct plugs plugs[];
int  coddicsize(int _dicsize);
int  codini(size_t insize, int codec, int lev, char *prm);
void codexit(int codec);
int  codstart(size_t insize, int codec, int lev, char *prm, int mode);
int  codend(  size_t insize, int codec, int lev, char *prm, int mode);

int  codcomp(  unsigned char *in, int inlen, unsigned char *out, int outsize, int codec, int lev, char *prm);
int  coddecomp(unsigned char *in, int inlen, unsigned char *out, int outlen,  int codec, int lev, char *prm);
char *codver(int codec, char *v, char *s);
void *_valloc(size_t size, unsigned a);
void _vfree(void *p, size_t size);
  #ifdef __cplusplus
}
  #endif
