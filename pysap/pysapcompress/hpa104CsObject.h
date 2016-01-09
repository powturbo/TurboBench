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
// CsObject.h: interface for the CsObject class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CSOBJECT_H__2AFFECE5_60FE_11D2_87FA_204C4F4F5020__INCLUDED_)
#define AFX_CSOBJECT_H__2AFFECE5_60FE_11D2_87FA_204C4F4F5020__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef NEAR
#ifdef SAPonWINDOWS
#define NEAR _near
#else
#define NEAR
#endif
#endif


/*--------------------------------------------------------------------*/
/* Flags for CsCompr and CsDecompr                                    */
/*--------------------------------------------------------------------*/
#define CS_LZH_VERSION          1

#define CS_LZH1                 (1 << 4)
#define CS_LZH2                 (2 << 4)
#define CS_LZH3                 (3 << 4)
#define CS_LZH4                 (4 << 4)
#define CS_LZH5                 (5 << 4)
#define CS_LZH6                 (6 << 4)
#define CS_LZH7                 (7 << 4)
#define CS_LZH8                 (8 << 4)
#define CS_LZH9                 (9 << 4)


/* call flags for compression ........................................*/
#define CS_NORMAL_COMPRESS      0x0    /* normal .....................*/
#define CS_INIT_COMPRESS        0x1    /* first call CsCompr..........*/
#define CS_INIT_DECOMPRESS      0x1    /* first call CsDeCompr........*/

#define CS_LZC                  0x0    /* use lzc ....................*/
#define CS_LZH                  0x2    /* use lzh ....................*/
#define CS_GRAPHIC_DATA         0x8    /* not supported now ..........*/

/* Header info in compressed file ....................................*/
#define CS_ALGORITHM_LZC        (SAP_BYTE) 1
#define CS_ALGORITHM_LZH        (SAP_BYTE) 2

#define CS_HEAD_SIZE             8     /* size of common header ......*/

/*--------------------------------------------------------------------*/
/* Error & Return Codes for CsCompr and CsDecompr                     */
/*--------------------------------------------------------------------*/
#define CS_END_INBUFFER          3     /* End of input buffer ........*/
#define CS_END_OUTBUFFER         2     /* End of output buffer .......*/
#define CS_END_OF_STREAM         1     /* End of data ................*/
#define CS_OK                    0

#define CS_IEND_OF_STREAM       -1     /* End of data (internal) .....*/
#define CS_IEND_OUTBUFFER       -2     /* End of output buffer .......*/
#define CS_IEND_INBUFFER        -3     /* End of input buffer ........*/

#define CS_ERROR               -10     /* First Error Code ...........*/
#define CS_E_OUT_BUFFER_LEN    -10     /* Invalid output length ......*/
#define CS_E_IN_BUFFER_LEN     -11     /* Invalid input length .......*/
#define CS_E_NOSAVINGS         -12
#define CS_E_INVALID_SUMLEN    -13     /* Invalid len of stream ......*/
#define CS_E_IN_EQU_OUT        -14     /* inbuf == outbuf ............*/
#define CS_E_INVALID_ADDR      -15     /* inbuf == NULL,outbuf == NULL*/
#define CS_E_FATAL             -19     /* Internal Error ! ...........*/
#define CS_E_BOTH_ZERO         -20     /* inlen = outlen = 0 .........*/
#define CS_E_UNKNOWN_ALG       -21     /* unknown algorithm ..........*/
#define CS_E_UNKNOWN_TYPE      -22

/* for decompress */
#define CS_E_FILENOTCOMPRESSED -50     /* Input not compressed .......*/
#define CS_E_MAXBITS_TOO_BIG   -51     /* maxbits to large ...........*/
#define CS_E_BAD_HUF_TREE      -52     /* bad hufman tree   ..........*/
#define CS_E_NO_STACKMEM       -53     /* no stack memory in decomp ..*/
#define CS_E_INVALIDCODE       -54     /* invalid code ...............*/
#define CS_E_BADLENGTH         -55     /* bad lengths ................*/

#define CS_E_STACK_OVERFLOW    -60     /* stack overflow in decomp    */
#define CS_E_STACK_UNDERFLOW   -61     /* stack underflow in decomp   */

/* only Windows */
#define CS_NOT_INITIALIZED     -71     /* storage not allocated ......*/


/* Internal Implementation class */
class CsObjectInt;

class CsObject
{
private:
    CsObjectInt * Cs;

public:
	CsObject ();
	virtual ~CsObject ();

	virtual int CsCompr (SAP_INT    sumlen,
             SAP_BYTE * inbuf,
             SAP_INT    inlen,
             SAP_BYTE * outbuf,
             SAP_INT    outlen,
             SAP_INT    option,
             SAP_INT  * bytes_read,
             SAP_INT  * bytes_written);

	virtual int CsDecompr (SAP_BYTE * inbuf,     /* ptr input .......*/
               SAP_INT    inlen,         /* len of input ....*/
               SAP_BYTE * outbuf,        /* ptr output ......*/
               SAP_INT    outlen,        /* len output ......*/
               SAP_INT    option,        /* decompr. option  */
               SAP_INT *  bytes_read,    /* bytes read ......*/
               SAP_INT *  bytes_decompressed); /* bytes decompr.  */

	virtual int CsGetAlgorithm (SAP_BYTE * data);

	virtual SAP_INT CsGetLen (SAP_BYTE * data);
	virtual int CsGetVersion (SAP_BYTE * data);

	virtual int CsSetHead (SAP_BYTE * outbuf,
               SAP_INT    len,
               SAP_BYTE   veralg,
               SAP_BYTE   special);

	virtual int CsInitCompr (SAP_BYTE * outbuf, SAP_INT sumlen, SAP_INT option);
	virtual int CsInitDecompr (SAP_BYTE * inbuf);
};

#endif // !defined(AFX_CSOBJECT_H__2AFFECE5_60FE_11D2_87FA_204C4F4F5020__INCLUDED_)
