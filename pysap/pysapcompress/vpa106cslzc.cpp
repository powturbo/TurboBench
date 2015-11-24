/* static char sccsid[] = "@(#)cslzc    20.10     SAP     97/11/11"; 


    ========== licence begin  GPL
    Copyright (c) 1992-2005 SAP AG

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

/*
 *     SAP AG Walldorf
 *     Systeme, Anwendungen und Produkte in der Datenverarbeitung
 *
 */

/*
 * Compress - Decompress Module (LZC Algorithm)
 */

#include <string.h>          /* C Header Files ........................*/

#include "hpa101saptype.h"        /* Common SAP Header Files ...............*/
#include "hpa106cslzc.h"          /* Internal Definitions for LZC algorithm */
#include "hpa107cslzh.h"

#include "hpa104CsObject.h"
#include "hpa105CsObjInt.h"



/* 2 ** 8 - 2 ** i (i=0..8) ..........................................*/
static BYTE_TYP lmask[9] =
       {0xff, 0xfe, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80, 0x00};

/* 2 ** i - 1 (i=0..8) ...............................................*/
static BYTE_TYP rmask[9] =
       {0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff};

/* needed for HP 64 compiling. Compare hpa106cslzc.h */
CODE_INT DE_STACK_OFFSET = 1<<(CS_BITS+1);
// extern int csbufsize2;
// static BYTE_TYP *stack_end;  

// #define CS_STACK_CHECK 1

/* Macros for STACK CHECK in CsDecomp */
#ifdef CS_STACK_CHECK

/*
#define STACK_OVERFLOW_CHECK(p) ((p) >= ((BYTE_TYP *)&CsDeInterBuf[csbufsize2 / 4 - 1]))
*/
#define STACK_OVERFLOW_CHECK(p) ((p) >= (csc.stack_end))

#define STACK_UNDERFLOW_CHECK(p) ((p) < (DE_STACK))

#define OVERFLOW_CHECK 	                 \
      if (STACK_OVERFLOW_CHECK(stackp))  \
	  {                                  \
        return CS_E_STACK_OVERFLOW;      \
	  }

#define UNDERFLOW_CHECK                  \
	  if (STACK_UNDERFLOW_CHECK(stackp))  \
	  {                                  \
        return CS_E_STACK_UNDERFLOW;     \
	  }

#else

#define OVERFLOW_CHECK
#define UNDERFLOW_CHECK

#endif


