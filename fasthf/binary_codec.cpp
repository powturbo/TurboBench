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


// - - Inclusion - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#include <stdlib.h>
#include <string.h>
#include "binary_codec.h"


// - - Constants - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// #define TREE_DECODE

const unsigned HC__MaxCount = 1 << 15;  // Max. symbol count for adaptive code


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - Static functions  - - - - - - - - - - - - - - - - - - - - - - - - - - -

static void HC_Error(const char * msg)
{
  fprintf(stderr, "\n\n -> Huffman/binary coding error: ");
  fputs(msg, stderr);
  fputs("\n Execution terminated!\n", stderr);
  exit(1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

static void Form_Huffman_Tree(int number_of_symbols,
                              unsigned original_count[],
                              int left_branch[],
                              int right_branch[])
{
  int tc, ts, i, n, nc, it = 0, k = (number_of_symbols >> 1) + 1;
  int * count = right_branch - 1, * symbol = left_branch - 1;

  for (n = k - 1; n < number_of_symbols; n++) {     // second half of the heap
    symbol[n+1] = n;
    count[n+1]  = original_count[n];
  }

  for (;;) {
    if (k > 1) {
      ts = (--k) - 1;                                // add first half to heap
      tc = original_count[ts];
    }
    else
      if ((++it) & 1) {
        tc = count[n];
        ts = symbol[n];
        left_branch[--n] = symbol[1];                  // set-up merging nodes
        if (n == 1) break;
        nc = count[1];
      }
      else {
        right_branch[n] = symbol[1];                 // finalize merging nodes
        ts = -n;
        tc = nc + count[1];
      }
    for (int j = (i = k) << 1; j <= n; j += (i = j)) {            // heap sort
      if ((j < n) && (count[j] > count[j+1])) ++j;
      if (tc <= count[j]) break;
      symbol[i] = symbol[j];
      count[i]  = count[j];
    }
    symbol[i] = ts;
    count[i]  = tc;
  }
  right_branch[1] = ts;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

static void Set_Huffman_Code(int * tree[2],
                             unsigned codeword[],
                             unsigned codeword_length[])
{
  int stack[32], node, depth = 0, current_code = 0;

  for (stack[0] = 1; depth >= 0;) {       // transverse tree setting codewords

    if ((node = tree[current_code&1][stack[depth]]) < 0)
      do {
        current_code <<= 1;
        stack[++depth] = -node;
      } while ((node = tree[0][-node]) < 0);

    codeword[node] = current_code;              // set codeword to leaf symbol
    codeword_length[node] = depth + 1;

    while (current_code & 1) {                                    // backtrack
      current_code >>= 1;
      if (--depth < 0) break;
    }
    current_code |= 1;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

static void Set_Huffman_Code(int table_bits,
                             int * tree[2],
                             int decoder_table[],
                             unsigned codeword[],
                             unsigned codeword_length[])
{
  int stack[32], node, depth = 0, current_code = 0;

  for (stack[0] = 1; depth >= 0;) {       // transverse tree setting codewords

    if ((node = tree[current_code&1][stack[depth]]) < 0)
      do {
        if (depth + 1 == table_bits) decoder_table[current_code] = node;
        current_code <<= 1;
        stack[++depth] = -node;
      } while ((node = tree[0][-node]) < 0);

    codeword[node] = current_code;              // set codeword to leaf symbol
    int nb = codeword_length[node] = depth + 1;

    if (nb <= table_bits)                      // add entries to decoder table
      if (nb == table_bits)
        decoder_table[current_code] = node;
      else {
        int db = table_bits - nb, sc = current_code << db;
        for (int k = 1 << db; k;) decoder_table[sc+--k] = node;
      }

    while (current_code & 1) {                                    // backtrack
      current_code >>= 1;
      if (--depth < 0) break;
    }
    current_code |= 1;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - Coding implementations  - - - - - - - - - - - - - - - - - - - - - - - -

unsigned Binary_Codec::get_bit(void)
{
#ifdef _DEBUG
  if (mode != 2) HC_Error("decoder not initialized");
#endif
                                            // refill bit buffer (32-bit word)
  while (bit_position >= 8) {
    bit_buffer |= unsigned(*bc_pointer++) << (bit_position -= 8);
  }
                       // next data bit is most-significant bit in 32-bit word
  ++bit_position;
  unsigned data = bit_buffer >> 31;
  bit_buffer <<= 1;
  return data;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

unsigned Binary_Codec::get_bits(unsigned bits)
{
#ifdef _DEBUG
  if (mode != 2) HC_Error("decoder not initialized");
  if ((bits < 1) || (bits > 24)) HC_Error("invalid number of bits");
#endif
                                            // refill bit buffer (32-bit word)
  while (bit_position >= 8) {
    bit_buffer |= unsigned(*bc_pointer++) << (bit_position -= 8);
  }
                    // next data bits are most-significant bits in 32-bit word
  bit_position += bits;
  unsigned data = bit_buffer >> (32 - bits);
  bit_buffer <<= bits;
  return data;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Binary_Codec::put_bit(unsigned bit)
{
#ifdef _DEBUG
  if (mode != 1) HC_Error("encoder not initialized");
#endif
                      // save data bit in most-significant bits of 32-bit word
  --bit_position;
  if (bit) bit_buffer |= 1U << bit_position;
  if (bit_position <= 24)                    // empty bit buffer (32-bit word)
    do {
      *bc_pointer++ = (unsigned char) (bit_buffer >> 24);
      bit_buffer <<= 8;
    } while ((bit_position += 8) <= 24);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Binary_Codec::put_bits(unsigned data,
                            unsigned bits)
{
#ifdef _DEBUG
  if (mode != 1) HC_Error("encoder not initialized");
  if ((bits < 1) || (bits > 24)) HC_Error("invalid number of bits");
  if (data >= (1U << bits)) HC_Error("invalid data");
#endif
                     // save data bits in most-significant bits of 32-bit word
  bit_buffer |= data << (bit_position -= bits);
  if (bit_position <= 24)                    // empty bit buffer (32-bit word)
    do {
      *bc_pointer++ = (unsigned char) (bit_buffer >> 24);
      bit_buffer <<= 8;
    } while ((bit_position += 8) <= 24);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Binary_Codec::encode(unsigned data,
                          Static_Huffman_Code & C)
{
#ifdef _DEBUG
  if (mode != 1) HC_Error("encoder not initialized");
  if (data >= C.data_symbols) HC_Error("invalid data symbol");
#endif

  bit_buffer |= C.codeword[data] << (bit_position -= C.length[data]);  // code
  if (bit_position <= 24)
    do {                                                   // save bytes ready
      *bc_pointer++ = (unsigned char) (bit_buffer >> 24);
      bit_buffer <<= 8;
    } while ((bit_position += 8) <= 24);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Binary_Codec::encode(unsigned data,
                          Adaptive_Huffman_Code & C)
{
#ifdef _DEBUG
  if (mode != 1) HC_Error("encoder not initialized");
  if (data >= C.data_symbols) HC_Error("invalid data symbol");
#endif

  bit_buffer |= C.codeword[data] << (bit_position -= C.length[data]);  // code
  if (bit_position <= 24)
    do {                                                   // save bytes ready
      *bc_pointer++ = (unsigned char) (bit_buffer >> 24);
      bit_buffer <<= 8;
    } while ((bit_position += 8) <= 24);

  ++C.symbol_count[data];                          // update symbol statistics
  if (--C.symbols_until_update == 0) C.update(true);    // periodic adaptation
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#ifdef TREE_DECODE

unsigned Binary_Codec::decode(Static_Huffman_Code & C)
{
#ifdef _DEBUG
  if (mode != 2) HC_Error("decoder not initialized");
#endif

  while (bit_position >= 8)                                     // fill buffer
    bit_buffer |= unsigned(*bc_pointer++) << (bit_position -= 8);

  int data = -1;

  do {
    data = C.tree[bit_buffer>>31][-data];               // bit-by-bit decoding
    bit_buffer <<= 1;
  } while (data < 0);

  bit_position += C.length[data];

  return data;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

unsigned Binary_Codec::decode(Adaptive_Huffman_Code & C)
{
#ifdef _DEBUG
  if (mode != 2) HC_Error("decoder not initialized");
#endif

  while (bit_position >= 8)                                     // fill buffer
    bit_buffer |= unsigned(*bc_pointer++) << (bit_position -= 8);

  int data = -1;

  do {
    data = C.tree[bit_buffer>>31][-data];               // bit-by-bit decoding
    bit_buffer <<= 1;
  } while (data < 0);

  bit_position += C.length[data];

  ++C.symbol_count[data];                          // update symbol statistics
  if (--C.symbols_until_update == 0) C.update(false);   // periodic adaptation

  return data;
}

#else

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

unsigned Binary_Codec::decode(Static_Huffman_Code & C)
{
#ifdef _DEBUG
  if (mode != 2) HC_Error("decoder not initialized");
#endif

  while (bit_position >= 8)                                     // fill buffer
    bit_buffer |= unsigned(*bc_pointer++) << (bit_position -= 8);

  int data = C.decd_table[bit_buffer>>C.table_shift];        // table decoding

  if (data >= 0)
    bit_buffer <<= C.length[data];                                     // done
  else {
    bit_buffer <<= C.table_bits;                 // codeword longer than table
    do {                                                     // read more bits
      data = C.tree[bit_buffer>>31][-data];
      bit_buffer <<= 1;
    } while (data < 0);
  }

  bit_position += C.length[data];

  return data;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

unsigned Binary_Codec::decode(Adaptive_Huffman_Code & C)
{
#ifdef _DEBUG
  if (mode != 2) HC_Error("decoder not initialized");
#endif

  while (bit_position >= 8)                                     // fill buffer
    bit_buffer |= unsigned(*bc_pointer++) << (bit_position -= 8);

  int data = C.decd_table[bit_buffer>>C.table_shift];        // table decoding

  if (data >= 0)
    bit_buffer <<= C.length[data];                                     // done
  else {
    bit_buffer <<= C.table_bits;                 // codeword longer than table
    do {                                                     // read more bits
      data = C.tree[bit_buffer>>31][-data];
      bit_buffer <<= 1;
    } while (data < 0);
  }

  bit_position += C.length[data];

  ++C.symbol_count[data];                          // update symbol statistics
  if (--C.symbols_until_update == 0) C.update(false);   // periodic adaptation

  return data;
}

#endif


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - Other Binary_Codec implementations  - - - - - - - - - - - - - - - - - -

Binary_Codec::Binary_Codec(void)
{
  mode = buffer_size = 0;
  new_buffer = code_buffer = 0;
}

Binary_Codec::Binary_Codec(unsigned max_code_bytes,
                           unsigned char * user_buffer)
{
  mode = buffer_size = 0;
  new_buffer = code_buffer = 0;
  set_buffer(max_code_bytes, user_buffer);
}

Binary_Codec::~Binary_Codec(void)
{
  delete [] new_buffer;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Binary_Codec::set_buffer(unsigned max_code_bytes,
                              unsigned char * user_buffer)
{
                                                  // test for reasonable sizes
  if ((max_code_bytes < 16) /*|| (max_code_bytes > 0x1000000U)*/)
    HC_Error("invalid codec buffer size");
  if (mode != 0) HC_Error("cannot set buffer while encoding or decoding");

  if (user_buffer != 0) {                       // user provides memory buffer
    buffer_size = max_code_bytes;
    code_buffer = user_buffer;               // set buffer for compressed data
    delete [] new_buffer;                 // free anything previously assigned
    new_buffer = 0;
    return;
  }

  if (max_code_bytes <= buffer_size) return;               // enough available

  buffer_size = max_code_bytes;                           // assign new memory
  delete [] new_buffer;                   // free anything previously assigned
  if ((new_buffer = new unsigned char[buffer_size+8]) == 0)   // 8 extra bytes
    HC_Error("cannot assign memory for compressed data buffer");
  code_buffer = new_buffer;                  // set buffer for compressed data
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Binary_Codec::start_encoder(void)
{
  if (mode != 0) HC_Error("cannot start encoder");
  if (buffer_size == 0) HC_Error("no code buffer set");

  mode         =  1;             // initialize encoder bit-buffer and pointers
  bit_buffer   =  0;
  bit_position = 32;
  bc_pointer   = code_buffer;                     // pointer to next data byte
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Binary_Codec::start_decoder(void)
{
  if (mode != 0) HC_Error("cannot start decoder");
  if (buffer_size == 0) HC_Error("no code buffer set");

  mode         =  2;             // initialize decoder bit-buffer and pointers
  bit_buffer   =  0;
  bit_position = 32;
  bc_pointer   = code_buffer;                     // pointer to next data byte
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Binary_Codec::read_from_file(FILE * code_file)
{
  unsigned shift = 0, code_bytes = 0;
  int file_byte;
                      // read variable-length header with number of code bytes
  do {
    if ((file_byte = getc(code_file)) == EOF)
      HC_Error("cannot read code from file");
    code_bytes |= unsigned(file_byte & 0x7F) << shift;
    shift += 7;
  } while (file_byte & 0x80);
                                                       // read compressed data
  if (code_bytes > buffer_size) HC_Error("code buffer overflow");
  if (fread(code_buffer, 1, code_bytes, code_file) != code_bytes)
    HC_Error("cannot read code from file");

  start_decoder();                                       // initialize decoder
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

unsigned Binary_Codec::stop_encoder(void)
{
  if (mode != 1) HC_Error("invalid to stop encoder");

  mode = 0;                  // done encoding: set final data byte, reset mode

  if (bit_position < 32) *bc_pointer++ = (unsigned char) (bit_buffer >> 24);

  unsigned code_bytes = unsigned(bc_pointer - code_buffer);
  if (code_bytes > buffer_size) HC_Error("code buffer overflow");

  return code_bytes;                                   // number of bytes used
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

unsigned Binary_Codec::write_to_file(FILE * code_file)
{
  unsigned header_bytes = 0, code_bytes = stop_encoder(), nb = code_bytes;

                     // write variable-length header with number of code bytes
  do {
    int file_byte = int(nb & 0x7FU);
    if ((nb >>= 7) > 0) file_byte |= 0x80;
    if (putc(file_byte, code_file) == EOF)
      HC_Error("cannot write compressed data to file");
    header_bytes++;
  } while (nb);
                                                      // write compressed data
  if (fwrite(code_buffer, 1, code_bytes, code_file) != code_bytes)
    HC_Error("cannot write compressed data to file");

  return code_bytes + header_bytes;                              // bytes used
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Binary_Codec::stop_decoder(void)
{
  if (mode != 2) HC_Error("invalid to stop decoder");
  mode = 0;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - Static Huffman code implementation  - - - - - - - - - - - - - - - - - -

Static_Huffman_Code::Static_Huffman_Code(void)                  // constructor
{
  code_data    = 0;
  data_symbols = 0;
}

Static_Huffman_Code::~Static_Huffman_Code(void)                  // destructor
{
  delete [] code_data;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Static_Huffman_Code::set_distribution(unsigned number_of_symbols,
                                           const double probability[])
{
  if ((number_of_symbols < 3) || (number_of_symbols > (1 << 16)))
    HC_Error("invalid number of data symbols");

  if (data_symbols != number_of_symbols) {      // assign memory for code data

    data_symbols = number_of_symbols;
    delete [] code_data;                    // free previously assigned memory

    for (table_bits = 2; (1U << table_bits) < data_symbols; ++table_bits);

    unsigned table_size = 1U << ++table_bits;
    table_shift = 32 - table_bits;
                                                   // single memory assignment
    if ((code_data = new int[table_size+4*data_symbols]) == 0)
      HC_Error("cannot assign Huffman code memory");

    decd_table = code_data;                    // define pointers to code data
    tree[0] = code_data + table_size;
    tree[1] = tree[0]   + data_symbols;
    length  = (unsigned *) (tree[1] + data_symbols);
    codeword = length + data_symbols;
  }

  unsigned n, * symbol_count = codeword;        // temporarily use this memory
  if (probability == 0)
    for (n = 0; n < data_symbols; n++)
      symbol_count[n] = 1;
  else
    for (n = 0; n < data_symbols; n++)                  // convert to integers
      symbol_count[n] = unsigned(1.0 + 1e6 * probability[n]);

                                                     // construct optimal code
  Form_Huffman_Tree(data_symbols, symbol_count, tree[0], tree[1]);
  Set_Huffman_Code(table_bits, tree, decd_table, codeword, length);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - Adaptive data model implementation  - - - - - - - - - - - - - - - - - -

Adaptive_Huffman_Code::Adaptive_Huffman_Code(void)             // constructors
{
  code_data    = 0;
  data_symbols = 0;
}

Adaptive_Huffman_Code::Adaptive_Huffman_Code(unsigned number_of_symbols)
{
  code_data    = 0;
  data_symbols = 0;
  set_alphabet(number_of_symbols);
}

Adaptive_Huffman_Code::~Adaptive_Huffman_Code(void)             // destructors
{
  delete [] code_data;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Adaptive_Huffman_Code::set_alphabet(unsigned number_of_symbols)
{
  if ((number_of_symbols < 3) || (number_of_symbols > (1 << 12)))
    HC_Error("invalid number of data symbols");

  if (data_symbols != number_of_symbols) {     // assign memory for code data

    data_symbols = number_of_symbols;
    delete [] code_data;                    // free previously assigned memory

    for (table_bits = 2; (1U << table_bits) < data_symbols; ++table_bits);

    unsigned table_size = 1U << ++table_bits;
    table_shift = 32 - table_bits;
                                                   // single memory assignment
    if ((code_data = new int[table_size+5*data_symbols]) == 0)
      HC_Error("cannot assign Huffman code memory");

    decd_table = code_data;                    // define pointers to code data
    tree[0] = code_data + table_size;
    tree[1] = tree[0]   + data_symbols;
    length  = (unsigned *) (tree[1] + data_symbols);
    codeword = length + data_symbols;
    symbol_count = codeword + data_symbols;
  }

  reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Adaptive_Huffman_Code::update(bool encoder)
{
  total_count += update_cycle;
  while (total_count >= HC__MaxCount)
    for (unsigned n = total_count = 0; n < data_symbols; n++)
      total_count += (symbol_count[n] = (symbol_count[n] + 1) >> 1);

                                                // compute optimal Hufman code
  Form_Huffman_Tree(data_symbols, symbol_count, tree[0], tree[1]);
  if (encoder)
    Set_Huffman_Code(tree, codeword, length);              // no decoder table
  else
    Set_Huffman_Code(table_bits, tree, decd_table, codeword, length);

  if (update_cycle < (data_symbols << 4))
    update_cycle = 1 + ((5 * update_cycle) >> 2);
  symbols_until_update = update_cycle;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Adaptive_Huffman_Code::reset(void)
{                                                      // uniform distribution
  total_count = 0;
  for (unsigned k = 0; k < data_symbols; k++) symbol_count[k] = 1;
  update_cycle = symbols_until_update = data_symbols;
  update(false);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
