/*
# encoding: utf-8
# pysap - Python library for crafting SAP's network protocols packets
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# Author:
#   Martin Gallo (@martingalloar)
#   Code contributed by SecureAuth to the OWASP CBAS project
#   Based on the work performed by Dennis Yurichev <dennis@conus.info>
#
*/

#include <Python.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "hpa101saptype.h"
#include "hpa104CsObject.h"
#include "hpa106cslzc.h"
#include "hpa107cslzh.h"
#include "hpa105CsObjInt.h"


/* Memory allocation factor constant */
#define MEMORY_ALLOC_FACTOR 10


/* Compression algorithm constants */
#define ALG_LZC CS_LZC
#define ALG_LZH CS_LZH


/* Return code for memory errors */
#define CS_E_MEMORY_ERROR -99


/* Returns an error strings for compression library return codes */
const char *error_string(int return_code){
	switch (return_code){
		case CS_IEND_OF_STREAM: return ("CS_IEND_OF_STREAM: end of data (internal)");
		case CS_IEND_OUTBUFFER: return ("CS_IEND_OUTBUFFER: end of output buffer");
		case CS_IEND_INBUFFER: return ("CS_IEND_INBUFFER: end of input buffer");
		case CS_E_OUT_BUFFER_LEN: return ("CS_E_OUT_BUFFER_LEN: invalid output length");
		case CS_E_IN_BUFFER_LEN: return ("CS_E_IN_BUFFER_LEN: invalid input length");
		case CS_E_NOSAVINGS: return ("CS_E_NOSAVINGS: no savings");
		case CS_E_INVALID_SUMLEN: return ("CS_E_INVALID_SUMLEN: invalid len of stream");
		case CS_E_IN_EQU_OUT: return ("CS_E_IN_EQU_OUT: inbuf == outbuf");
		case CS_E_INVALID_ADDR: return ("CS_E_INVALID_ADDR: inbuf == NULL,outbuf == NULL");
		case CS_E_FATAL: return ("CS_E_FATAL: internal error !");
		case CS_E_BOTH_ZERO: return ("CS_E_BOTH_ZERO: inlen = outlen = 0");
		case CS_E_UNKNOWN_ALG: return ("CS_E_UNKNOWN_ALG: unknown algorithm");
		case CS_E_UNKNOWN_TYPE: return ("CS_E_UNKNOWN_TYPE: unknown type");
		/* for decompress */
		case CS_E_FILENOTCOMPRESSED: return ("CS_E_FILENOTCOMPRESSED: input not compressed");
		case CS_E_MAXBITS_TOO_BIG: return ("CS_E_MAXBITS_TOO_BIG: maxbits to large");
		case CS_E_BAD_HUF_TREE: return ("CS_E_BAD_HUF_TREE: bad hufman tree");
		case CS_E_NO_STACKMEM: return ("CS_E_NO_STACKMEM: no stack memory in decomp");
		case CS_E_INVALIDCODE: return ("CS_E_INVALIDCODE: invalid code");
		case CS_E_BADLENGTH: return ("CS_E_BADLENGTH: bad lengths");
		case CS_E_STACK_OVERFLOW: return ("CS_E_STACK_OVERFLOW: stack overflow in decomp");
		case CS_E_STACK_UNDERFLOW: return ("CS_E_STACK_UNDERFLOW: stack underflow in decomp");
		/* only Windows */
		case CS_NOT_INITIALIZED: return ("CS_NOT_INITIALIZED: storage not allocated");
		/* non error return codes */
		case CS_END_INBUFFER: return ("CS_END_INBUFFER: end of input buffer");
		case CS_END_OUTBUFFER: return ("CS_END_OUTBUFFER: end of output buffer");
		case CS_END_OF_STREAM: return ("CS_END_OF_STREAM: end of data");
		/* custom error */
		case CS_E_MEMORY_ERROR: return ("CS_E_MEMORY_ERROR: custom memory error");
		/* unknown error */
		default: return ("unknown error");
	}
}


/* Custom exception for compression library errors */
static char compression_exception_short[] = "CompressError";
static char compression_exception_name[] = "pysapcompress.CompressError";
PyObject *compression_exception = NULL;

static char decompression_exception_short[] = "DecompressError";
static char decompression_exception_name[] = "pysapcompress.DecompressError";
PyObject *decompression_exception = NULL;


/* Hexdump helper function for debugging */
void hexdump(unsigned char *address, unsigned int size)
{
	unsigned int i = 0, j = 0, offset = 0;
	printf("[%08x] ", offset);
	for (; i<size; i++){
		j++; printf("%02x ", address[i]);
		if (j==8) printf(" ");
		if (j==16){
			offset+=j; printf("\n[%08x] ", offset); j=0;
		}
	}
	printf("\n");
}


