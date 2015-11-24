// Copyright 2011 Google Inc. All Rights Reserved.
// Authors: Rasto Lenhardt and Jyrki Alakuijala
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef GIPFELI_FIND_MATCH_LENGTH_H_
#define GIPFELI_FIND_MATCH_LENGTH_H_

#include <stdlib.h>
#include "integral_types.h"

namespace gipfeli {

// Separate implementation for x86_64, for speed.
#if defined(__GNUC__) && defined(ARCH_K8)
static inline int FindMatchLength(const uint8* s1,
                                  const uint8* s2,
                                  const uint8* s2_limit) {
  int matched = 0;
  // Find out how long the match is. We loop over the data 64 bits at a
  // time until we find a 64-bit block that doesn't match; then we find
  // the first non-matching bit and use that to calculate the total
  // length of the match.
  while (PREDICT_TRUE(s2 <= s2_limit - 8)) {
    if (PREDICT_FALSE(UNALIGNED_LOAD64(s2) == UNALIGNED_LOAD64(s1 + matched))) {
      s2 += 8;
      matched += 8;
    } else {
      // On current (mid-2008) Opteron models there is a 3% more
      // efficient code sequence to find the first non-matching byte.
      // However, what follows is ~10% better on Intel Core 2 and newer,
      // and we expect AMD's bsf instruction to improve.
      uint64 x = UNALIGNED_LOAD64(s2) ^ UNALIGNED_LOAD64(s1 + matched);
      int matching_bits = Bits::FindLSBSetNonZero64(x);
      matched += matching_bits >> 3;
      return matched;
    }
  }
  while (PREDICT_TRUE(s2 < s2_limit)) {
    if (PREDICT_TRUE(s1[matched] == *s2)) {
      ++s2;
      ++matched;
    } else {
      return matched;
    }
  }
  return matched;
}
#else
static inline int FindMatchLength(const uint8* s1,
                                  const uint8* s2,
                                  const uint8* s2_limit) {
  int matched = 0;
  const uint8 *s2_ptr = s2;
  // Find out how long the match is. We loop over the data 32 bits at a
  // time until we find a 32-bit block that doesn't match; then we find
  // the first non-matching bit and use that to calculate the total
  // length of the match.
  while (s2_ptr <= s2_limit - 4 &&
         UNALIGNED_LOAD32(s2_ptr) == UNALIGNED_LOAD32(s1 + matched)) {
    s2_ptr += 4;
    matched += 4;
  }
  while ((s2_ptr < s2_limit) && (s1[matched] == *s2_ptr)) {
    ++s2_ptr;
    ++matched;
  }
  return matched;
}

static inline int FindMatchLengthWithLimit(const uint8* s1,
                                           const uint8* s2,
                                           size_t limit) {
  return FindMatchLength(s1, s2, s2 + limit);
}

#endif

}  // namespace gipfeli

#endif  // GIPFELI_FIND_MATCH_LENGTH_H_