#define PUTCODE(cod)  /* PUTCODE Macro -----------------------------*/ \
/*------------------------------------------------------------------*/ \
/* Output one code  (A maximum of CS_BITS is written)               */ \
/*------------------------------------------------------------------*/ \
{                                                                      \
  register BYTE_TYP * bp = csc.buf1;                                   \
  register int code = cod;                                             \
  register unsigned int r_off, bits = csc.n_bits;                      \
                                                                       \
  if (csc.put_n_bytes)                /* put out the rest ........*/   \
  {                                                                    \
    if (csc.end_outbuf - csc.outptr >= csc.put_n_bytes)                \
    {                                                                  \
      BYTES_OUT (csc.outptr, csc.buf1 + csc.csc_offset, csc.put_n_bytes) \
      csc.put_n_bytes = 0;                                             \
      csc.csc_offset  = 0;                                             \
      csc.cs_offset   = 0;                                             \
    }                                                                  \
    else return CS_E_OUT_BUFFER_LEN;                                   \
  }                                                                    \
                                                                       \
  r_off = csc.cs_offset;                                               \
                                                                       \
  if (code >= 0)                      /* valid code...............*/   \
  {                                                                    \
     /* Get to the first byte ....................................*/   \
    bp += (r_off >> 3);                                                \
    r_off &= 7;                                                        \
                                                                       \
   /*                                                                  \
    * Since code is always >= 8 bits, only need to mask the first      \
    * hunk on the left.                                                \
    */                                                                 \
                                                                       \
    *bp = (*bp & rmask[r_off]) | ((code << r_off) & lmask[r_off]);     \
    bp++;                                                              \
    r_off = 8 - r_off;                                                 \
    bits -= r_off;                                                     \
    code >>= r_off;                                                    \
                                                                       \
    /* Get any 8 bit parts in the middle (<=1 for up to 16 bits)..*/   \
    if (bits >= 8)                                                     \
    {                                                                  \
      *bp++ = code;                                                    \
      code >>= 8;                                                      \
      bits -= 8;                                                       \
    }                                                                  \
                                                                       \
    /* Last bits .................................................*/   \
    if (bits)                                                          \
      *bp = code;                                                      \
                                                                       \
    csc.cs_offset += csc.n_bits;                                       \
    if (csc.cs_offset == (csc.n_bits << 3))                            \
    {                                                                  \
      if (csc.end_outbuf - csc.outptr >= csc.n_bits)                   \
      {                                                                \
        BYTES_OUT (csc.outptr, csc.buf1, csc.n_bits)                   \
      }                                                                \
      else                                                             \
      {                                                                \
        csc.put_n_bytes = (SAP_INT) (csc.end_outbuf - csc.outptr);                 \
        BYTES_OUT (csc.outptr, csc.buf1, csc.put_n_bytes)              \
        csc.csc_offset = csc.put_n_bytes;                              \
        csc.put_n_bytes = csc.n_bits - csc.put_n_bytes;                \
      }                                                                \
      csc.cs_offset = 0;                                               \
    }                                                                  \
                                                                       \
   /*                                                                  \
    * If the next entry is going to be too big for the code size,      \
    * then increase it, if possible.                                   \
    */                                                                 \
                                                                       \
    if (csc.free_ent > csc.maxcode || (csc.clear_flg > 0))             \
    {                                                                  \
     /*                                                                \
      * Write the whole buffer, because the input side won't           \
      * discover the size increase until after it has read it.         \
      */                                                               \
                                                                       \
      if (csc.cs_offset > 0)                                           \
      {                                                                \
        if (csc.end_outbuf - csc.outptr >= csc.n_bits)                 \
        {                                                              \
          BYTES_OUT (csc.outptr, csc.buf1, csc.n_bits)                 \
        }                                                              \
        else                                                           \
        {                                                              \
          csc.put_n_bytes = (SAP_INT) (csc.end_outbuf - csc.outptr);               \
          BYTES_OUT (csc.outptr, csc.buf1, csc.put_n_bytes)            \
          csc.csc_offset = csc.put_n_bytes;                            \
          csc.put_n_bytes = csc.n_bits - csc.put_n_bytes;              \
        }                                                              \
        csc.cs_offset = 0;                                             \
      }                                                                \
                                                                       \
      if (csc.clear_flg)                                               \
      {                                                                \
        csc.maxcode = MAXCODE (csc.n_bits = INIT_CS_BITS);             \
        csc.clear_flg = 0;                                             \
      }                                                                \
      else                                                             \
      {                                                                \
        (csc.n_bits)++;                                                \
        if (csc.n_bits == csc.maxbits)                                 \
          csc.maxcode = csc.maxmaxcode;                                \
        else                                                           \
          csc.maxcode = MAXCODE(csc.n_bits);                           \
      }                                                                \
    }                                                                  \
  }                                                                    \
  else       /* At EOF, write the rest of the buffer .............*/   \
  {                                                                    \
    if (csc.cs_offset > 0)                                             \
    {                                                                  \
      csc.n_bits = (csc.cs_offset + 7) / 8;                            \
      if (csc.end_outbuf - csc.outptr >= csc.n_bits)                   \
      {                                                                \
        BYTES_OUT (csc.outptr, csc.buf1, csc.n_bits)                   \
      }                                                                \
      else                                                             \
      {                                                                \
        csc.put_n_bytes = (SAP_INT) (csc.end_outbuf - csc.outptr);                 \
        BYTES_OUT (csc.outptr, csc.buf1, csc.put_n_bytes)              \
        csc.csc_offset = csc.put_n_bytes;                              \
        csc.put_n_bytes = csc.n_bits - csc.put_n_bytes;                \
      }                                                                \
      csc.cs_offset = 0;                                               \
    }                                                                  \
  }                                                                    \
} /* ------------ End of PUTCODE Macro -----------------------------*/


/*--------------------------------------------------------------------*/
/* Static functions (prototypes)                                      */
/*--------------------------------------------------------------------*/

SAP_INT CsObjectInt::CsGetStorageSize (void)
/*--------------------------------------------------------------------*/
/* Get the size for internal storage                                  */
/*--------------------------------------------------------------------*/
{
  return ((SAP_INT) sizeof (csc.htab) + (SAP_INT) sizeof (csc.codetab));
}


