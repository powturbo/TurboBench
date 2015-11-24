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
// CsObjectInt.cpp: implementation of the CSObjectInt class.
//
//////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hpa101saptype.h"
// #include "cscompr.h"
#include "hpa106cslzc.h"
#include "hpa107cslzh.h"

#include "hpa104CsObject.h"
#include "hpa105CsObjInt.h"


static SAP_BYTE CsMagicHead[] = { "\037\235" };  /* 1F 9D */

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


CsObjectInt::CsObjectInt ()
{
}

CsObjectInt::~CsObjectInt ()
{
}

int CsObjectInt::CsCompr (SAP_INT    sumlen,
             SAP_BYTE * inbuf,
             SAP_INT    inlen,
             SAP_BYTE * outbuf,
             SAP_INT    outlen,
             SAP_INT    option,
             SAP_INT  * bytes_read,
             SAP_INT  * bytes_written)
/*--------------------------------------------------------------------*/
/* Compress a memory segmented                                        */
/*                                                                    */
/* Adaptive Dictionary Compression                                    */
/*   Lempel-Zip                                                       */
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
/*                             CS_LZC or CS_LZH      initial          */
/*                             initial options have to be or'ed       */
/*                             CS_NORMAL_COMPRESS                     */
/* Output:                                                            */
/* ------                                                             */
/*        bytes_read        Bytes read from input buffer              */
/*        bytes_compressed  Bytes compressed to output buffer         */
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

  if (option & CS_INIT_COMPRESS)
  {
	algorithm = 0;
    algorithm = (int) (option & 0xE);
  }

  /* Plausibility check for negative input length */
  if (inlen < 0) inlen = 0;

  switch (algorithm)
  {
    case CS_LZC:
      return CsComprLZC (sumlen, inbuf, inlen, outbuf, outlen,
                         option, bytes_read, bytes_written);
    case CS_LZH:
      return CsComprLZH (sumlen, inbuf, inlen, outbuf, outlen,
                         option, bytes_read, bytes_written);

    default: return CS_E_UNKNOWN_ALG;
  }
}


int CsObjectInt::CsDecompr (SAP_BYTE * inbuf,         /* ptr input .......*/
               SAP_INT    inlen,         /* len of input ....*/
               SAP_BYTE * outbuf,        /* ptr output ......*/
               SAP_INT    outlen,        /* len output ......*/
               SAP_INT    option,        /* decompr. option  */
               SAP_INT *  bytes_read,    /* bytes read ......*/
               SAP_INT *  bytes_decompressed) /* bytes decompr.  */
/*--------------------------------------------------------------------*/
/*     Decompress                                                     */
/*                                                                    */
/* Adaptive Dictionary Compression                                    */
/*   Lempel-Zip                                                       */
/*                                                                    */
/* Input:                                                             */
/* -----                                                              */
/*        inbuf             Pointer to input memory                   */
/*        inlen             Length of input memory                    */
/*        outbuf            Pointer to output area                    */
/*        outlen            Length of output area                     */
/*        option            DeCompress option:                        */
/*                             CS_INIT_DECOMPRESS      initial        */
/*                             CS_NORMAL_COMPRESS                     */
/*                                                                    */
/* Output:                                                            */
/* ------                                                             */
/*        bytes_read        Bytes read from input buffer              */
/*        bytes_written     Bytes decompressed to output buffer       */
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

  if (option & CS_INIT_DECOMPRESS)
  {
    if (inlen < CS_HEAD_SIZE) return CS_E_IN_BUFFER_LEN;
    algorithm = CsGetAlgorithm (inbuf);
  }

  switch (algorithm)
  {
    case CS_ALGORITHM_LZC:
      return CsDecomprLZC (inbuf, inlen, outbuf, outlen,
                           option, bytes_read, bytes_decompressed);
    case CS_ALGORITHM_LZH:
      return CsDecomprLZH (inbuf, inlen, outbuf, outlen,
                           option, bytes_read, bytes_decompressed);

    default: return CS_E_UNKNOWN_ALG;
  }
}


int CsObjectInt::CsGetAlgorithm (SAP_BYTE * data)
/*--------------------------------------------------------------------*/
/* Get Algorithm number of compressed data                            */
/*--------------------------------------------------------------------*/
{
   return ((int) (data[4] & (unsigned char)0x0F));
}


