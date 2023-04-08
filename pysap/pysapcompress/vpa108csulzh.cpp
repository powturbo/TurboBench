/*



    ========== licence begin  GPL
    Copyright (c) 2000-2005 SAP AG

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
    ========== licence end





*/
#include <stdlib.h>
#include <string.h>

#include "hpa101saptype.h"
#include "hpa106cslzc.h"
#include "hpa107cslzh.h"

#include "hpa104CsObject.h"
#include "hpa105CsObjInt.h"


/* Huffman code lookup table entry--this entry is four bytes for
   machines that have 16-bit pointers (e.g. PC's in the small
   or medium model).


   Valid extra bits are 0..13.  e == 15 is EOB (end of block), e == 16
   means that v is a literal, 16 < e < 32 means that v is a pointer to
   the next table, which codes e - 16 bits, and lastly e == 99 indicates
   an unused code.  If a code with e == 99 is looked up, this implies
   an error in the data. */

#define EOBCODE    15
#define LITCODE    16
#define INVALIDCODE 99



/* Tables for uncompress .............................................*/

extern int NEAR CsExtraLenBits[];
static int *cplext = &CsExtraLenBits[0];


extern int NEAR CsExtraDistBits[];
static int * cpdext = &CsExtraDistBits[0];

static unsigned short mask_bits[] =
{
 0x0000, 0x0001, 0x0003, 0x0007, 0x000f, 0x001f, 0x003f, 0x007f, 0x00ff,
 0x01ff, 0x03ff, 0x07ff, 0x0fff, 0x1fff, 0x3fff, 0x7fff, 0xffff
};

static unsigned border[] =
{    /* Order of the bit length code lengths */
  16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
};

static int cplens[] =
       {       /* Copy lengths for literal codes 257..285 */
        3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
        35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258, 0, 0
       };
        /* note: see note #13 above about the 258 in this list. */

static int cpdist[] =
       {       /* Copy offsets for distance codes 0..29 */
        1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
        257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145,
        8193, 12289, 16385, 24577
       };

/* Macros for inflate() bit peeking and grabbing.
   The usage is:

        NEEDBITS(j)
        x = b & mask_bits[j];
        DUMPBITS(j)

   where NEEDBITS makes sure that b has at least j bits in it, and
   DUMPBITS removes the bits from b.  The macros use the variable k
   for the number of bits in b.  Normally, b and k are register
   variables for speed, and are initialized at the begining of a
   routine that uses these macros from a global bit buffer and count.

   If we assume that EOB will be the longest code, then we will never
   ask for bits with NEEDBITS that are beyond the end of the stream.
   So, NEEDBITS should not read any more bytes than are needed to
   meet the request.  Then no bytes need to be "returned" to the buffer
   at the end of the last block.

   However, this assumption is not true for fixed blocks--the EOB code
   is 7 bits, but the other literal/length codes can be 8 or 9 bits.
   (Why PK made the EOB code, which can only occur once in a block,
   the *shortest* code in the set, I'll never know.)  However, by
   making the first table have a lookup of seven bits, the EOB code
   will be found in that first lookup, and so will not require that too
   many bits be pulled from the stream.
 */


#define DUMPBITS(n) { cshu.bb >>= (n); cshu.bk -= (n); }

#define NEEDBITS(n)                                             \
{                                                               \
  while (cshu.bk < (n))                                               \
  {                                                             \
    if (cshu.MemInoffset < cshu.MemInsize)                                \
    {                                                           \
      cshu.bytebuf = (unsigned short) cshu.MemInbuffer[(cshu.MemInoffset)++];    \
      bitcount = 8;                                             \
    }                                                           \
    else bitcount = 0;                                          \
    if (!bitcount) break;                                       \
    cshu.bb |= (cshu.bytebuf) << cshu.bk;                                          \
    cshu.bk += 8;                                                     \
  }                                                             \
}

