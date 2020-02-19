// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                                                                           -
//                       ****************************                        -
//                        ARITHMETIC CODING EXAMPLES                         -
//                       ****************************                        -
//                                                                           -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                                                                           -
// Fast arithmetic coding implementation                                     -
// -> 32-bit variables, 32-bit product, periodic updates, table decoding     -
//                                                                           -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                                                                           -
// Version 1.00  -  April 25, 2004                                           -
//                                                                           -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                                                                           -
//                                  WARNING                                  -
//                                 =========                                 -
//                                                                           -
// The only purpose of this program is to demonstrate the basic principles   -
// of arithmetic coding. It is provided as is, without any express or        -
// implied warranty, without even the warranty of fitness for any particular -
// purpose, or that the implementations are correct.                         -
//                                                                           -
// Permission to copy and redistribute this code is hereby granted, provided -
// that this warning and copyright notices are not removed or altered.       -
//                                                                           -
// Copyright (c) 2004 by Amir Said (said@ieee.org) &                         -
//                       William A. Pearlman (pearlw@ecse.rpi.edu)           -
//                                                                           -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                                                                           -
// A description of the arithmetic coding method used here is available in   -
//                                                                           -
// Lossless Compression Handbook, ed. K. Sayood                              -
// Chapter 5: Arithmetic Coding (A. Said), pp. 101-152, Academic Press, 2003 -
//                                                                           -
// A. Said, Introduction to Arithetic Coding Theory and Practice             -
// HP Labs report HPL-2004-76  -  http://www.hpl.hp.com/techreports/         -
//                                                                           -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


// - - Definitions - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#ifndef ARITHMETIC_CODEC
#define ARITHMETIC_CODEC

#include <stdio.h>


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - Class definitions - - - - - - - - - - - - - - - - - - - - - - - - - - -

class Static_Bit_Model                         // static model for binary data
{
public:

  Static_Bit_Model(void);

  void set_probability_0(double);             // set probability of symbol '0'

private:  //  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .
  unsigned bit_0_prob;
  friend class Arithmetic_Codec;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

class Static_Data_Model                       // static model for general data
{
public:

  Static_Data_Model(void);
 ~Static_Data_Model(void);

  unsigned model_symbols(void) { return data_symbols; }

  void set_distribution(unsigned number_of_symbols,
                        const double probability[] = 0);    // 0 means uniform

private:  //  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .
  unsigned * distribution, * decoder_table;
  unsigned data_symbols, last_symbol, table_size, table_shift;
  friend class Arithmetic_Codec;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

class Adaptive_Bit_Model                     // adaptive model for binary data
{
public:

  Adaptive_Bit_Model(void);         

  void reset(void);                             // reset to equiprobable model

private:  //  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .
  void     update(void);
  unsigned update_cycle, bits_until_update;
  unsigned bit_0_prob, bit_0_count, bit_count;
  friend class Arithmetic_Codec;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

class Adaptive_Data_Model                    // adaptive model for binary data
{
public:

  Adaptive_Data_Model(void);
  Adaptive_Data_Model(unsigned number_of_symbols);
 ~Adaptive_Data_Model(void);

  unsigned model_symbols(void) { return data_symbols; }

  void reset(void);                             // reset to equiprobable model
  void set_alphabet(unsigned number_of_symbols);

private:  //  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .
  void     update(bool);
  unsigned * distribution, * symbol_count, * decoder_table;
  unsigned total_count, update_cycle, symbols_until_update;
  unsigned data_symbols, last_symbol, table_size, table_shift;
  friend class Arithmetic_Codec;
};


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - Encoder and decoder class - - - - - - - - - - - - - - - - - - - - - - -

// Class with both the arithmetic encoder and decoder.  All compressed data is
// saved to a memory buffer

class Arithmetic_Codec
{
public:

  Arithmetic_Codec(void);
 ~Arithmetic_Codec(void);
  Arithmetic_Codec(unsigned max_code_bytes,
                   unsigned char * user_buffer = 0);         // 0 = assign new

  unsigned char * buffer(void) { return code_buffer; }

  void set_buffer(unsigned max_code_bytes,
                  unsigned char * user_buffer = 0);          // 0 = assign new

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

  void     encode(unsigned bit,
                  Static_Bit_Model &);
  unsigned decode(Static_Bit_Model &);

  void     encode(unsigned data,
                  Static_Data_Model &);
  unsigned decode(Static_Data_Model &);

  void     encode(unsigned bit,
                  Adaptive_Bit_Model &);
  unsigned decode(Adaptive_Bit_Model &);

  void     encode(unsigned data,
                  Adaptive_Data_Model &);
  unsigned decode(Adaptive_Data_Model &);

private:  //  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .
  void propagate_carry(void);
  void renorm_enc_interval(void);
  void renorm_dec_interval(void);
  unsigned char * code_buffer, * new_buffer, * ac_pointer;
  unsigned base, value, length;                     // arithmetic coding state
  unsigned buffer_size, mode;     // mode: 0 = undef, 1 = encoder, 2 = decoder
};

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#endif

