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

#ifndef GIPFELI_INTERNAL_READ_BITS_H_
#define GIPFELI_INTERNAL_READ_BITS_H_

#include "integral_types.h"

namespace gipfeli {

// Reads bits from the "input" char stream and returns pointer to the stream
// after reading of the input finished. Sample usage:
// char* input;
// char* input_end
// ReadBits bits;
// bits.Start(input, input_end);
// x = bits.Read(3);
// ...
// y = bits.Read(7);
// input = bits.Stop();
// if (input > input_end) error has occurred
class ReadBits {
 public:
  void Start(char* input, const char* input_end) {
    bits_input_ = reinterpret_cast<uint64*>(input);
    bits_left_ = 64;
    current_ = bits_input_[0];
    input_end_ = input_end;
    error_ = false;
  }

  uint32 Read(int length) {
    uint32 ret = current_ >> (64 - length);
    if (PREDICT_TRUE(length < bits_left_)) {
      current_ <<= length;
      bits_left_ -= length;
    } else {
      ret = (current_ >> (64 - bits_left_)) << (length - bits_left_);
      bits_input_++;
      if (PREDICT_TRUE(length != bits_left_)) {
        if (PREDICT_FALSE(reinterpret_cast<char*>(bits_input_ + 1) >
                          input_end_)) {
          // It is not safe to read bits any more. To avoid crash, move back
          // bits_input_ pointer and remember that an error has occurred.
          error_ = true;
          bits_input_--;
          return 0;
        }
        current_ = *bits_input_;
        ret += current_ >> (64 - (length - bits_left_));
        current_ <<= length - bits_left_;
        bits_left_ = 64 - (length - bits_left_);
      } else {
        if (PREDICT_TRUE(reinterpret_cast<char*>(bits_input_ + 1) <=
                          input_end_)) {
          // We are at the end of 64 bit block, we read the next block to
          // current_ only if it is safe.
          current_ = *bits_input_;
          bits_left_ = 64;
        } else {
          // If it is not safe to read the next block, it means we are at the
          // end of the input and no more reads are valid. Therefore we set
          // bits_left_ to 0, which will cause an error if we try to read again.
          bits_left_ = 0;
        }
      }
    }
    return ret;
  }

  template<int kLength>
  uint32 ReadNBits() {
    uint32 ret = current_ >> (64 - kLength);
    if (PREDICT_TRUE(kLength < bits_left_)) {
      current_ <<= kLength;
      bits_left_ -= kLength;
    } else {
      ret = (current_ >> (64 - bits_left_)) << (kLength - bits_left_);
      bits_input_++;
      if (PREDICT_TRUE(kLength != bits_left_)) {
        if (PREDICT_FALSE(reinterpret_cast<char*>(bits_input_ + 1) >
                          input_end_)) {
          // It is not safe to read bits any more. To avoid crash, move back
          // bits_input_ pointer and remember that an error has occurred.
          error_ = true;
          bits_input_--;
          return 0;
        }
        current_ = *bits_input_;
        ret += current_ >> (64 - (kLength - bits_left_));
        current_ <<= kLength - bits_left_;
        bits_left_ = 64 - (kLength - bits_left_);
      } else {
        if (PREDICT_TRUE(reinterpret_cast<char*>(bits_input_ + 1) <=
                          input_end_)) {
          // We are at the end of 64 bit block, we read the next block to
          // current_ only if it is safe.
          current_ = *bits_input_;
          bits_left_ = 64;
        } else {
          // If it is not safe to read the next block, it means we are at the
          // end of the input and no more reads are valid. Therefore we set
          // bits_left_ to 0, which will cause an error if we try to read again.
          bits_left_ = 0;
        }
      }
    }
    return ret;
  }

  char* Stop() {
    if (error_) {
      // Pointer set to input_end + 1 signals that an error has occurred.
      return const_cast<char*>(input_end_ + 1);
    }
    if (bits_left_ < 64 && bits_left_ > 0) {
      bits_input_++;
    }
    return reinterpret_cast<char*>(bits_input_);
  }

 private:
  int bits_left_;
  uint64 current_;
  uint64* bits_input_;
  const char* input_end_;
  bool error_;
};

}  // namespace gipfeli

#endif  // GIPFELI_INTERNAL_READ_BITS_H_