int CsObjectInt::InitComp (BYTE_TYP * outbuf,
                     SAP_INT    outlen,
                     SAP_INT    sumlen)
/*--------------------------------------------------------------------*/
/* Setup header info                                                  */
/* Clear hash table                                                   */
/* Initialize static variables for compression                        */
/*--------------------------------------------------------------------*/
{
  if (outlen < CS_HEAD_SIZE)       /* too small ......................*/
    return CS_E_OUT_BUFFER_LEN;

  if (sumlen <= 0L)
    return CS_E_INVALID_SUMLEN;

  csc.clear_flg      = 0;              /* init compression states ........*/
  csc.ratio          = 0;

  csc.block_compress = BLOCK_MASK;
  csc.maxbits        = CS_BITS;

  csc.checkpoint     = CHECK_GAP;
  csc.maxcode        = MAXCODE (csc.n_bits = INIT_CS_BITS);
  csc.maxmaxcode     = (CODE_INT)1 << CS_BITS;
  csc.free_ent       = ((csc.block_compress) ? FIRST : 256);
  csc.hsize          = HSIZE;
  CL_HASH (csc.hsize);         /* clear hash table .......................*/
                           /* fill in header informations ............*/
  CsSetHead (outbuf, sumlen,
             (BYTE_TYP) ((CS_VERSION << 4) | CS_ALGORITHM),
             (BYTE_TYP) (csc.maxbits | csc.block_compress));

  return 0;
}


int CsObjectInt::CsComprLZC (SAP_INT    sumlen,
                SAP_BYTE * inbuf,
                SAP_INT    inlen,
                SAP_BYTE * outbuf,
                SAP_INT    outlen,
                SAP_INT    option,
                SAP_INT *  bytes_read,
                SAP_INT *  bytes_written)
