/*
**  $Id: test_lzmat.c,v 1.1 2008/07/08 17:51:01 Vitaly Exp $
**  $Revision: 1.1 $
**  $Date: 2008/07/08 17:51:01 $
** 
**  $Author: Vitaly $
**
***************************************************************************
** This program shows the basic usage of the LZMAT library.
** test_lzmat.c
** This file written and distributed to public domain by Vitaly Evseenko.
** This file is part of LZMAT compression library.
**
**
** The LZMAT library is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License as
** published by the Free Software Foundation; either version 2 of
** the License, or (at your option) any later version.
**
** The LZMAT library is distributed WITHOUT ANY WARRANTY;
** without even the implied warranty of MERCHANTABILITY or
** FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
** License for more details.
**
** You should have received a copy of the GNU General Public License
** along with the LZMAT library; see the file GPL.TXT.
** If not, write to the Free Software Foundation, Inc.,
** 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
**
** Vitaly Evseenko
** <ve@matcode.com>
** http://www.matcode.com/lzmat.htm
***************************************************************************
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <tchar.h>
#include "lzmat.h"

int _tmain(int argc, _TCHAR *argv[])
{
	FILE *f;
	unsigned char *buf, *buf1;
	int cb, cbOut, encode, err;

	_tprintf(_T("\nTest program for the LZMAT compression library.\nCopyright (c) 2007,2008 Vitaly Evseenko\n\n"));
	if(argc<4)
	{
		_tprintf(_T("Arguments required!\nUSAGE: test_lzmat <e|d> <InputFile> <OutputFile>\n"));
		return -1;
	}
	f=_tfopen(argv[2], _T("rb"));
	if(f==NULL)
	{
		_tprintf(_T("LZMAT: unable to open %s. Error: #%d\n"), argv[2], errno);
		return -1;
	}
	fseek(f, 0, SEEK_END);
	cb=ftell(f);
	fseek(f, 0, SEEK_SET);
	buf = (unsigned char *)malloc(cb);
	cb = fread(buf, 1, cb, f);
	fclose(f);
	if((char)(argv[1][0]|0x20)=='e')
		encode = 1;
	else if((char)(argv[1][0]|0x20)=='d')
		encode = 0;
	else
	{
		_tprintf(_T("LZMAT: invalid argument!\n"));
		return -1;
	}
//__asm int 3;
	if(encode)
	{
		cbOut=MAX_LZMAT_ENCODED_SIZE(cb);
		buf1 = (unsigned char *)malloc(cbOut+sizeof(int));

		*(int *)buf1 = cb;
		err = lzmat_encode(buf1+sizeof(int), &cbOut, buf, cb);
		if(err)
		{
			_tprintf(_T("LZMAT: encode err! #%d\n"), err);
			return err;
		}
		_tprintf(_T("LZMAT: encoded %d bytes to %d bytes\n"), cb, cbOut);
		f=_tfopen(argv[3], _T("w+b"));
		if(f==NULL)
		{
			_tprintf(_T("LZMAT: unable to open %s! Error: #%d\n"), argv[3], errno);
			return -1;
		}
		fwrite(buf1, 1, cbOut+sizeof(int), f);
		fclose(f);
		return 0;
	}
	cbOut = *(int *)buf;
	cb-=sizeof(int);
	buf1 = (unsigned char *)malloc(cbOut);
	err = lzmat_decode(buf1, &cbOut, buf+sizeof(int), cb);
	if(err)
	{
		_tprintf(_T("LZMAT: decode error %d\n"), err);
		return err;
	}
	_tprintf(_T("LZMAT: decoded %d to %d\n"), cb, cbOut);
	f=_tfopen(argv[3], _T("w+b"));
	if(f==NULL)
	{
		_tprintf(_T("LZMAT: unable to open %s! Error: #%d\n"), argv[3], errno);
		return -1;
	}
	fwrite(buf1, 1, cbOut, f);
	fclose(f);
	return 0;
}