/*
   Huffman code decoding is performed using a multi-level table lookup.
   The fastest way to decode is to simply build a lookup table whose
   size is determined by the longest code.  However, the time it takes
   to build this table can also be a factor if the data being decoded
   is not very long.  The most common codes are necessarily the
   shortest codes, so those codes dominate the decoding time, and hence
   the speed.  The idea is you can have a shorter table that decodes the
   shorter, more probable codes, and then point to subsidiary tables for
   the longer codes.  The time it costs to decode the longer codes is
   then traded against the time it takes to make longer tables.

   This results of this trade are in the variables lbits and dbits
   below. lbits is the number of bits the first level table for literal/
   length codes can decode in one step, and dbits is the same thing for
   the distance codes.  Subsequent tables are also less than or equal to
   those sizes.  These values may be adjusted either when all of the
   codes are shorter than that, in which case the longest code length in
   bits is used, or when the shortest code is *longer* than the
   requested table size, in which case the length of the shortest
   code in bits is used.

   There are two different values for the two tables, since they code a
   different number of possibilities each.  The literal/length table
   codes 286 possible values, or in a flat code, a little over eight
   bits.  The distance table codes 30 possible values, or a little less
   than five bits, flat.  The optimum values for speed end up being
   about one bit more than those, so lbits is 8+1 and dbits is 5+1.
   The optimum values may differ though from machine to machine, and
   possibly even between compilers.  Your mileage may vary.
 */

#define LBITS 9
#define DBITS 6

/* If BMAX needs to be larger than 16, then h and x[] should be ULONG */
#define BMAX 16    /* maximum bit length of any code (16 for explode) */
#define N_MAX 288  /* maximum number of codes in any set .............*/




int CsObjectInt::BuildHufTree (
             unsigned * b,  /* code lengths in bits (all assumed <= BMAX) */
             unsigned   n,  /* number of codes (assumed <= N_MAX) */
             unsigned   s,  /* number of simple-valued codes (0..s-1) */
             int      * d,  /* list of base values for non-simple codes */
             int      * e,  /* list of extra bits for non-simple codes */
             HUFTREE  **t,  /* result: starting table */
             int      * m)  /* maximum lookup bits, returns actual */

