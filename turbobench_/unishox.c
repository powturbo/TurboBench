#include <stdio.h>

#include "../Unishox/unishox2.h"
int unishox2_compressx(const char *in, int inlen, char *out, int lev) {
  switch(lev) {
    case  0: return unishox2_compress_lines(in, inlen, out, USX_PSET_DFLT,                   NULL);
    case  1: return unishox2_compress_lines(in, inlen, out, USX_PSET_ALPHA_ONLY,             NULL);
    case  2: return unishox2_compress_lines(in, inlen, out, USX_PSET_ALPHA_NUM_ONLY,         NULL);
    case  3: return unishox2_compress_lines(in, inlen, out, USX_PSET_ALPHA_NUM_SYM_ONLY,     NULL);
    case  4: return unishox2_compress_lines(in, inlen, out, USX_PSET_ALPHA_NUM_SYM_ONLY_TXT, NULL);
    case  5: return unishox2_compress_lines(in, inlen, out, USX_PSET_FAVOR_ALPHA,            NULL);
    case  6: return unishox2_compress_lines(in, inlen, out, USX_PSET_FAVOR_DICT,             NULL);
    case  7: return unishox2_compress_lines(in, inlen, out, USX_PSET_FAVOR_SYM,              NULL);
    case  8: return unishox2_compress_lines(in, inlen, out, USX_PSET_FAVOR_UMLAUT,           NULL);
    case  9: return unishox2_compress_lines(in, inlen, out, USX_PSET_NO_DICT,                NULL);
    case 10: return unishox2_compress_lines(in, inlen, out, USX_PSET_NO_UNI,                 NULL);
    case 11: return unishox2_compress_lines(in, inlen, out, USX_PSET_NO_UNI_FAVOR_TEXT,      NULL);
    case 12: return unishox2_compress_lines(in, inlen, out, USX_PSET_URL,                    NULL);
    case 13: return unishox2_compress_lines(in, inlen, out, USX_PSET_JSON,                   NULL);
    case 14: return unishox2_compress_lines(in, inlen, out, USX_PSET_JSON_NO_UNI,            NULL);
    case 15: return unishox2_compress_lines(in, inlen, out, USX_PSET_XML,                    NULL);
    case 16: return unishox2_compress_lines(in, inlen, out, USX_PSET_HTML,                   NULL);
  }
}

int unishox2_decompressx(const char *in, int inlen, char *out, int lev) {
  switch(lev) {
    case  0: return unishox2_decompress_lines(in, inlen, out, USX_PSET_DFLT,                   NULL);
    case  1: return unishox2_decompress_lines(in, inlen, out, USX_PSET_ALPHA_ONLY,             NULL);
    case  2: return unishox2_decompress_lines(in, inlen, out, USX_PSET_ALPHA_NUM_ONLY,         NULL);
    case  3: return unishox2_decompress_lines(in, inlen, out, USX_PSET_ALPHA_NUM_SYM_ONLY,     NULL);
    case  4: return unishox2_decompress_lines(in, inlen, out, USX_PSET_ALPHA_NUM_SYM_ONLY_TXT, NULL);
    case  5: return unishox2_decompress_lines(in, inlen, out, USX_PSET_FAVOR_ALPHA,            NULL);
    case  6: return unishox2_decompress_lines(in, inlen, out, USX_PSET_FAVOR_DICT,             NULL);
    case  7: return unishox2_decompress_lines(in, inlen, out, USX_PSET_FAVOR_SYM,              NULL);
    case  8: return unishox2_decompress_lines(in, inlen, out, USX_PSET_FAVOR_UMLAUT,           NULL);
    case  9: return unishox2_decompress_lines(in, inlen, out, USX_PSET_NO_DICT,                NULL);
    case 10: return unishox2_decompress_lines(in, inlen, out, USX_PSET_NO_UNI,                 NULL);
    case 11: return unishox2_decompress_lines(in, inlen, out, USX_PSET_NO_UNI_FAVOR_TEXT,      NULL);
    case 12: return unishox2_decompress_lines(in, inlen, out, USX_PSET_URL,                    NULL);
    case 13: return unishox2_decompress_lines(in, inlen, out, USX_PSET_JSON,                   NULL);
    case 14: return unishox2_decompress_lines(in, inlen, out, USX_PSET_JSON_NO_UNI,            NULL);
    case 15: return unishox2_decompress_lines(in, inlen, out, USX_PSET_XML,                    NULL);
    case 16: return unishox2_decompress_lines(in, inlen, out, USX_PSET_HTML,                   NULL);
  }  
}