/*--------------------------------------------------------------------*/
/* LZC method to compress a file                                      */
/*                                                                    */
/* Adaptive Dictionary Compression                                    */
/*   Lempel-Zip-Thomas                                                */
/*                                                                    */
/*                                                                    */
/* Input:                                                             */
/* -----                                                              */
/*        sumlen            length of data stream to compress         */
/*        inbuf             Pointer to input memory                   */
/*        inlen             Length of input memory                    */
/*        outbuf            Pointer to output area                    */
/*        outlen            Length of output area                     */
/*        option            Compress option:                          */
/*                             CS_INIT_COMPRESS      initial          */
/*                             CS_NORMAL_COMPRESS                     */
/* Output:                                                            */
/* ------                                                             */
/*        bytes_read        Bytes read from input buffer              */
/*        bytes_compressed  Bytes compressed to output buffer         */
/*                                                                    */
/* Internal Functions:                                                */
/* ------------------                                                 */
/*       InitComp              Initialize Compression                 */
/*       PUTCODE               Write Code to output buffer            */
/*                                                                    */
/* Return Code:                                                       */
/* -----------                                                        */
/*        CS_END_OF_STREAM     End of input reached                   */
/*        CS_END_INBUFFER      End of input reached                   */
/*        CS_END_OUTBUFFER     End of output reached                  */
/*                                                                    */
/*        CS_E_IN_BUFFER_LEN   Input buffer length to short           */
/*        CS_E_OUT_BUFFER_LEN  Output Buffer length to short          */
/*        CS_E_INVALID_SUMLEN  sumlen <= 0                            */
/*        CS_E_INVALID_ADDR    Invalid addr for input or output buffer*/
/*        CS_E_FATAL           internal (should never happen)         */
/*                                                                    */
/*--------------------------------------------------------------------*/
{
  register SAP_INT fcode;
  register CODE_INT i = 0;
  register BYTE_TYP *inptr = inbuf;
  register int c;
  register CODE_INT disp;

  int rc;

  *bytes_read    = 0;
  *bytes_written = 0;

  /* Check input parameters ..........................................*/

  if (inlen < 0) return CS_E_IN_BUFFER_LEN;     /* return ............*/

  if (outlen < 0) return CS_E_OUT_BUFFER_LEN;   /* return ............*/

  if (inbuf == (BYTE_TYP *) 0 || outbuf == (BYTE_TYP *) 0)
    return CS_E_INVALID_ADDR;                   /* return ............*/

  if (inbuf == outbuf) return CS_E_IN_EQU_OUT;  /* return ............*/

  csc.outptr = outbuf;

  if (option & CS_INIT_COMPRESS)      /* only initial ................*/
  {
    if (sumlen <= 0) return CS_E_INVALID_SUMLEN;  /* return ..........*/

    rc = InitComp (outbuf, outlen, sumlen);
    if (rc) return rc;            /* return with Error ...............*/

    csc.cs_offset    = 0;
    csc.csc_offset   = 0;
    csc.in_count_sum = 0;
    csc.put_n_bytes  = 0;
    csc.bytes_out    = 0;
    csc.sflush       = 0;
    csc.org_len   = sumlen;           /* save sum length .................*/
    csc.outptr    = outbuf + CS_HEAD_SIZE; /* 4 byte length, 4 byte header*/

    csc.hshift = 0;
    for (fcode = (SAP_INT) csc.hsize;  fcode < 65536; fcode += fcode)
      (csc.hshift)++;
    csc.hshift = 8 - csc.hshift;          /* set hash code range bound .......*/

    csc.ent = (CODE_INT) -1;
    if (inlen == 0)               /* only header .....................*/
    {
      *bytes_written = CS_HEAD_SIZE;
      csc.bytes_out     += *bytes_written;
      return CS_END_INBUFFER;
    }
  }

  if (csc.ent == (CODE_INT) -1)       /* get first byte ..................*/
    csc.ent = *inptr++;

  csc.end_inbuf  = inbuf  + inlen;    /* end of input buffer .............*/
  csc.end_outbuf = outbuf + outlen;   /* end of output buffer ............*/

  if (csc.sflush == 1)   /* end of outbuf in last run, must output CLEAR .*/
  {
    if ((rc = PutCode ((CODE_INT) CLEAR)) < 0)
      goto ende;

    if (csc.put_n_bytes)              /* no more space in outbuf .........*/
    {
      rc = CS_END_OUTBUFFER;
      goto ende;
    }
    csc.sflush = 0;
  }

  while (inptr < csc.end_inbuf)          /* until end of input ...........*/
  {
    c = *inptr++;                    /* next char ....................*/
                                     /* hash function ................*/
    fcode = (SAP_INT) (((SAP_INT) c << csc.maxbits) + csc.ent);
    i = (((CODE_INT)c << csc.hshift) ^ csc.ent);    /* xor hashing ...........*/

    if (i < 0) return CS_E_FATAL;    /* should never happen ..........*/

    if (HTABOF (i) == fcode)         /* found in htab ................*/
    {
      csc.ent = CODETABOF (i);
      continue;
    }
    else
    if ((SAP_INT)HTABOF (i) < 0)       /* empty slot ....................*/
      goto nomatch;

    disp = csc.hsize - i;               /* secondary hash (G. Knott) .....*/
    if (i == 0) disp = 1;

probe:
    if ((i -= disp) < 0)
      i += csc.hsize;

    if (HTABOF (i) == fcode)        /* found in htab .................*/
    {
      csc.ent = CODETABOF (i);
      continue;                     /* get next char .................*/
    }

    if ( (SAP_INT)HTABOF (i) > 0 )
      goto probe;                   /* test again ....................*/

nomatch:                            /* new entry .....................*/
    PUTCODE ((CODE_INT) csc.ent)

    csc.ent = c;
    if (csc.free_ent < csc.maxmaxcode)      /* enough space in table ? .......*/
    {
      CODETABOF (i) = (csc.free_ent)++;   /* code -> hashtable .............*/
      HTABOF (i) = fcode;
    }
    else                            /* test clear code table .........*/
    if (((SAP_INT)(inptr - inbuf) + csc.in_count_sum >=
                                     csc.checkpoint) && csc.block_compress)
    {
      rc = ClearBlock (csc.in_count_sum + (SAP_INT)(inptr - inbuf),
                       csc.bytes_out + (SAP_INT) (csc.outptr - outbuf));
      if (rc)
      {
        if (csc.put_n_bytes)            /* no more space in outbuf .......*/
        {
          csc.sflush = 1;
          rc = CS_END_OUTBUFFER;
          goto ende;
        }
        if ((rc = PutCode ((CODE_INT) CLEAR)) < 0)
          goto ende;
      }
    }

    if (csc.put_n_bytes)                /* not enough space in outbuffer .*/
    {
      rc = CS_END_OUTBUFFER;
      goto ende;
    }
  }  /* end while ....................................................*/

  rc = CS_END_INBUFFER;
                                                 /* end of stream ....*/
  if (csc.in_count_sum + (SAP_INT)(inptr - inbuf) >= csc.org_len)
  {
    if (csc.sflush < 3)
    {
      if ((rc = PutCode ((CODE_INT) csc.ent)) < 0)
        goto ende;
      if (csc.put_n_bytes)            /* not enough space in outbuffer ...*/
      {
        csc.sflush = 3;
        rc = CS_END_OUTBUFFER;
        goto ende;
      }
    }

    if (csc.sflush != 4)
    {
      if ((rc = PutCode ((CODE_INT) -1)) < 0)
        goto ende;

      if (csc.put_n_bytes)            /* not enough space in outbuffer ...*/
      {
        csc.sflush = 4;
        rc = CS_END_OUTBUFFER;
        goto ende;
      }
    }
    else
      if ((rc = PutCode ((CODE_INT) -1)) < 0)
        goto ende;

    rc = CS_END_OF_STREAM;       /* end of data reached ..............*/
  }

ende:                            /* set output parameters and save    */
  *bytes_written = (SAP_INT) (csc.outptr - outbuf);
  *bytes_read    = (SAP_INT) (inptr  - inbuf);
  csc.in_count_sum  += *bytes_read;     /* the sum of input length .......*/
  csc.bytes_out     += *bytes_written;  /* the sum of output length ......*/

  return rc;
}