SAP_INT CsObjectInt::CsGetLen (SAP_BYTE * data)
/*--------------------------------------------------------------------*/
/* Get the length of the original data stream                         */
/*                                                                    */
/*   Returns CS_E_FILENOT_COMPRESSED if the magic number is           */
/*                                      different from magic header   */
/*                                   else                             */
/*           Length of org. data stream                               */
/*--------------------------------------------------------------------*/
{
  SAP_INT len;
                                      /* file not compressed !!! .....*/
  if ((CsMagicHead[0] != data[5]) ||
      (CsMagicHead[1] != data[6]))
  {
    return ((SAP_INT)CS_E_FILENOTCOMPRESSED);
  }

  len = (SAP_INT)data[0]         +    /* read length from first buf ..*/
        ((SAP_INT)data[1] << 8)  +
        ((SAP_INT)data[2] << 16) +
        ((SAP_INT)data[3] << 24);

  return len;
}


int CsObjectInt::CsGetVersion (SAP_BYTE * data)
/*--------------------------------------------------------------------*/
/* Get version number of compressed data                              */
/*--------------------------------------------------------------------*/
{
   return ((int)data[4] & 0xF0) >> 4;
}


int CsObjectInt::CsSetHead (SAP_BYTE * outbuf,
               SAP_INT    len,
               SAP_BYTE   veralg,
               SAP_BYTE   special)
/*--------------------------------------------------------------------*/
/* Setup length of data stream and header information                 */
/*--------------------------------------------------------------------*/
{
  SAP_UINT l;

  if (len < 0) return CS_E_INVALID_SUMLEN;

  l = (SAP_UINT) len;

  /* set length ......................................................*/
  outbuf[0] = (SAP_BYTE) (l & 0xff);
  outbuf[1] = (SAP_BYTE) ((l & 0xff00) >> 8);
  outbuf[2] = (SAP_BYTE) ((l & 0xff0000L) >> 16);
  outbuf[3] = (SAP_BYTE) ((l & 0xff000000L) >> 24);

  /* setup header - magic number and maxbits .........................*/
  outbuf[4] = veralg;
  outbuf[5] = CsMagicHead[0];
  outbuf[6] = CsMagicHead[1];
  outbuf[7] = special;

  return 0;
}


int CsObjectInt::CsInitCompr (SAP_BYTE * outbuf, SAP_INT sumlen, SAP_INT option)
/*--------------------------------------------------------------------*/
/* Only initalize Compression                                         */
/*   outbuf must point to a location which has at least               */
/*   CS_HEAD_SIZE bytes left                                          */
/*                                                                    */
/* Input:                                                             */
/* -----                                                              */
/*        outbuf            Pointer to output memory                  */
/*        sumlen            length of data stream to compress         */
/*        option            Compress option CS_LZC, CS_LZH            */
/*                                                                    */
/* Return Code:                                                       */
/* -----------                                                        */
/*        0                    OK                                     */
/*                                                                    */
/*        CS_E_OUT_BUFFER_LEN  Output Buffer length to short          */
/*        CS_E_INVALID_SUMLEN  sumlen <= 0                            */
/*        CS_E_INVALID_ADDR    Invalid addr for input or output buffer*/
/*        CS_E_FATAL           internal (should never happen)         */
/*                                                                    */
/*--------------------------------------------------------------------*/
{
  SAP_BYTE input_field;
  SAP_BYTE *inbuf = &input_field;
  SAP_INT bytes_read, bytes_written;
  int rc;

  rc = CsCompr (sumlen, inbuf, 0, outbuf,
                CS_HEAD_SIZE, option | CS_INIT_COMPRESS,
                &bytes_read, &bytes_written);

  if (rc < 0) return rc;
  return 0;
}


int CsObjectInt::CsInitDecompr (SAP_BYTE * inbuf)
/*--------------------------------------------------------------------*/
/* Only initalize Decompression                                       */
/*   inbuf must point to a location which has at least                */
/*   CS_HEAD_SIZE bytes left                                          */
/*                                                                    */
/* Input:                                                             */
/* -----                                                              */
/*        inbuf             Pointer to input memory                   */
/*                                                                    */
/* Return Code:                                                       */
/* -----------                                                        */
/*        0                    OK                                     */
/*                                                                    */
/*        CS_E_IN_BUFFER_LEN   Input Buffer length to short           */
/*        CS_E_INVALID_ADDR    Invalid addr for input                 */
/*        CS_E_FATAL           internal (should never happen)         */
/*                                                                    */
/*--------------------------------------------------------------------*/
{
  SAP_BYTE output_field;
  SAP_BYTE * outbuf = &output_field;
  SAP_INT bytes_read, bytes_written;
  int rc;

  rc = CsDecompr (inbuf, CS_HEAD_SIZE,
                  outbuf, 0, CS_INIT_DECOMPRESS,
                  &bytes_read, &bytes_written);

  if (rc < 0) return rc;
  return 0;
}
