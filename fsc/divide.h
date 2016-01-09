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
// Everything needed to implement divide-by-multiply
//
// You need to define RECIPROCAL_BITS before including this header, with value
// being either 32, 16 or 0 (=use float)
// This header will define the following:
// *  inv_t type (to store the data needed for the reciprocal calculation)
// * void FSCInitDivide(ANSProba p, inv_t* div):
//       to be called to initialize the data to compute 1/p
// * ANSStateW FSCDivide(ANSStateW x, inv_t div)
//       to perform x/p   (corner case: returns 'x' for p==0).
//
// A bit of theory from this paper (mentioned by Ryg):
//   Alverson 1991: "Integer Division Using Reciprocals"
//   http://citeseerx.ist.psu.edu/viewdoc/summary?doi=10.1.1.33.1710
//
// If x and p are unsigned words represented by FIX bits (say: FIX=32 e.g.),
// then you can *exactly* compute x/p (floor-rounded) using:
//    k = ceil(log2(p))    <- number of bits needed to represent 'p'
//    s = FIX + k
//    a = ceil((1 << s) / y)  = ((1 << s) - 1) / p + 1
//    x/p = (a * x) >> s = ((a * x) >> FIX) >> k
// So, one need to store 'a' using FIX + 1 bits, and to be able to perform
// the multiply 'a * x' with (FIX + 1) x FIX bits operands.
// Fortunately we can decompose the multiply in two steps:
//  b = (a * x) >> FIX (usually easier with specialized instruction),
// followed by 'b >> k'.
// Also, 'a' always have a leading '1' at bit-position FIX. We don't need to
// store it and just store a - (1 << FIX), which fits in 'FIX' bits exactly.
// Then the final multiply can be rewritten as:
//   b = (a * x) >> FIX   // <- FIX*FIX multiply, keeping the upper FIX bits
//   x/p = (b + x) >> k
// By using this trick, one can use the full word range for probabilities 'p'.
// Note that the last 'b+x' addition must take care of potential overflow by
// one bit. Beware!

#include "./fsc_utils.h"

#ifndef HAVE_CEIL_LOG2
#define HAVE_CEIL_LOG2
static inline int CeilLog2(uint64_t p) {
  int s = 0;
  while (p > (1ull << s)) ++s;
  return s;
}
#endif // HAVE_CEIL_LOG2

#if (RECIPROCAL_BITS == 32)

typedef struct {
  uint64_t mult_;
  int shift_;
} inv_t;

#define DIV_FIX 64
static inline void FSCInitDivide(ANSProba p, inv_t* div) {
  if (p > 0) {
    const int s = CeilLog2(p);
    assert(s <= PROBA_BITS);
    const uint64_t base = 1ull << (s + DIV_FIX - 32);
    const uint64_t hi = (base / p) << 32;     // remove leading '1' bit
    const uint64_t lo = (((base % p) << 32) + p - 1) / p;
    div->mult_ = hi | lo;
    div->shift_ = s;
  } else {
    div->mult_ = 0;
    div->shift_ = 0;
  }
}

static inline ANSStateW FSCDivide(ANSStateW x, inv_t div) {
  const ANSStateW tmp = ((unsigned __int128)x * div.mult_) >> DIV_FIX;
  return ((unsigned __int128)tmp + x) >> div.shift_;
}

#undef DIV_FIX

#elif (RECIPROCAL_BITS == 16)

typedef struct {
  uint32_t mult_;
  int shift_;
} inv_t;

#define DIV_FIX 32
static inline void FSCInitDivide(ANSProba p, inv_t* div) {
  if (p > 0) {
    const int s = CeilLog2(p);
    assert(s <= PROBA_BITS);
    const uint64_t base = (1ull << (DIV_FIX + s)) - 1;
    div->mult_ = (base / p + 1) & 0xfffffffful;  // remove leading '1' bit
    div->shift_ = s;
  } else {
    div->mult_ = 0;
    div->shift_ = 0;
  }
}

static inline ANSStateW FSCDivide(ANSStateW x, inv_t div) {
  const uint32_t tmp = ((uint64_t)x * div.mult_) >> DIV_FIX;
  return ((uint64_t)tmp + x) >> div.shift_;  // (should be add-shift-with-carry-overflow)
}

#undef DIV_FIX

#elif (RECIPROCAL_BITS == 0)

typedef ANSProba inv_t;

static inline void FSCInitDivide(ANSProba p, inv_t* div) { *div = p; }

static inline ANSStateW FSCDivide(ANSStateW x, inv_t div) {
  return x / div;
}

#endif