int CsObjectInt::ClearBlock (SAP_INT in_count, SAP_INT bytes_out)
/*--------------------------------------------------------------------*/
/* table clear for block compress if compression ratio is "bad"       */
/*       returns   1: clear block                                     */
/*       returns   0: no clear                                        */
/*--------------------------------------------------------------------*/
{
  register SAP_INT rat;

  csc.checkpoint = in_count + CHECK_GAP;

  if (in_count > 0x007fffffL)   /* shift will overflow ...............*/
  {
    rat = bytes_out >> 8;
    if (rat == 0)               /* Don't divide by zero ..............*/
      rat = 0x7fffffffL;
    else
      rat = in_count / rat;
  }
  else
    rat = (in_count << 8) / bytes_out;      /* 8 fractional bits .....*/

  if (rat > csc.ratio)
    csc.ratio = rat;
  else                          /* clear hash table ..................*/
  {
    csc.ratio = 0;
    CL_HASH (csc.hsize);
    csc.free_ent = FIRST;
    csc.clear_flg = 1;
    return 1;                  /* indicates: flush a CLEAR ...........*/
  }
  return 0;                    /* no CLEAR ...........................*/
}


int CsObjectInt::CsDecomprLZC (SAP_BYTE * inbuf,
                  SAP_INT    inlen,
                  SAP_BYTE * outbuf,
                  SAP_INT    outlen,
                  SAP_INT    option,
                  SAP_INT *  bytes_read,
                  SAP_INT *  bytes_written)
