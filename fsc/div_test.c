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
// Test for fast division
//

#include "./fsc_utils.h"

#define CHECK_DIV(x, p, div) do {                                        \
  const ANSStateW q0 = (p > 0) ? (ANSStateW)(x) / (p) : (x);             \
  const ANSStateW q1 = FSCDivide((ANSStateW)(x), (div));                 \
  if (q0 != q1) {                                                        \
    printf("Error! 0x%.16llx / 0x%.16llx = 0x%.16llx != 0x%.16llx  ",    \
      (uint64_t)x, (uint64_t)p, (uint64_t)q0, (uint64_t)q1);             \
  }                                                                      \
} while (0)

#define MAX_N 300   // max inside sample points
#define DO_TEST(NAME)                                          \
static void NAME() {                                           \
  const uint64_t half = 1ull << (WBITS + PROBA_BITS - 1);      \
  const ANSStateW max_x = half + (half - 1); /* no overflow!*/ \
  const uint64_t max_p = 1ull << PROBA_BITS;                   \
  uint64_t p;                                                  \
  int n;                                                       \
  inv_t div;                                                   \
  for (p = 0; p < max_p; ++p) {                                \
    FSCInitDivide((ANSProba)p, &div);                          \
    CHECK_DIV(0, p, div);                                      \
    CHECK_DIV(max_x / 3, p, div);                              \
    CHECK_DIV(max_x / 2, p, div);                              \
    CHECK_DIV(max_x, p, div);                                  \
    for (n = 0; n < MAX_N; ++n) {                              \
      const double scale = (n + .5) / MAX_N;                   \
      const ANSStateW x = (ANSStateW)(scale * max_x);          \
      CHECK_DIV(x, p, div);                                    \
    }                                                          \
    if (!(p & 512)) printf("%.3lf     \r", 100. * p / max_p);  \
  }                                                            \
  printf("\nDone testing " #NAME " (max_p=%llu)!\n", max_p);   \
}

///////// Test 16bit case //////////

#define PROBA_BITS      16  // main param: precision for probabilities
#define RECIPROCAL_BITS 16  // controls the method used in divide.h
#define WBITS           16  // word size for I/O    
#define ANSProba uint16_t   // word for storing PROBA_BITS
#define ANSStateW uint32_t  // word for storing WBITS + PROBA_BITS state
#define inv_t inv16_t       // names aliasing ...
#define FSCInitDivide FSCInitDivide16
#define FSCDivide FSCDivide16
#include "./divide.h"
DO_TEST(DoTest16)
#undef PROBA_BITS
#undef RECIPROCAL_BITS
#undef WBITS
#undef ANSProba
#undef ANSStateW
#undef inv_t
#undef FSCInitDivide
#undef FSCDivide

///////// Test 32bit case //////////

#define RECIPROCAL_BITS 32
#define WBITS      32
#define PROBA_BITS 20    // works up to 32bit, but is quite long to test.
#define ANSProba uint64_t
#define ANSStateW uint64_t
#define inv_t inv32_t
#define FSCInitDivide FSCInitDivide32
#define FSCDivide FSCDivide32
#include "./divide.h"
DO_TEST(DoTest32)
#undef PROBA_BITS
#undef RECIPROCAL_BITS
#undef WBITS
#undef ANSProba
#undef ANSStateW
#undef inv_t
#undef FSCInitDivide
#undef FSCDivide


int main() {
  DoTest16();
  DoTest32();
  return 0;
}
