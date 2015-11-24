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

#include "gipfeli.h"

#include <stdlib.h>
#include <string.h>
#include <string>

#include "integral_types.h"
#include "enum.h"
#include "read_bits.h"

namespace gipfeli {

// Copy "len" bytes from "src" to "op", one byte at a time.  Used for
// handling COPY operations where the input and output regions may
// overlap.  For example, suppose:
//    src    == "ab"
//    op     == src + 2
//    len    == 20
// After IncrementalCopy(src, op, len), the result will have
// eleven copies of "ab"
//    ababababababababababab
// Note that this does not match the semantics of either memcpy()
// or memmove().
static inline void IncrementalCopy(const char* src, char* op, int len) {
  do {
    *op++ = *src++;
  } while (--len > 0);
}

// Equivalent to IncrementalCopy except that it can write up to ten extra
// bytes after the end of the copy, and that it is faster.
//
// The main part of this loop is a simple copy of eight bytes at a time until
// we've copied (at least) the requested amount of bytes.  However, if op and
// src are less than eight bytes apart (indicating a repeating pattern of
// length < 8), we first need to expand the pattern in order to get the correct
// results. For instance, if the buffer looks like this, with the eight-byte
// <src> and <op> patterns marked as intervals:
//
//    abxxxxxxxxxxxx
//    [------]           src
//      [------]         op
//
// a single eight-byte copy from <src> to <op> will repeat the pattern once,
// after which we can move <op> two bytes without moving <src>:
//
//    ababxxxxxxxxxx
//    [------]           src
//        [------]       op
//
// and repeat the exercise until the two no longer overlap.
//
// This allows us to do very well in the special case of one single byte
// repeated many times, without taking a big hit for more general cases.
//
// The worst case of extra writing past the end of the match occurs when
// op - src == 1 and len == 1; the last copy will read from byte positions
// [0..7] and write to [4..11], whereas it was only supposed to write to
// position 1. Thus, ten excess bytes.

namespace {

const int kMaxIncrementCopyOverflow = 10;

inline void IncrementalCopyFastPath(const char* src, char* op, int len) {
  while (op - src < 8) {
    UNALIGNED_STORE64(op, UNALIGNED_LOAD64(src));
    len -= op - src;
    op += op - src;
  }
  while (len > 0) {
    UNALIGNED_STORE64(op, UNALIGNED_LOAD64(src));
    src += 8;
    op += 8;
    len -= 8;
  }
}

}  // namespace

// This method reads bitmask containing the infromation about which
// entropy code is used. It builds the conversion tables "convert_6bit"
// and "convert_8bit", which assign to each compressed symbol of length 6 or 8
// bits, the original symbol. They are used later to convert the literals.
// REQUIRES: convert_6bit to have size 32
// REQUIRES: convert_8bit to have size 64
char* DecompressMask(const uint32 upper,
                     char* input,
                     uint8* convert_6bit,
                     uint8* convert_8bit) {
  uint8* ip = reinterpret_cast<uint8*>(input);

  // First phase: read 96 symbols that will have 6 or 8-bit long codes.
  uint8 to_be_converted[96];
  int count = 0;

  uint32 and_value = 1U << 31;
  for (int i = 0; i < 32; i++) {
    if ((and_value & upper) != 0) {
      for (int j = 0; j < 8; j++) {
        if (((*ip) & (1 << (7 - j))) != 0) {
          to_be_converted[count] = 8 * i + j;
          count++;
        }
      }
      ip++;
    }
    and_value >>= 1;
  }

  // Second phase: Split 96 symbols to ones with 6-bit and ones with 8-bit
  // long codes and assign the symbol to every code.
  int count_6bit = 0;
  int count_8bit = 0;
  for (int i = 0; i < (count + 7) / 8; i++) {
    for (int j = 0; j < 8; j++) if (8 * i + j < count) {
      if (((*ip) & (1 << (7 - j))) != 0) {
        convert_6bit[count_6bit] = to_be_converted[8 * i + j];
        count_6bit++;
      } else {
        convert_8bit[count_8bit] = to_be_converted[8 * i + j];
        count_8bit++;
      }
    }
    ip++;
  }
  return reinterpret_cast<char*>(ip);
}

static const uint16 length_length[] = {0, 0, 2, 2, 2, 3, 3, 6};
static const uint16 offset_length[] = {0, 0, 10, 13, 16, 10, 16, 16};
static const uint16 length_change[] = {0, 0, 4, 4, 4, 8, 8, 4};

char* DecompressCommands(char* input,
                         const char* input_end,
                         uint32 commands_real_size,
                         uint32* commands) {
  ReadBits bits;
  char* ip = input;
  bits.Start(ip, input_end);
  uint32 commands_size = 0;
  while (commands_size < commands_real_size) {
    uint32 value = bits.ReadNBits<3>();
    if (value < 2) {
      value = (value << 5) + bits.ReadNBits<5>();
      if (value < 53) {
        commands[commands_size++] = value + 1;
      } else {
        commands[commands_size++] = bits.Read(value - 47) + 1;
      }
    } else {
      int length = bits.Read(length_length[value]) + length_change[value];
      int offset = bits.Read(offset_length[value]) + 1;
      commands[commands_size++] = CommandCopy(length, offset);
    }
  }
  return bits.Stop();
}

class Uncompressor::Impl {
 public:
  Impl() : commands_(NULL), commands_size_(0) {}