/* Given a list of code lengths and a maximum table size, make a set of
   tables to decode that set of codes.  Return zero on success, one if
   the given code set is incomplete (the tables are still built in this
   case), two if the input is invalid (all zero length codes or an
   oversubscribed set of lengths), and three if not enough memory. */
{
  unsigned a;                   /* counter for codes of length k */
  unsigned c[BMAX+1];           /* bit length count table */
  unsigned f;                   /* i repeats in table every f entries */
  int g;                        /* maximum code length */
  int h;                        /* table level */
  REGISTER unsigned i;          /* counter, current code */
  REGISTER unsigned j;          /* counter */
  REGISTER int k;               /* number of bits in current code */
  int l;                        /* bits per table (returned in m) */
  REGISTER unsigned *p;         /* pointer into c[], b[], or v[] */
  REGISTER HUFTREE *q;          /* points to current table */
  HUFTREE  r = {0};             /* table entry for struct assignment */
  HUFTREE *u[BMAX];             /* table stack */
  unsigned v[N_MAX];            /* values in order of bit length */
  REGISTER int w;               /* bits before this table == (l * h) */
  unsigned x[BMAX+1];           /* bit offsets, then code stack */
  unsigned *xp;                 /* pointer into x */
  int y;                        /* number of dummy codes added */
  unsigned z;                   /* number of entries in current table */

  /* Generate counts for each bit length .............................*/
  memset(c, 0, sizeof(c));
  memset(u, 0, sizeof(u));
  memset(v, 0, sizeof(v));
  memset(x, 0, sizeof(x));
  
  p = b;  i = n;
  do
  {
    c[*p++]++;                   /* assume all entries <= BMAX .......*/
  } while (--i);


  if (c[0] == n)                 /* bad input--all zero length codes .*/
    return CS_E_BAD_HUF_TREE;

  /* Find minimum and maximum length, bound *m by those ..............*/
  l = *m;
  for (j = 1; j <= BMAX; j++)
    if (c[j]) break;

  k = j;                        /* minimum code length ...............*/
  if ((unsigned)l < j) l = j;

  for (i = BMAX; i; i--)
    if (c[i]) break;

  g = i;                        /* maximum code length ...............*/
  if (( unsigned)l > i) l = i;
  *m = l;

  /* Adjust last length count to fill out codes, if needed ...........*/
  for (y = 1 << j; j < i; j++, y <<= 1)
    if ((y -= c[j]) < 0)
      return CS_E_BAD_HUF_TREE;  /* bad input: more codes than bits ..*/

  if ((y -= c[i]) < 0) return CS_E_BAD_HUF_TREE;

  c[i] += y;

  /* Generate starting offsets into the value table for each length ..*/
  x[1] = j = 0;
  p = c + 1;  xp = x + 2;
  while (--i)              /* note that i == g from above ............*/
  {
    *xp++ = (j += *p++);
  }

  /* Make a table of values in order of bit lengths ..................*/
  p = b;  i = 0;
  do
  {
    if ((j = *p++) != 0) v[x[j]++] = i;
  } while (++i < n);

  /* Generate the Huffman codes and for each, make the table entries */
  x[0] = i = 0;                 /* first Huffman code is zero */
  p = v;                        /* grab values in bit order */
  h = -1;                       /* no tables yet--level -1 */
  w = -l;                       /* bits decoded == (l * h) */
  u[0] = (HUFTREE *)NULL;       /* just to keep compilers happy */
  q = (HUFTREE *)NULL;          /* dito */
  z = 0;                        /* dito */

  /* go through the bit lengths (k already is bits in shortest code) */
  for (; k <= g; k++)
  {
    a = c[k];
    while (a--)
    {
      /* here i is the Huffman code of length k bits for value *p */
      /* make tables up to required level */
      while (k > w + l)
      {
        h++;
        w += l;                 /* previous table always l bits */

        /* compute minimum size table less than or equal to l bits */
        z = (z = g - w) > (unsigned)l ? l : z; /* upper limit on table size */
        if ((f = 1 << (j = k - w)) > a + 1)    /* try a k-w bit table */
        {                       /* too few codes for k-w bit table */
          f -= a + 1;           /* deduct codes from patterns left */
          xp = c + k;
          while (++j < z)       /* try smaller tables up to z bits */
          {
            if ((f <<= 1) <= *++xp)
              break;            /* enough codes to use up j bits */
            f -= *xp;           /* else deduct codes from patterns */
          }
        }
        z = 1 << j;             /* table entries for j-bit table */

        /* allocate and link in new table ............................*/
        q = AllocHufTree (z+1);

        if (q == (HUFTREE *) 0)
        {
          return CS_E_NO_STACKMEM;       /* not enough memory ........*/
        }

        *t = q + 1;
        *(t = &(q->v.t)) = (HUFTREE *)NULL;
        u[h] = ++q;                      /* table starts after link ..*/

        /* connect to last table, if there is one ................... */
        if (h)
        {
          x[h] = i;                    /* save pattern for backing up */
          r.b = (unsigned char)l;           /* bits to dump before this .. */
          r.e = (unsigned char)(16 + j);      /* bits in this table .......*/
          r.v.t = q;                     /* pointer to this table ....*/
          j = i >> (w - l);              /* (get around Turbo C bug) .*/
          u[h-1][j] = r;                 /* connect to last table ....*/
        }
      }

      /* set up table entry in r .....................................*/
      r.b = (unsigned char)(k - w);
      if (p >= v + n)
      {
        r.e = INVALIDCODE;             /* out of values--invalid code */
      }
      else if (*p < s)
      {                         /* 256 is end-of-block code */
        r.e = (unsigned char)(*p < 256 ? LITCODE : EOBCODE);
        r.v.n = (unsigned short) *p;  /* simple code is just the value*/
        p++;
      }
      else
      {
        /* Added check for out-of-bounds read of e & d arrays */
        if ((*p - s) >= (LENGTH_CODES + 2)){
          return (CS_E_BAD_HUF_TREE);
        }

        r.e = (unsigned char) e[*p - s]; /*non-simple,look up in lists*/
        r.v.n = (unsigned short) d[*p - s];
        p++;
      }

      /* fill code-like entries with r ...............................*/
      f = 1 << (k - w);
      for (j = i >> w; j < z; j += f)
        q[j] = r;

      /* backwards increment the k-bit code i ........................*/
      for (j = 1 << (k - 1); i & j; j >>= 1)
        i ^= j;
      i ^= j;

      /* backup over finished tables .................................*/
      while ((i & ((1 << w) - 1)) != x[h])
      {
        h--;                    /* don't need to update q ............*/
        w -= l;
      }
    }
  }

  /* Return true (1) if we were given an incomplete table ............*/
  return y != 0 && n != 1;
}