/* Decompress a packet buffer */
int decompress_packet (const unsigned char *in, const int in_length, unsigned char **out, int *out_length)
{
	class CsObjectInt csObject;
	int rt = 0, finished = false;
	SAP_BYTE *bufin = NULL, *bufin_pos = NULL, *bufout = NULL, *bufout_pos = NULL;
	SAP_INT bufin_length = 0, bufin_rest = 0, bufout_length = 0, bufout_rest = 0, data_length = 0, bytes_read = 0, bytes_decompressed = 0, total_decompressed = 0;

#ifdef DEBUG
	printf("pysapcompress.cpp: Decompressing (%d bytes, reported length of %d bytes)...\n", in_length, *out_length);
#endif

	*out = NULL;

	/* Check for invalid inputs */
	if (in == NULL)
		return (CS_E_INVALID_ADDR);
	if (in_length <= 0)
		return (CS_E_IN_BUFFER_LEN);
	if (*out_length <= 0)
		return (CS_E_OUT_BUFFER_LEN);

	/* Point the input buffer to the input */
	bufin = bufin_pos = (SAP_BYTE*) in;
	bufin_length = bufin_rest = (SAP_INT)in_length;

	/* Initialize and obtain the reported uncompressed data length */
	rt = csObject.CsInitDecompr(bufin);
	if (rt != 0){
#ifdef DEBUG
		printf("pysapcompress.cpp: Initialization failed !\n");
#endif
		*out_length = 0;
		return (rt);
	}

	/* Check the length in the header vs the reported one */
	data_length = csObject.CsGetLen(bufin);
	if (data_length != *out_length){
#ifdef DEBUG
		printf("pysapcompress.cpp: Length reported (%d) doesn't match with the one in the header (%d)\n", *out_length, data_length);
#endif
		*out_length = 0;
		return (CS_E_OUT_BUFFER_LEN);
	}

	/* Advance the buffer pointer as we've already read the header */
	bufin_pos += CS_HEAD_SIZE;

#ifdef DEBUG
	printf("pysapcompress.cpp: Initialized, reported length in header: %d bytes\n", data_length);
#endif

	/* Allocate the output buffer. We use the reported output size
	 * as the output buffer size.
	 */
	bufout_length = bufout_rest = *out_length;
	bufout = bufout_pos = (SAP_BYTE*) malloc(bufout_length);
	if (!bufout){
		*out_length = 0;
		return (CS_E_MEMORY_ERROR);
	}
	memset(bufout, 0, bufout_length);

#ifdef DEBUG_TRACE
	printf("pysapcompress.cpp: Input buffer %p (%d bytes), output buffer %p (%d bytes)\n", bufin, bufin_length, bufout, bufout_length);
#endif

	while (finished == false && bufin_rest > 0 && bufout_rest > 0) {

#ifdef DEBUG_TRACE
		printf("pysapcompress.cpp: Input position %p (rest %d bytes), output position %p\n", bufin_pos, bufin_rest, bufout_pos);
#endif
		rt = csObject.CsDecompr(bufin_pos, bufin_rest, bufout_pos, bufout_rest, 0, &bytes_read, &bytes_decompressed);

#ifdef DEBUG
		printf("pysapcompress.cpp: Return code %d (%s) (%d bytes read, %d bytes decompressed)\n", rt, error_string(rt), bytes_read, bytes_decompressed);
#endif

		/* Successful decompression, we've finished with the stream */
		if (rt == CS_END_OF_STREAM){
			finished = true;
		}
		/* Some error occurred */
		if (rt != CS_END_INBUFFER && rt != CS_END_OUTBUFFER){
			finished = true;
		}

		/* Advance the input buffer */
		bufin_pos += bytes_read;
		bufin_rest -= bytes_read;
		/* Advance the output buffer */
		bufout_pos += bytes_decompressed;
		bufout_rest -= bytes_decompressed;
		total_decompressed += bytes_decompressed;

	}

	/* Successful decompression */
	if (rt == CS_END_OF_STREAM) {
		*out_length = total_decompressed;

        /* Allocate the required memory */
        *out = (unsigned char *)malloc(total_decompressed);
        if (*out){

		    /* Copy the buffer in the out parameter */
		    for (int i = 0; i < total_decompressed; i++)
			    (*out)[i] = (char) bufout[i];

#ifdef DEBUG_TRACE
		    printf("pysapcompress.cpp: Out buffer:\n");
			hexdump(*out, total_decompressed);
#endif
		/* Memory error */
        } else {
    		rt = CS_E_MEMORY_ERROR; *out_length = 0;
        }
	}

	/* Free the buffers */
	free(bufout);

#ifdef DEBUG
	printf("pysapcompress.cpp: Out Length: %d\n", *out_length);
#endif

	return (rt);
};


