//Copyright 2014 The FSC Authors. All Rights Reserved.
//
//Licensed under the Apache License, Version 2.0 (the "License");
//you may not use this file except in compliance with the License.
//You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and
//    limitations under the License.
//------------------------------------------------------------------------------
//
// Finite State Coder. Main header.
//
//  based on Jarek Duda's paper: http://arxiv.org/pdf/1311.2540v1.pdf
//
// Author: Skal (pascal.massimino@gmail.com)

#ifndef FSC_FSC_H_
#define FSC_FSC_H_

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// Coder parameters
#define BLOCK_SIZE  8192   // sliding window size (must be >= 256)
#define MAX_SYMBOLS 256    // byte-based
#define LOG_TAB_SIZE      14    // max internal precision (must be <= 14)
#define MAX_LOG_TAB_SIZE  16    // max precision for word-based coding
#define CRYPTO_KEY  0
// disabled for now (so we investigate core algo):
// #define CRYPTO_KEY 0x3fdc

// Compression methods
typedef enum {
  CODING_METHOD_BUCKET = 0,
  CODING_METHOD_REVERSE,
  CODING_METHOD_MODULO,
  CODING_METHOD_PACK,

  CODING_METHOD_16B,
  CODING_METHOD_16B_2X,
  CODING_METHOD_16B_ALIAS,
  CODING_METHOD_16B_ALIAS_2X,

  CODING_METHOD_16B_4X,   // default

  CODING_METHOD_UNIQUE,   // internal, do not use directly

  CODING_METHOD_LAST,
  CODING_METHOD_DEFAULT = CODING_METHOD_16B_4X
} FSCCodingMethod;

typedef uint32_t FSCStateW;
typedef uint16_t FSCType;  // storage type
#define FSC_BITS      16
#define FSC_MAX       ((FSCStateW)1 << FSC_BITS)
#define FSC_BITS_MASK (((FSCStateW)1 << FSC_BITS) - 1)

// derived params
#define TAB_SIZE (1U << LOG_TAB_SIZE)
#define TAB_MASK (TAB_SIZE - 1)
#define MAX_TAB_SIZE (1 << MAX_LOG_TAB_SIZE)

// Header parameter
#define TAB_HDR_BITS 6
#define HDR_SYMBOL_LIMIT 20

//------------------------------------------------------------------------------
// Decoding

// Return 0 upon error.
// Result is in *out, must deallocated using free()
int FSCDecode(const uint8_t* in, size_t in_size, uint8_t** out, size_t* out_size);

// non-canned API:
typedef struct FSCDecoder FSCDecoder;
FSCDecoder* FSCInit(const uint8_t* input, size_t len);
int FSCIsOk(FSCDecoder* dec);
int FSCDecompress(FSCDecoder* dec, uint8_t** out, size_t* size);
void FSCDelete(FSCDecoder* dec);

//------------------------------------------------------------------------------
// Encoding

// Return 0 upon error.
// Result is in *out, must deallocated using free()
int FSCEncode(const uint8_t* in, size_t in_size,
              uint8_t** out, size_t* out_size,
              int log_tab_size, FSCCodingMethod method);

// utils
void FSCCountSymbols(const uint8_t* in, size_t in_size,
                     uint32_t counts[MAX_SYMBOLS]);
int FSCNormalizeCounts(uint32_t counts[MAX_SYMBOLS], int max_symbol,
                       int log_tab_size);

//
//------------------------------------------------------------------------------
// mapping function (common to enc/dec)

// returns 0 in case of error
typedef int (*FSCBuildSpreadTableFunc)(int max_symbol, const uint32_t counts[],
                                       int log_tab_size, uint8_t symbols[]);

extern int BuildSpreadTableBucket(int max_symbol, const uint32_t counts[],
                                  int log_tab_size, uint8_t symbols[]);
extern int BuildSpreadTableReverse(int max_symbol, const uint32_t counts[],
                                   int log_tab_size, uint8_t symbols[]);
extern int BuildSpreadTableModulo(int max_symbol, const uint32_t counts[],
                                  int log_tab_size, uint8_t symbols[]);
extern int BuildSpreadTablePack(int max_symbol, const uint32_t counts[],
                                int log_tab_size, uint8_t symbols[]);

#endif  // FSC_FSC_H_