HUFTREE * CsObjectInt::AllocHufTree (unsigned size)
{
  HUFTREE * p;

  p = cshu.InterBuf + cshu.AllocStackSize;

  cshu.AllocStackSize += size; // * sizeof(HUFTREE);
  /* InterBuf is at least 0x1000 in size (5.9.96 ab) .................*/
  if (cshu.AllocStackSize >= DE_STACK_SIZE) return (HUFTREE *) 0;

  return p;
}


int CsObjectInt::FlushOut (unsigned w)          /* number of bytes to flush */
/*--------------------------------------------------------------------*/
/* Do the equivalent of OUTB for the bytes Slide[0..w-1]. ............*/
/*--------------------------------------------------------------------*/
{
  unsigned n;
  unsigned char *p;

  p = cshu.Slide + cshu.SlideOffset;
  if (w)
  {                                     /* try to fill up buffer .....*/
    if (cshu.MemOutoffset + (int)w <= cshu.MemOutsize)
    {
      memcpy (cshu.OutPtr, p, w);
      cshu.OutPtr       += w;
      cshu.BytesPending  = 0;
      cshu.MemOutoffset += w;
      cshu.SumOut       += w;
      cshu.SlideOffset   = 0;
    }
    else
    {
      n = (unsigned) (cshu.MemOutsize - cshu.MemOutoffset);
      memcpy (cshu.OutPtr, p, n);
      cshu.BytesPending  = (int)w - (int)n;
      cshu.MemOutoffset += n;
      cshu.SumOut       += n;
      cshu.SlideOffset  += n;

      return CS_END_OUTBUFFER;
    }
  }
  return 0;
}



int CsObjectInt::DecompCodes ( 
              int     *state,     /* state of last run ...............*/
              HUFTREE *tl,        /* literal/length decoder tables */
              HUFTREE *td,        /* distance decoder tables */
              int      bl,        /* number of bits decoded by tl[] */
              int      bd)        /* number of bits decoded by td[] */