/*--------------------------------------------------------------------*/
/* LZC decompress                                                     */
/*                                                                    */
/* Adaptive Dictionary Compression                                    */
/*   Lempel-Zip-Welch-Thomas                                          */
/*                                                                    */
/* Input:                                                             */
/* -----                                                              */
/*        inbuf             Pointer to input memory                   */
/*        inlen             Length of input memory                    */
/*        outbuf            Pointer to output area                    */
/*        outlen            Length of output area                     */
/*        option            Compress option:                          */
/*                             CS_INIT_COMPRESS      initial          */
/*                             CS_NORMAL_COMPRESS                     */
/*                                                                    */
/* Output:                                                            */
/* ------                                                             */
/*        bytes_read        Bytes read from input buffer              */
/*        bytes_written     Bytes decompressed to output buffer       */
/*                                                                    */
/* Internal Functions:                                                */
/* ------------------                                                 */
/*       GetCode               Get a code from input buffer           */
/*                                                                    */
/* Return Code:                                                       */
/* -----------                                                        */
/*        CS_END_OF_STREAM     End of input stream reached            */
/*        CS_END_INBUFFER      End of input buffer reached            */
/*        CS_END_OUTBUFFER     End of output buffer reached           */
/*                                                                    */
/*        CS_E_OUT_BUFFER_LEN  Output buffer length to short          */
/*        CS_E_IN_BUFFER_LEN   Input buffer length to short           */
/*        CS_E_MAXBITS_TOO_BIG No internal memory to decompress       */
/*        CS_E_INVALID_LEN     inlen < 0 or outlen < CS_BITS          */
/*        CS_E_FILENOTCOMPRESSED Input is not compressed              */
/*        CS_E_IN_EQU_OUT      Same addr for input and output buffer  */
/*        CS_E_INVALID_ADDR    Invalid addr for input or output buffer*/
/*        CS_E_FATAL           Internal (should never happen)         */
/*                                                                    */
/*--------------------------------------------------------------------*/
{
  register BYTE_TYP *stackp;
  register CODE_INT code, oldcode, incode, finchar;
  register SAP_INT rest_lenr;
/*
  static BYTE_TYP *sstackp = (BYTE_TYP *) 0;

  static long dorg_len;
  static CODE_INT scode, soldcode, sincode, sfinchar;
  static int restart;
*/
#ifdef SAPonWINDOWS
  Suffixtab = (BYTE_TYP *) CsDeInterBuf;

  Prefixtab = (CODE_ENTRY *) CsDeWindowBuf;
#endif

  *bytes_read    = 0;                    /* init output parameters ...*/
  *bytes_written = 0;

  /* Check input parameters ..........................................*/
  if (inlen < 0)                         /* invalid len of inbuf .....*/
    return CS_E_IN_BUFFER_LEN;
                                         /* invalid addr .............*/
  if (inbuf == (BYTE_TYP *) 0 || outbuf == (BYTE_TYP *) 0)
    return CS_E_INVALID_ADDR;

  if (inbuf == outbuf)                   /* inbuf == outbuf: invalid !*/
    return CS_E_IN_EQU_OUT;

  csc.end_inbuf  = inbuf + inlen;            /* set start & end ptrs .....*/
  csc.end_outbuf = outbuf + outlen;
  csc.outptr     = outbuf;
  rest_lenr  = csc.rest_len;                 /* push to register .........*/

  if (option & CS_INIT_DECOMPRESS)       /* only initial .............*/
  {
	csc.sstackp = (BYTE_TYP *) 0;
    csc.restart    =  0;
    csc.csc_offset =  0;
    csc.get_size   =  0;
    csc.get_r_bits = -1;
//    csc.stack_end = &(csc.Suffixtab[csbufsize2 - 1]);

    if (inlen < CS_HEAD_SIZE)         /* input buffer too small ......*/
      return CS_E_IN_BUFFER_LEN;

    csc.dorg_len = CsGetLen (inbuf);       /* get sum length               */

    if (csc.dorg_len < 0)                  /* and check if file is compr. .*/
      return (CS_E_FILENOTCOMPRESSED);

    csc.maxbits        = inbuf[7];        /* get max. bits ...............*/
    csc.block_compress = csc.maxbits & BLOCK_MASK;
    csc.maxbits       &= BIT_MASK;
    csc.maxmaxcode     = (CODE_INT) 1 << csc.maxbits;
    csc.maxcode        = MAXCODE(csc.n_bits = INIT_CS_BITS);

    if (csc.maxbits > CS_BITS + 1)     /* not enough memory to decompress */
      return CS_E_MAXBITS_TOO_BIG;

    /* get version and algorithm .....................................*/
    /* not supported at the moment ...................................*/

    for (code = 255; code >= 0; code--)    /* init. code table .......*/
    {
      TAB_PREFIXOF(code) = 0;
      TAB_SUFFIXOF(code) = (BYTE_TYP) code;
    }

    csc.free_ent = ((csc.block_compress) ? FIRST : 256);   /* first entry ....*/

    csc.in_ptr    = inbuf + CS_HEAD_SIZE; /* skip header .................*/
    csc.rest_len  = csc.dorg_len;              /* save sum length .............*/
    rest_lenr = csc.rest_len;
    stackp    = DE_STACK;             /* init. stack ptr .............*/

    if (outlen == 0)                  /* End of output buffer ........*/
    {
      code = CS_END_OUTBUFFER;
      goto ende;
    }

    if (csc.in_ptr >= csc.end_inbuf)          /* End of input buffer .........*/
    {
      code = CS_END_INBUFFER;
      goto ende;
    }
  }
  else                                /* not initial .................*/
  {
    csc.in_ptr  = inbuf;
    stackp  = csc.sstackp;                /* restore states ..............*/
    finchar = csc.sfinchar;
    oldcode = csc.soldcode;

    if (outlen <= 0)                  /* min. size for outbuffer .....*/
      return CS_E_OUT_BUFFER_LEN;

    if (rest_lenr <= 0)               /* end of input ................*/
    {
      code = CS_END_OF_STREAM;
      goto ende;
    }

    if (csc.restart)  /* output buffer to small in last run ..............*/
    {
      /* restore machine state .......................................*/
      code    = csc.scode;
      incode  = csc.sincode;
      csc.restart = 0;
      goto contin;
    }
  }

  if (csc.get_r_bits == -1)               /* init. decoding ..............*/
  {
    finchar = oldcode = (CODE_INT) GetCode ();
    csc.get_r_bits = 0;                   /* not redundant !!! ...........*/

    if (outlen == 0)                  /* must have some space ........*/
    {
      code = CS_END_OUTBUFFER;
      goto ende;
    }

    *csc.outptr++ = (BYTE_TYP) finchar;
    if (--rest_lenr <= 0)             /* End of stream ...............*/
    {
      code = CS_END_OF_STREAM;
      goto ende;
    }
  }

  for (;;)                            /* until not end of inbuf ......*/
  {
    code = GetCode ();
    if (code < 0) break;

    if ((code == CLEAR) && csc.block_compress)
    {
      /* clear code table ............................................*/
      memset (csc.Prefixtab, '\0', sizeof (CODE_ENTRY) << 8);
      csc.clear_flg = 1;
      csc.free_ent = FIRST - 1;

      if ((code = GetCode ()) < 0)
        break;
    }

    incode = code;

    /* Special case for ababa string .................................*/
    if (code >= csc.free_ent)
    {
      *stackp++ = (BYTE_TYP) finchar;
      OVERFLOW_CHECK
      code = oldcode;
    }

    /* Generate output characters in reverse order ...................*/
    while (code >= 256)
    {

      /* Check for end of stack */
      if (stackp >= (DE_STACK + DE_STACK_OFFSET)){
        return (CS_E_STACK_OVERFLOW);
      }

      *stackp++ = TAB_SUFFIXOF(code);
      OVERFLOW_CHECK
      code = TAB_PREFIXOF(code);
    }

    finchar = TAB_SUFFIXOF(code);
    *stackp++ = (BYTE_TYP) finchar;
    OVERFLOW_CHECK

contin:
    /* and put them out in forward order .............................*/
    for (;;)
    {
      if (csc.outptr >= csc.end_outbuf)        /* End of outbuffer ...........*/
      {
        csc.scode    = code;
        csc.sincode  = incode;
        csc.restart  = 1;
        code     = CS_END_OUTBUFFER;
        goto ende;
      }

      *csc.outptr++ = *--stackp;

      if (--rest_lenr <= 0)            /* End of Stream ..............*/
      {
        code = CS_END_OF_STREAM;
        goto ende;
      }

      if (stackp == DE_STACK) break;   /* End of Stack ...............*/
    }  /* end for (;;) ...............................................*/

    /* Generate the new entry ........................................*/
    if ((code = csc.free_ent) < csc.maxmaxcode)
    {
      TAB_PREFIXOF(code) = (CODE_ENTRY)oldcode;
      TAB_SUFFIXOF(code) = (BYTE_TYP) finchar;
      csc.free_ent = code + 1;
    }

    /* Remember previous code ........................................*/
    oldcode = incode;

  }  /* end for (;;) .................................................*/

ende:
  csc.sstackp  = stackp;               /* save state of the compressor ...*/
  csc.soldcode = oldcode;
  csc.sfinchar = finchar;
  csc.rest_len = rest_lenr;
                                   /* set output parameters ..........*/
  *bytes_written = (SAP_INT) (csc.outptr - outbuf);
  *bytes_read    = (SAP_INT) (csc.in_ptr - inbuf);

  if (code == CS_IEND_INBUFFER) return CS_END_INBUFFER;
  else
    return code;
}


