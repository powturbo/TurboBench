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

#ifndef GIPFELI_INTEGRAL_TYPES_H_
#define GIPFELI_INTEGRAL_TYPES_H_

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;

#if defined(COMPILER_GCC3)
#define PREDICT_FALSE(x) (__builtin_expect(x, 0))
#define PREDICT_TRUE(x) (__builtin_expect(!!(x), 1))
#else
#define PREDICT_FALSE(x) x
#define PREDICT_TRUE(x) x
#endif

#define UNALIGNED_LOAD16(_p) (*reinterpret_cast<const uint16 *>(_p))
#define UNALIGNED_LOAD32(_p) (*reinterpret_cast<const uint32 *>(_p))
#define UNALIGNED_LOAD64(_p) (*reinterpret_cast<const uint64 *>(_p))

#define UNALIGNED_STORE16(_p, _val) (*reinterpret_cast<uint16 *>(_p) = (_val))
#define UNALIGNED_STORE32(_p, _val) (*reinterpret_cast<uint32 *>(_p) = (_val))
#define UNALIGNED_STORE64(_p, _val) (*reinterpret_cast<uint64 *>(_p) = (_val))

namespace Bits {

static int CountOnes(uint32 n) {
  n -= ((n >> 1) & 0x55555555);
  n = ((n >> 2) & 0x33333333) + (n & 0x33333333);
  return (((n + (n >> 4)) & 0xF0F0F0F) * 0x1010101) >> 24;
}

static int Log2Floor(uint32 n) {
  return n == 0 ? -1 : 31 ^ __builtin_clz(n);
}

static int Log2FloorNonZero(uint32 n) {
  return 31 ^ __builtin_clz(n);
}

}  // Bits

#endif  // GIPFELI_INTEGRAL_TYPES_H_