/* inflate (decompress) the codes in a deflated (compressed) block.
   Return an error code or zero if it all goes ok. ...................*/
{
  REGISTER unsigned e;   /* table entry flag/number of extra bits */
  unsigned n, d;         /* length and index for copy */
  unsigned w;            /* current window position */
  unsigned ml, md;       /* masks for bl and bd bits */
  REGISTER int bitcount;
  int rc;

  /* make local copies of globals ....................................*/
  bitcount = 1;
  w = cshu.wp;                       /* initialize window position */

  /* precompute masks for speed ......................................*/
  ml = mask_bits[bl];
  md = mask_bits[bd];

  switch (*state)    /* depending on state in last run ...............*/
  {
    case 2:
      n = cshu.save_n;
      d = cshu.save_d;
      e = cshu.save_e;
      *state = 0;

      goto STATE_2;

    case 20:
      *state = 0;
      break;

    case 21:
      *state = 0;
      e = cshu.save_e;
      goto STATE_21;

    case 22:
      *state = 0;
      e = cshu.save_e;
      goto STATE_22;

    case 23:
      n = cshu.save_n;
      e = cshu.save_e;
      *state = 0;
      goto STATE_23;

    case 24:
      e = cshu.save_e;
      n = cshu.save_n;
      *state = 0;
      goto STATE_24;

    case 25:
      n = cshu.save_n;
      e = cshu.save_e;
      *state = 0;
      goto STATE_25;

    default: break;
  }

  *state = 0;

  for (;;)
  {
    NEEDBITS((unsigned)bl)
    if (bitcount == 0)
    {
      *state = 20;
      cshu.wp = w;

      return CS_END_INBUFFER;
    }

    if ((e = (cshu.htp = tl + ((unsigned)cshu.bb & ml))->e) > LITCODE)
    {
      do
      {
        if (e == INVALIDCODE) return CS_E_INVALIDCODE;

        DUMPBITS(cshu.htp->b)
        e -= LITCODE;

      STATE_21:
        NEEDBITS(e)
        if (bitcount == 0)
        {
          cshu.wp = w;
          cshu.save_e = e;
          *state = 21;

          return CS_END_INBUFFER;
        }
      }
      while ((e = (cshu.htp = cshu.htp->v.t + ((unsigned)cshu.bb & mask_bits[e]))->e) > LITCODE);
    }

    DUMPBITS(cshu.htp->b)

    if (e == LITCODE)           /* then it's a literal ...............*/
    {
      cshu.Slide[w++] = (unsigned char)cshu.htp->v.n;
      if (w == WSIZE)
      {
        if ((rc = FlushOut (w)) != 0)
        {
          cshu.wp = 0;

          *state = 1;
          return rc;
        }
        w = 0;
      }
    }
    else                        /* it's an EOB or a length ...........*/
    {
      /* exit if end of block ........................................*/
      if (e == EOBCODE)
      {
        break;
      }

      /* get length of block to copy .................................*/
    STATE_22:
      NEEDBITS(e)
      if (bitcount == 0)
      {
        cshu.wp = w;
        cshu.save_e = e;
        *state = 22;
        return CS_END_INBUFFER;
      }
      n = cshu.htp->v.n + ((unsigned)cshu.bb & mask_bits[e]);
      DUMPBITS(e);

      /* decode distance of block to copy ............................*/
    STATE_23:
      NEEDBITS((unsigned)bd)
      if (bitcount == 0)
      {
        cshu.wp = w;
        cshu.save_n = n;
        cshu.save_e = e;
        *state = 23;
        return CS_END_INBUFFER;
      }

      if ((e = (cshu.htp = td + ((unsigned)cshu.bb & md))->e) > LITCODE)
      {
        do
        {
          if (e == INVALIDCODE) return CS_E_INVALIDCODE;

          DUMPBITS(cshu.htp->b)
          e -= LITCODE;
        STATE_24:
          NEEDBITS(e)
          if (bitcount == 0)
          {
            cshu.wp = w;
            cshu.save_e = e;
            cshu.save_n = n;
            *state = 24;
            return CS_END_INBUFFER;
          }
        }
        while ((e = (cshu.htp = cshu.htp->v.t + ((unsigned)cshu.bb & mask_bits[e]))->e) > LITCODE);
      }

      DUMPBITS(cshu.htp->b)

    STATE_25:
      NEEDBITS(e)
      if (bitcount == 0)
      {
        cshu.wp = w;

        cshu.save_e = e;
        cshu.save_n = n;
        *state = 25;
        return CS_END_INBUFFER;
      }

      d = w - cshu.htp->v.n - ((unsigned)cshu.bb & mask_bits[e]);
      DUMPBITS(e)

      /* do the copy .................................................*/
      do
      {
        n -= (e = (e = WSIZE - ((d &= WSIZE-1) > w ? d : w)) > n ? n : e);

        if (w - d >= e)    /* (this test assumes unsigned comparison) */
        {
          memcpy (cshu.Slide + w, cshu.Slide + d, e);
          w += e;
          d += e;
        }
        else                /* do it slow to avoid memcpy() overlap ..*/
        {
          do
          {
            cshu.Slide[w++] = cshu.Slide[d++];
          } while (--e);
        }

        if (w == WSIZE)
        {
          if ((rc = FlushOut (w)) != 0)
          {
            cshu.wp = 0;

            cshu.save_n = n;
            cshu.save_d = d;
            cshu.save_e = e;
            *state = 2;
            return rc;
          }
        STATE_2:
          w = 0;
        }
      } while (n);
    }
  }

  cshu.wp = w;                       /* restore global window pointer .....*/

  return 0;
}