CODE_INT CsObjectInt::GetCode (void)
/*--------------------------------------------------------------------*/
/* Read the next code from input stream                               */
/*--------------------------------------------------------------------*/
/* Returns: code on default                                           */
/*          CS_IEND_INBUFFER on end of input buffer                   */
/*                                                                    */
/*--------------------------------------------------------------------*/
{
  register CODE_INT code;

  register int r_off, bits;
  register BYTE_TYP *bp = csc.buf1;

  for (;;)
  {
    if (csc.get_r_bits > 0)   /* does not fit in last run .....*/
    {
      csc.get_r_bits = (SAP_INT) MIN (csc.get_r_bits, csc.end_inbuf - csc.in_ptr);
      csc.get_r_bits = MAX (0, csc.get_r_bits);
      BYTES_IN (csc.buf1 + csc.get_size, csc.in_ptr, csc.get_r_bits)
      csc.get_size += csc.get_r_bits;
      csc.get_r_bits = 0;
      csc.get_size = (csc.get_size << 3) - (csc.n_bits - 1);
      csc.csc_offset = 0;
    }
    else
    if (csc.clear_flg > 0 || csc.csc_offset >= csc.get_size || csc.free_ent > csc.maxcode)
    {
     /*
      * If the next entry will be too big for the current code
      * get_size, then we must increase the get_size.
      * This implies reading a new buffer full, too.
      */

      if (csc.free_ent > csc.maxcode)
      {
        (csc.n_bits)++;
        if (csc.n_bits == csc.maxbits)
          csc.maxcode = csc.maxmaxcode;   /* won't get any bigger now .....*/
        else
          csc.maxcode = MAXCODE(csc.n_bits);
      }

      if (csc.clear_flg > 0)
      {
        csc.maxcode = MAXCODE (csc.n_bits = INIT_CS_BITS);
        csc.clear_flg = 0;
      }

      csc.get_size = (SAP_INT) (csc.end_inbuf - csc.in_ptr);
      if (csc.get_size < csc.n_bits)        /* does not fit in buffer .....*/
      {
        if (csc.get_size <= 0)
        {
          code = CS_IEND_INBUFFER;  /* end of stream ..............*/
          break;
        }
        if (csc.get_r_bits < 0)         /* initial !!! ................*/
          csc.get_r_bits = 0;
        else                        /* end of input buffer ........*/
        {
          BYTES_IN (csc.buf1, csc.in_ptr, csc.get_size)
          csc.get_r_bits = csc.n_bits - csc.get_size;
          csc.csc_offset = 0;
          code = CS_IEND_INBUFFER;
          break;
        }
      }
      else                               /* min (n_bits, get_size) .*/
        csc.get_size = csc.n_bits;

      BYTES_IN (csc.buf1, csc.in_ptr, csc.get_size)
      csc.csc_offset = 0;

      /* Round get_size down to integral number of codes ...........*/
      csc.get_size = (csc.get_size << 3) - (csc.n_bits - 1);
    }

    /* Do all the terrible bit staff ...............................*/
    r_off = csc.csc_offset;
    bits = csc.n_bits;

    bp += (r_off >> 3);
    r_off &= 7;

    /* Get first part (low order bits) .............................*/
    code = (int)*bp >> r_off;
    bp++;
    r_off = 8 - r_off;
    bits -= r_off;

    /* Get any 8 bit parts in the middle (<=1 for up to 16 bits)    */
    if (bits >= 8)
    {
      code |= (int) *bp << r_off;
      bp++;
      r_off += 8;
      bits -= 8;
    }

    /* high order bits .............................................*/
    code |= ((int)*bp & rmask[bits]) << r_off;
    csc.csc_offset += csc.n_bits;
    break;
  }

  return code;
}


int CsObjectInt::PutCode (CODE_INT c)
/*--------------------------------------------------------------------*/
/* Put out  next code to output stream                                */
/*--------------------------------------------------------------------*/
/* Returns: 0    on default                                           */
/*          CS_E_OUT_BUFFER_LEN on end of output buffer               */
/*                                                                    */
/*--------------------------------------------------------------------*/
{
  PUTCODE (c)
  return 0;
}
