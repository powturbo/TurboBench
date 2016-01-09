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
// Bit reader/writer
//
// Author: Skal (pascal.massimino@gmail.com)

#ifndef FSC_BITS_H_
#define FSC_BITS_H_

#include <assert.h>
#ifdef _MSC_VER
#define FSC_INLINE
#else
#define FSC_INLINE inline
#endif

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

// -----------------------------------------------------------------------------
// 32b/64b type'ing

#if (defined(__x86_64__) || defined(_M_X64))     // 64 bits

typedef uint64_t fsc_val_t;
#define RBYTES 4

#else                                            // 32 bits

typedef uint32_t fsc_val_t;
#define RBYTES 2

#endif

#define RBITS (RBYTES * 8)

// -----------------------------------------------------------------------------
// BitReader

typedef struct {
  fsc_val_t      bits_;       // bits accumulator
  const uint8_t* buf_;        // current position
  const uint8_t* end_;        // end of read position
  int            bit_pos_;    // unread bit position
  int            eof_;        // true if buf_ reached end_
} FSCBitReader;

void FSCInitBitReader(FSCBitReader* const br,
                      const uint8_t* const start,
                      size_t length);

uint32_t FSCReadBits(FSCBitReader* const br, int nb);
static FSC_INLINE uint32_t FSCSeeBits(FSCBitReader* const br) {
  return (uint32_t)(br->bits_ >> br->bit_pos_);
}

static FSC_INLINE void FSCDiscardBits(FSCBitReader* const br, int nb) {
  br->bit_pos_ += nb;
}

// Make sure the buffer contains at least RBYTES bytes,
// available to FSCSeeBits()
extern void FSCDoFillBitWindow(FSCBitReader* const br);
static FSC_INLINE void FSCFillBitWindow(FSCBitReader* const br) {
  if (br->bit_pos_ >= RBITS) FSCDoFillBitWindow(br);
}

const uint8_t* FSCBitAlign(FSCBitReader* const br);
extern const uint8_t* FSCGetBytePos(FSCBitReader* const br);
extern const uint8_t* FSCGetByteEnd(FSCBitReader* const br);
extern void FSCSetReadBufferPos(FSCBitReader* const br, const uint8_t* buf);

// -----------------------------------------------------------------------------
// BitWriter

typedef struct {
  fsc_val_t bits_;     // currently assembled bits
  int used_;           // bit position (<= RBITS)
  uint8_t* cur_;       // current write position
  uint8_t* buf_;       // start of writable buffer
  uint8_t* end_;       // non-writable pos
  int error_;          // true if malloc failed (or other error)
} FSCBitWriter;

// Returns 0 in case of malloc error
int FSCBitWriterInit(FSCBitWriter* const bw, size_t expected_size);

void FSCBitWriterFlush(FSCBitWriter* const bw);
static FSC_INLINE size_t FSCBitWriterNumBytes(FSCBitWriter* const bw) {
  return (uint8_t*)bw->cur_ - (uint8_t*)bw->buf_;
}
static FSC_INLINE uint8_t* FSCBitWriterFinish(FSCBitWriter* const bw) {
  return (uint8_t*)bw->buf_;
}
void FSCBitWriterDestroy(FSCBitWriter* const bw);
void FSCWriteBits(FSCBitWriter* const bw, uint32_t bits, int nb);

int FSCAppend(FSCBitWriter* const bw, const uint8_t* const buf, size_t len);

#ifdef __cplusplus
}    // extern "C"
#endif

#endif  /* FSC_BITS_H_ */