int CsObjectInt::DecompFixed (int *state)
/*--------------------------------------------------------------------*/
/* Decompress an fixed Huffman codes block.                           */
/*--------------------------------------------------------------------*/
{
  int i, rc;               /* temporary variable */
  unsigned l[288];         /* length list for BuildHufTree */

  if (*state == 0)
  {
    /* set up literal table, make a complete, but wrong code set .....*/
    for (i = 0; i < 144; i++) l[i] = 8;
    for (; i < 256; i++) l[i] = 9;
    for (; i < 280; i++) l[i] = 7;
    for (; i < 288; i++) l[i] = 8;

    cshu.blitlen = 7;
    rc = BuildHufTree (l, 288, 257, cplens, cplext, &(cshu.tlitlen), &(cshu.blitlen));
    if (rc)
    {
      cshu.AllocStackSize = 0;
      return rc;
    }

    /* set up distance table .........................................*/
    for (i = 0; i < 30; i++) l[i] = 5; /* make an incomplete code set */

    cshu.bdistlen = 5;
    if ((rc = BuildHufTree (l, 30, 0, cpdist, cpdext, &(cshu.tdistcode), &(cshu.bdistlen))) < 0)
    {
      cshu.AllocStackSize = 0;
      return rc;
    }
  }

  /* decompress until an end-of-block code ...........................*/
  if ((rc = DecompCodes (state, cshu.tlitlen, cshu.tdistcode, cshu.blitlen, cshu.bdistlen)) != 0)
    return rc;

  /* free the decoding tables, return ................................*/
  cshu.AllocStackSize = 0;

  return 0;
}


int CsObjectInt::DecompDynamic (int *state)
/*--------------------------------------------------------------------*/
/* Decompress an dynamic Huffman Code block.                          */
/*--------------------------------------------------------------------*/
{
  unsigned j;

  REGISTER int bitcount;
  int rc;

  bitcount = 1;

  switch (*state)
  {
    case 0:
    case 5:
      NEEDBITS(5)
      if (bitcount == 0)
      {
        *state = 5;
        return CS_END_INBUFFER;
      }
                                    /* number of literal/length codes */
      cshu.dd_nl = 257 + ((unsigned)cshu.bb & 0x1f);
      DUMPBITS(5)

    case 6:
      NEEDBITS(5)
      if (bitcount == 0)
      {
        *state = 6;
        return CS_END_INBUFFER;
      }

      cshu.dd_nd = 1 + ((unsigned)cshu.bb & 0x1f);  /* number of distance codes ....*/
      DUMPBITS(5)

    case 7:
      NEEDBITS(4)
      if (bitcount == 0)
      {
        *state = 7;
        return CS_END_INBUFFER;
      }

      cshu.dd_nb = 4 + ((unsigned)cshu.bb & 0xf);   /* number of bit length codes ..*/
      DUMPBITS(4)

      if (cshu.dd_nl > 286 || cshu.dd_nd > 30)
        return CS_E_BADLENGTH;        /* bad lengths .................*/

      *state = 0;
      break;

    case 8:
      j = cshu.dd_jj;
      *state = 0;
      goto STATE_8;

    case 9:
      *state = 0;
      goto STATE_9;

    case 10:
      *state = 0;
      goto STATE_10;

    case 11:
      *state = 0;
      goto STATE_11;

    case 12:
      *state = 0;
      goto STATE_12;

    default: break;
  }

  if (*state == 0)
  {
    cshu.dd_jj = 0;
    for (j = cshu.dd_jj; j < cshu.dd_nb; j++)  /* read in bit-length-code lengths ....*/
    {
    STATE_8:
      NEEDBITS(3)
      if (bitcount == 0)
      {
        cshu.dd_jj = j;
        *state = 8;
        return CS_END_INBUFFER;
      }

      cshu.dd_ll[border[j]] = (unsigned)cshu.bb & 7;
      DUMPBITS(3)
    }

    for (; j < 19; j++) cshu.dd_ll[border[j]] = 0;

    /* build decoding table for trees--single level, 7 bit lookup ....*/
    cshu.dd_bl = 7;
    if ((rc = BuildHufTree (cshu.dd_ll, 19, 19, NULL, NULL, &(cshu.dd_tl), &(cshu.dd_bl))) != 0)
    {
      cshu.AllocStackSize = 0;
      return rc;                   /* incomplete code set ............*/
    }

    cshu.dd_nolen = cshu.dd_nl + cshu.dd_nd;
    cshu.dd_maskbit = mask_bits[cshu.dd_bl];
    cshu.dd_ii = cshu.dd_lastlen = 0;
                    /* read in literal and distance code lengths .....*/
    while ((unsigned)cshu.dd_ii < cshu.dd_nolen)
    {
    STATE_9:
      NEEDBITS((unsigned)cshu.dd_bl)
      if (bitcount == 0)
      {
        *state = 9;
        return CS_END_INBUFFER;
      }

      j = (cshu.dd_td = cshu.dd_tl + ((unsigned)cshu.bb & cshu.dd_maskbit))->b;
      DUMPBITS(j)
      j = cshu.dd_td->v.n;
      if (j < 16)                 /* length of code in bits (0..15) ..*/
        cshu.dd_ll[cshu.dd_ii++] = cshu.dd_lastlen = j;          /* save last length in l ...........*/
      else if (j == 16)           /* repeat last length 3 to 6 times .*/
      {
      STATE_10:
        NEEDBITS(2)
        if (bitcount == 0)
        {
          *state = 10;
          return CS_END_INBUFFER;
        }
        j = 3 + ((unsigned)cshu.bb & 3);
        DUMPBITS(2)
        if ((unsigned)cshu.dd_ii + j > cshu.dd_nolen) return CS_E_INVALIDCODE;
        while (j--) cshu.dd_ll[cshu.dd_ii++] = cshu.dd_lastlen;
      }
      else if (j == 17)           /* 3 to 10 zero length codes .......*/
      {
      STATE_11:
        NEEDBITS(3)
        if (bitcount == 0)
        {
          *state = 11;
          return CS_END_INBUFFER;
        }

        j = 3 + ((unsigned)cshu.bb & 7);
        DUMPBITS(3)
        if ((unsigned)cshu.dd_ii + j > cshu.dd_nolen) return CS_E_INVALIDCODE;
        while (j--) cshu.dd_ll[cshu.dd_ii++] = 0;
        cshu.dd_lastlen = 0;
      }
      else                    /* j == 18: 11 to 138 zero length codes */
      {
      STATE_12:
        NEEDBITS(7)
        if (bitcount == 0)
        {
          *state = 12;
          return CS_END_INBUFFER;
        }

        j = 11 + ((unsigned)cshu.bb & 0x7f);
        DUMPBITS(7)
        if ((unsigned)cshu.dd_ii + j > cshu.dd_nolen) return CS_E_INVALIDCODE;
        while (j--) cshu.dd_ll[cshu.dd_ii++] = 0;
        cshu.dd_lastlen = 0;
      }
    }

    /* build the decoding tables for literal/length and distance codes*/
    cshu.dd_bl = LBITS;
    if ((rc = BuildHufTree (cshu.dd_ll, cshu.dd_nl, 257, cplens, cplext, &(cshu.dd_tl), &(cshu.dd_bl))) !=0)
    {
      cshu.AllocStackSize = 0;
      return rc;                   /* incomplete code set ............*/
    }

    cshu.dd_bd = DBITS;
    rc = BuildHufTree (cshu.dd_ll + cshu.dd_nl, cshu.dd_nd, 0, cpdist, cpdext, &(cshu.dd_td), &(cshu.dd_bd));
    if (rc)
    {
      cshu.AllocStackSize = 0;
      return rc;      /* incomplete code set or no stack .............*/
    }
  }

  /* decompress until an end-of-block code ...........................*/
  if ((rc = DecompCodes (state, cshu.dd_tl, cshu.dd_td, cshu.dd_bl, cshu.dd_bd)) != 0)
    return rc;

  /* free the decoding tables, return ................................*/
  cshu.AllocStackSize = 0;
  return 0;
}


