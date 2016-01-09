// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                                                                           -
//                       *************************                           -
//                        HUFFMAN CODING EXAMPLES                            -
//                       *************************                           -
//                                                                           -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                                                                           -
//   Implementation of periodic-adaptive and static Huffman codes            -
//                                                                           -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                                                                           -
// Version 1.00  -  January 24, 2005                                         -
//                                                                           -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                                                                           -
//                                  WARNING                                  -
//                                 =========                                 -
//                                                                           -
// The only purpose of this program is to demonstrate the basic principles   -
// of Huffman codes. It is provided as is, without any express or implied    -
// warranty, without even the warranty of fitness for any particular         -
// purpose, or that the implementations are correct.                         -
//                                                                           -
// Permission to copy and redistribute this code is hereby granted, provided -
// that this warning and copyright notices are not removed or altered.       -
//                                                                           -
// Copyright (c) 2005 by Amir Said (said@ieee.org) &                         -
//                       William A. Pearlman (pearlw@ecse.rpi.edu)           -
//                                                                           -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


// - - Definitions - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#ifndef BINARY_H_CODEC
#define BINARY_H_CODEC

#include <stdio.h>


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - Class definitions - - - - - - - - - - - - - - - - - - - - - - - - - - -

class Static_Huffman_Code
{
public:

  Static_Huffman_Code(void);
 ~Static_Huffman_Code(void);

  unsigned code_symbols(void) { return data_symbols; }

  void set_distribution(unsigned number_of_symbols,
                        const double probability[] = 0);    // 0 means uniform

private:  //  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .
  int * code_data, * decd_table, * tree[2];
  unsigned * length, * codeword, data_symbols, table_shift, table_bits;
  friend class Binary_Codec;
};


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

class Adaptive_Huffman_Code
{
public:

  Adaptive_Huffman_Code(void);
  Adaptive_Huffman_Code(unsigned number_of_symbols);
 ~Adaptive_Huffman_Code(void);

  unsigned code_symbols(void) { return data_symbols; }

  void reset(void);                             // reset to equiprobable model
  void set_alphabet(unsigned number_of_symbols);

private:  //  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .
  void update(bool);
  int * code_data, * decd_table, * tree[2];
  unsigned total_count, data_symbols, update_cycle, symbols_until_update;
  unsigned * length, * codeword, * symbol_count, table_shift, table_bits;
  friend class Binary_Codec;
};


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - Huffman and binary encoder and decoder  - - - - - - - - - - - - - - - -

// Class with both the Huffman/binary encoder and decoder. All compressed data
// is saved to a memory buffer

class Binary_Codec
{
public:

  Binary_Codec(void);
 ~Binary_Codec(void);
  Binary_Codec(unsigned max_code_bytes,
               unsigned char * user_buffer = 0);             // 0 = assign new

  unsigned char * buffer(void) { return code_buffer; }

  void     set_buffer(unsigned max_code_bytes,
                      unsigned char * user_buffer = 0);      // 0 = assign new

  void     start_encoder(void);
  void     start_decoder(void);
  void     read_from_file(FILE * code_file);  // read code data, start decoder

  unsigned stop_encoder(void);                 // returns number of bytes used
  unsigned write_to_file(FILE * code_file);   // stop encoder, write code data
  void     stop_decoder(void);

  void     put_bit(unsigned bit);
  unsigned get_bit(void);

  void     put_bits(unsigned data, unsigned number_of_bits);
  unsigned get_bits(unsigned number_of_bits);

  void     encode(unsigned data,
                  Static_Huffman_Code &);
  unsigned decode(Static_Huffman_Code &);

  void     encode(unsigned data,
                  Adaptive_Huffman_Code &);
  unsigned decode(Adaptive_Huffman_Code &);

private:  //  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .
  unsigned char * code_buffer, * new_buffer, * bc_pointer;
  unsigned bit_buffer, bit_position;                     // binary codec state
  unsigned buffer_size, mode;     // mode: 0 = undef, 1 = encoder, 2 = decoder
};

// = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =

#endif