  ~Impl() {
    delete[] commands_;
  }

  uint32* ReallocateCommands(int size) {
    if (commands_ == NULL || commands_size_ < size) {
      delete[] commands_;
      commands_ = new uint32[size];
      commands_size_ = size;
    }
    return commands_;
  }

 private:
  uint32* commands_;
  int commands_size_;
};

Status Uncompressor::Init() {
  delete impl_;
  impl_ = new Impl();
  return kOk;
}

Uncompressor::~Uncompressor() {
  delete impl_;
}

// Returns the size of the uncompressed string.
Status Uncompressor::GetUncompressedLength(const char* compressed,
                                           size_t compressed_length,
                                           size_t* uncompressed_length) {
  int bytes_used = *compressed;
  if (bytes_used > 4 || compressed_length < 1 + bytes_used) {
    return kInputFail;
  }
  *uncompressed_length = 0;
  for (int i = bytes_used - 1; i >= 0; i--) {
    *uncompressed_length <<= 8;
    *uncompressed_length |= compressed[i + 1];
  }
  if (*uncompressed_length > 1ULL << 31) {
    // Uncompressed string can have at most 2 GB.
    return kCorrupted;
  }
  return kOk;
}

// Uncompresses the "compressed" data to string "uncompressed". Returns error
// status if the input is corrupted, otherwise returns kOk.
Status Uncompressor::Uncompress(const char* compressed,
                                size_t compressed_length,
                                /*std::string*/ char *uncompressed, int uncompressed_size) {
  if (impl_ == NULL) return kNotInitialised;
  char* ip = const_cast<char*>(compressed);
  char* ip_end = ip + compressed_length;
  if (compressed_length < 1) {
    return kInputFail; 
  }
  int bytes_used = *ip;
  size_t uncompressed_length = 0;
  //Status status = GetUncompressedLength(compressed, compressed_length, &uncompressed_length);
  //if (status != kOk) { return status;  }
  ip += 1 + bytes_used;
  //uncompressed->resize(uncompressed_length);

  char* op = uncompressed; //uncompressed->empty() ? NULL : &(*uncompressed)[0];
  char* op_start = op;
  char* op_end = op + uncompressed_size;
  while (ip - compressed < compressed_length) {
    if (PREDICT_FALSE(ip_end - ip < 10)) {
      // 2 bytes for commands size and at least 8 bytes for the first commands.
      return kInputFail;
    }
    uint32 commands_size = UNALIGNED_LOAD16(ip);
    ip += 2;

    uint32* commands = impl_->ReallocateCommands(commands_size);
    ip = DecompressCommands(ip, ip_end, commands_size, commands);
    if (PREDICT_FALSE(ip > ip_end)) {
      return kInputFail;
    }
    uint32 upper = 0;
    for (int i = 0; i < 4; i++) {
      uint8 val = (uint8)(*ip);
      ip++;
      upper <<= 8;
      upper += val;
    }
    uint8 convert_6bit[32] = { 0 };
    uint8 convert_8bit[64] = { 0 };
    if (upper == 0) {
      // No Entropy Coding.
      for (int i = 0; i < commands_size; i++) {
        if (!CommandIsCopy(commands[i])) {
          if (PREDICT_FALSE(op + commands[i] > op_end) ||
              ip + commands[i] > ip_end ) {
            if (op + commands[i] > op_end) {
              return kOutputFail;
            } else {
              return kInputFail;
            }
          }
          memcpy(op, ip, commands[i]);
          ip += commands[i];
          op += commands[i];
        } else {
          int len = CommandCopyLength(commands[i]);
          int offset = CommandCopyOffset(commands[i]);
          int space_left = op_end - op;
          if (PREDICT_FALSE(op < op_start + offset)) {
            return kCorrupted;
          }
          if (len <= 16 && offset >= 8 && space_left >= 16) {
            // Fast path, used for the majority (70-80%) of dynamic invocations.
            UNALIGNED_STORE64(op, UNALIGNED_LOAD64(op - offset));
            UNALIGNED_STORE64(op + 8, UNALIGNED_LOAD64(op - offset + 8));
          } else if (space_left >= len + kMaxIncrementCopyOverflow) {
            IncrementalCopyFastPath(op - offset, op, len);
          } else {
            if (PREDICT_FALSE(space_left < len)) {
              return kOutputFail;
            }
            IncrementalCopy(op - offset, op, len);
          }
          op += len;
        }
      }
    } else {
      // Entropy Coding.
      if (ip + Bits::CountOnes(upper) + 12 > ip_end) {
        return kInputFail;
      }
      ip = DecompressMask(upper, ip, convert_6bit, convert_8bit);
      ReadBits bits;
      bits.Start(ip, ip_end);
      if (PREDICT_FALSE(ip_end - ip < 8)) {
        return kInputFail;
      }
      for (int i = 0; i < commands_size; i++) {
        if (!CommandIsCopy(commands[i])) {
          if (PREDICT_FALSE(op + commands[i] > op_end)) {
            return kOutputFail;
          }
          for (int j = 0; j < commands[i]; j++) {
            uint32 val = bits.ReadNBits<6>();
            if (PREDICT_TRUE(val < 32)) {
              *op++ = convert_6bit[val];
            } else if (val >= 48) {
              val = ((val - 48) << 4) + bits.ReadNBits<4>();
              *op++ = val;
            } else {
              val = ((val - 32) << 2) + bits.ReadNBits<2>();
              *op++ = convert_8bit[val];
            }
          }
        } else {
          int len = CommandCopyLength(commands[i]);
          int offset = CommandCopyOffset(commands[i]);
          int space_left = op_end - op;
          if (PREDICT_FALSE(op < op_start + offset)) {
            return kCorrupted;
          }
          if (len <= 16 && offset >= 8 && space_left >= 16) {
            // Fast path, used for the majority (70-80%) of dynamic invocations.
            UNALIGNED_STORE64(op, UNALIGNED_LOAD64(op - offset));
            UNALIGNED_STORE64(op + 8, UNALIGNED_LOAD64(op - offset + 8));
          } else if (space_left >= len + kMaxIncrementCopyOverflow) {
            IncrementalCopyFastPath(op - offset, op, len);
          } else {
            if (PREDICT_FALSE(space_left < len)) {
              return kOutputFail;
            }
            IncrementalCopy(op - offset, op, len);
          }
          op += len;
        }
      }
      ip = bits.Stop();
      if (PREDICT_FALSE(ip > ip_end)) {
        return kCorrupted;
      }
    }
  }
  if (op != op_end) {
    return kCorrupted;
  }
  return kOk;
}

}  // namespace gipfeli