int CsObjectInt::DecompBlock (int *state, int *e) /* state, last block flag */
/*--------------------------------------------------------------------*/
/* Decompress a block of codes                                        */
/*--------------------------------------------------------------------*/
{
  REGISTER int bitcount;       /* bitcount ...........................*/

  bitcount = 1;

  switch (*state)
  {
    case 0:
    case 3:
                /* read in last block bit ............................*/
      NEEDBITS(1)
      if (bitcount == 0)
      {
        *state = 3;
        return CS_END_INBUFFER;
      }

      *e = (int)cshu.bb & 1;
      DUMPBITS(1)

    case 4:     /* read in block type ................................*/
      NEEDBITS(2)
      if (bitcount == 0)
      {
        *state = 4;
        return CS_END_INBUFFER;
      }

      cshu.blocktype = (unsigned)cshu.bb & 3;
      DUMPBITS(2)

      *state = 0;
      break;

    default: break;
  }

  switch (cshu.blocktype)   /* inflate that block type ............................*/
  {
    case 2:  return DecompDynamic (state);
    case 1:  return DecompFixed (state);
    default: return CS_E_UNKNOWN_TYPE;
  }
}


void CsObjectInt::NoBits (void)
/*--------------------------------------------------------------------*/
/*                                                                    */
/*--------------------------------------------------------------------*/
{
  unsigned   x;         /* number of bits in bit buffer ............*/
  int bitcount = 1;       /* bitcount ................................*/

  NEEDBITS(NONSENSE_LENBITS)
  x = (unsigned) (cshu.bb & ((1 << NONSENSE_LENBITS) - 1));
  DUMPBITS(NONSENSE_LENBITS)

  if (x)
  {
    NEEDBITS(x)
    DUMPBITS(x)
  }
}