/* Compress a packet buffer */
int compress_packet (const unsigned char *in, const int in_length, unsigned char **out, int *out_length, const unsigned int algorithm)
{
	class CsObjectInt csObject;
	int rt = 0, finished = false;
	SAP_BYTE *bufin = NULL, *bufin_pos = NULL, *bufout = NULL, *bufout_pos = NULL;
	SAP_INT bufin_length = 0, bufin_rest = 0, bytes_read = 0, bufout_length = 0, bufout_rest = 0, bytes_compressed = 0, total_compressed = 0;

#ifdef DEBUG
	printf("pysapcompress.cpp: Compressing (%d bytes) using algorithm %s ...\n", in_length, algorithm==ALG_LZC?"LZC":algorithm==ALG_LZH?"LZH":"unknown");
#endif

	*out = NULL; *out_length = 0;

	/* Check for invalid inputs */
	if (in == NULL)
		return (CS_E_INVALID_ADDR);
	if (in_length <= 0)
		return (CS_E_IN_BUFFER_LEN);

	/* Point the input buffer to the input */
	bufin = bufin_pos = (SAP_BYTE*) in;
	bufin_length = bufin_rest = (SAP_INT)in_length;

	/* Allocate the output buffer. We use the input size with a constant factor
	 * as the output buffer size.
	 */
	bufout_length = bufout_rest = bufin_length * MEMORY_ALLOC_FACTOR;
	bufout = bufout_pos = (SAP_BYTE*) malloc(bufout_length);
	if (!bufout){
		return (CS_E_MEMORY_ERROR);
	}
	memset(bufout, 0, bufout_length);

	/* Initialize */
	rt = csObject.CsInitCompr(bufout, bufin_length, algorithm);
	if (rt != 0){
#ifdef DEBUG
		printf("pysapcompress.cpp: Initialization failed !\n");
#endif
		free(bufout);
		return (rt);
	}

	/* Advance the buffer pointer as we've already written the header */
	bufout_pos += CS_HEAD_SIZE;
	bufout_rest -= CS_HEAD_SIZE;
	total_compressed += CS_HEAD_SIZE;

#ifdef DEBUG_TRACE
	printf("pysapcompress.cpp: Input buffer %p (%d bytes), output buffer %p (%d bytes)\n", bufin, bufin_length, bufout, bufout_length);
#endif

	while (finished == false && bufin_rest > 0 && bufout_rest > 0){

#ifdef DEBUG_TRACE
		printf("pysapcompress.cpp: Input position %p (rest %d bytes), output position %p\n", bufin_pos, bufin_rest, bufout_pos);
#endif
		/* Compress the buffer */
		rt = csObject.CsCompr(bufin_length, bufin_pos, bufin_rest, bufout_pos, bufout_rest, algorithm, &bytes_read, &bytes_compressed);

#ifdef DEBUG
		printf("pysapcompress.cpp: Return code %d (%s) (%d bytes read, %d bytes compressed)\n", rt, error_string(rt), bytes_read, bytes_compressed);
#endif
		/* Successful decompression, we've finished with the stream */
		if (rt == CS_END_OF_STREAM){
			finished = true;
		}
		/* Some error occurred */
		if (rt != CS_END_INBUFFER && rt != CS_END_OUTBUFFER){
			finished = true;
		}

		/* Advance the input buffer */
		bufin_pos += bytes_read;
		bufin_rest -= bytes_read;
		/* Advance the output buffer */
		bufout_pos += bytes_compressed;
		bufout_rest -= bytes_compressed;
		total_compressed += bytes_compressed;
	}

	/* Successful compression */
	if (rt == CS_END_OF_STREAM) {
		*out_length = total_compressed;

        /* Allocate the required memory */
        *out = (unsigned char *)malloc(total_compressed);
        if (*out){

		    /* Copy the buffer in the out parameter */
		    memcpy(*out, bufout, total_compressed);

		    /* Set the compression header */
		    csObject.CsSetHead(bufout, bufin_length, algorithm, 0);

#ifdef DEBUG_TRACE
		    printf("pysapcompress.cpp: Out buffer:\n");
			hexdump(*out, total_compressed);
#endif
		/* Memory error */
        } else {
        	rt = CS_E_MEMORY_ERROR; *out_length = 0;
        }
	}

	/* Free the buffers */
	free(bufout);

#ifdef DEBUG
	printf("pysapcompress.cpp: Out Length: %d\n", *out_length);
#endif

	return (rt);
};