int CsObjectInt::CsDecomprLZH (SAP_BYTE * inp,                 /* ptr input .......*/
                  SAP_INT    inlen,               /* len of input ....*/
                  SAP_BYTE * outp,                /* ptr output ......*/
                  SAP_INT    outlen,              /* len output ......*/
                  SAP_INT    option,              /* decompr. option  */
                  SAP_INT *  bytes_read,          /* bytes read ......*/
                  SAP_INT *  bytes_decompressed)  /* bytes decompr.   */
/*--------------------------------------------------------------------*/
/* Lempel-Ziv-Huffman                                                 */
/*--------------------------------------------------------------------*/
{
  int rc;

  cshu.MemOutbuffer = outp;
  cshu.MemOutoffset = 0;
  cshu.MemOutsize   = (unsigned) outlen;

  cshu.OutPtr       = outp;

  cshu.MemInbuffer  = inp;
  cshu.MemInoffset  = 0;
  cshu.MemInsize    = (unsigned) inlen;

  if (inlen == 0 && outlen == 0) return CS_E_BOTH_ZERO;

  if (option & CS_INIT_DECOMPRESS)
  {
    cshu.BytesPending   = 0;           /* bytes to flush in next run ......*/
    cshu.SlideOffset    = 0;           /* offset in window ................*/
    cshu.AllocStackSize = 0;           /* stack counter for trees .........*/
    cshu.staterun       = 0;           /* state of uncompress .............*/
    cshu.lastblockflag  = 0;           /* last block flag (1 = last block) */

    if (inlen < CS_HEAD_SIZE) return CS_E_IN_BUFFER_LEN;

    cshu.OrgLen = CsGetLen (inp);
    if (cshu.OrgLen < 0)
      return CS_E_FILENOTCOMPRESSED;   /* Input not compressed .......*/

    cshu.SumOut = 0;
    cshu.MemInoffset = CS_HEAD_SIZE;

    /* initialize window, bit buffer .................................*/
    cshu.wp = 0;
    cshu.bk = 0;
    cshu.bb = 0;
	cshu.save_e = cshu.save_n = cshu.save_d = 0;
    cshu.NonSenseflag = 0;
    if (inlen == CS_HEAD_SIZE) return CS_END_INBUFFER;
  }

  if (cshu.NonSenseflag == 0)
  {
    NoBits ();
    cshu.NonSenseflag = 1;
  }

  if (cshu.staterun == 1 || cshu.staterun == 2)   /* end of outbuffer in last run ....*/
  {
    rc = FlushOut (cshu.BytesPending);
    if (rc || ((SAP_INT)cshu.SumOut >= cshu.OrgLen))
    {
      *bytes_read         = cshu.MemInoffset;
      *bytes_decompressed = cshu.MemOutoffset;

      if (rc) return rc;
      *bytes_read = inlen;
      return CS_END_OF_STREAM;
    }
  }

  do                           /* decompress until the last block ....*/
  {
    rc = DecompBlock (&(cshu.staterun), &(cshu.lastblockflag));
    if (rc) break;
  } while (!(cshu.lastblockflag));

  if ((rc == 0) && (cshu.staterun == 0) && (cshu.lastblockflag))
  {
    rc = FlushOut (cshu.wp);        /* flush out Slide ....................*/
    if (rc) cshu.staterun = 2;
  }
                               /* set output params ..................*/
  *bytes_read         = cshu.MemInoffset;
  *bytes_decompressed = cshu.MemOutoffset;

  if (rc) return rc;
  *bytes_read = inlen;
  return CS_END_OF_STREAM;     /* all done ...........................*/
}