/* Compress Python function */
static char pysapcompress_compress_doc[] = "Compress a buffer using SAP's compression algorithms.\n\n"
                                           ":param str in: input buffer to compress\n\n"
                                           ":param int algorithm: algorithm to use\n\n"
                                           ":return: tuple with return code, output length and output buffer\n"
                                           ":rtype: tuple of int, int, string\n\n"
                                           ":raises CompressError: if an error occurred during compression\n";

static PyObject *
pysapcompress_compress(PyObject *self, PyObject *args, PyObject *keywds)
{
    const unsigned char *in = NULL;
    unsigned char *out = NULL;
    int status = 0, in_length = 0, out_length = 0, algorithm = ALG_LZC;

    /* Define the keyword list */
    static char kwin[] = "in";
    static char kwalgorithm[] = "algorithm";
    static char* kwlist[] = {kwin, kwalgorithm, NULL};

    /* Parse the parameters. We are also interested in the length of the input buffer. */
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "s#|i", kwlist, &in, &in_length, &algorithm))
        return (NULL);

    /* Call the compression function */
    status = compress_packet(in, in_length, &out, &out_length, algorithm);

    /* Perform some exception handling */
    if (status < 0){
    	/* If it was a memory error in this module, raise a NoMemory standard exception */
    	if (status==CS_E_MEMORY_ERROR)
    		return (PyErr_NoMemory());
    	/* If the error was in the compression module, raise a custom exception */
    	else {
    		return (PyErr_Format(compression_exception, "Compression error (%s)", error_string(status)));
    	}
    }

    /* It no error was raised, return the compressed buffer and the length */
	return (Py_BuildValue("iis#", status, out_length, out, out_length));
}


/* Decompress Python function */
static char pysapcompress_decompress_doc[] = "Decompress a buffer using SAP's compression algorithms.\n\n"
                                             ":param str in: input buffer to decompress\n"
                                             ":param int out_length: length of the output to decompress\n"
                                             ":return: tuple of return code, output length and output buffer\n"
                                             ":rtype: tuple of int, int, string\n\n"
                                             ":raises DecompressError: if an error occurred during decompression\n";

static PyObject *
pysapcompress_decompress(PyObject *self, PyObject *args)
{
    const unsigned char *in = NULL;
    unsigned char *out = NULL;
    int status = 0, in_length = 0, out_length = 0;

    /* Parse the parameters. We are also interested in the length of the input buffer. */
    if (!PyArg_ParseTuple(args, "s#i", &in, &in_length, &out_length))
        return (NULL);

    /* Call the compression function */
    status = decompress_packet(in, in_length, &out, &out_length);

    /* Perform some exception handling */
    if (status < 0){
    	/* If it was a memory error in this module, raise a NoMemory standard exception */
    	if (status==CS_E_MEMORY_ERROR)
    		return (PyErr_NoMemory());
    	/* If the error was in the compression module, raise a custom exception */
    	else
    		return (PyErr_Format(decompression_exception, "Decompression error (%s)", error_string(status)));
    }
    /* It no error was raised, return the uncompressed buffer and the length */
    return (Py_BuildValue("iis#", status, out_length, out, out_length));
}


/* Method definitions */
static PyMethodDef pysapcompressMethods[] = {
    {"compress", (PyCFunction)pysapcompress_compress, METH_VARARGS | METH_KEYWORDS, pysapcompress_compress_doc},
    {"decompress", pysapcompress_decompress, METH_VARARGS, pysapcompress_decompress_doc},
    {NULL, NULL, 0, NULL}
};


/* pysapcompress module doc string */
static char pysapcompress_module_doc[] = "Library implementing SAP's LZH and LZC compression algorithms.";

/* Module initialization */
PyMODINIT_FUNC
initpysapcompress(void)
{
    PyObject *module = NULL;
    /* Create the module and define the methods */
    module = Py_InitModule3("pysapcompress", pysapcompressMethods, pysapcompress_module_doc);

    /* Add the algorithm constants */
    PyModule_AddIntConstant(module, "ALG_LZC", ALG_LZC);
    PyModule_AddIntConstant(module, "ALG_LZH", ALG_LZH);

    /* Create a custom exception and add it to the module */
    compression_exception = PyErr_NewException(compression_exception_name, NULL, NULL);
    PyModule_AddObject(module, compression_exception_short, compression_exception);

    decompression_exception = PyErr_NewException(decompression_exception_name, NULL, NULL);
    PyModule_AddObject(module, decompression_exception_short, decompression_exception);

}
