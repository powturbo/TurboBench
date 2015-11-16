// Prefixer.cpp : Defines the entry point for the console application.
// (C) 2010, Andrew Polar under GPL ver. 3.
// Released April, 2010
//
//   LICENSE
//
//   This program is free software; you can redistribute it and/or
//   modify it under the terms of the GNU General Public License as
//   published by the Free Software Foundation; either version 3 of
//   the License, or (at your option) any later version.
//
//   This program is distributed in the hope that it will be useful, but
//   WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//   General Public License for more details at
//   Visit <http://www.gnu.org/copyleft/gpl.html>.
//


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <memory.h>
#include <math.h>

unsigned long long  bit_holding_buffer64;
unsigned char*      global_pointer_to_data_buffer;
unsigned char       bit_counter;
unsigned int        current_byte;

///////auxiliary functions///////////////////////////////
template <class T>
double calculate_entropy(T* data, int data_size) {
	
	int min, max, counter, buffer_size;
	int* buffer;
	double entropy;
	double log2 = log(2.0);
	double prob;

	min = data[0];
	max = data[0];
	for (counter=0; counter<data_size; ++counter) {
		if (data[counter] < min)
			min = data[counter];
		if (data[counter] > max)
			max = data[counter];
	}

	buffer_size = max - min + 1;
	buffer = (int*)malloc(buffer_size * sizeof(int));
	memset(buffer, 0x00, buffer_size * sizeof(int));
	for (counter=0; counter<data_size; ++counter) {
		++buffer[data[counter]-min];
	}

	entropy = 0.0;
	for (counter=0; counter<buffer_size; ++counter) {
		if (buffer[counter] > 0) {
			prob = (double)buffer[counter];
			prob /= (double)data_size;
			entropy += log(prob)/log2*prob;
		}
	}
	entropy *= -1.0;

	if (buffer)
		free(buffer);

	return  entropy;
}

int getFileSize(char* fileName) {
	FILE* f;
	if ((f = fopen(fileName, "rb")) == NULL)
		return -1;
	fseek(f, 0, SEEK_END);
	int bytes = ftell(f);
	fclose(f);
	return bytes;
}

//returns the bitlength of passed number
template <class AllIntegers>
static __inline int bitLen(AllIntegers n) {
	if (n < 0) 
		n = -n;
	int cnt = 1;
	while ((n >>= 1) > 0) 
		++cnt;
	return cnt;
}
//////end auxiliary functions list//////////////////

//input output
static __inline int readBits(int max_len) {
	if ((bit_counter - max_len) < 0) {
		bit_holding_buffer64 <<= 8;
		bit_holding_buffer64 |= global_pointer_to_data_buffer[current_byte++];
		bit_holding_buffer64 <<= 8;
		bit_holding_buffer64 |= global_pointer_to_data_buffer[current_byte++];
		bit_holding_buffer64 <<= 8;
		bit_holding_buffer64 |= global_pointer_to_data_buffer[current_byte++];
		bit_holding_buffer64 <<= 8;
		bit_holding_buffer64 |= global_pointer_to_data_buffer[current_byte++];
		bit_counter += 32;
	}
	return (int)(bit_holding_buffer64 >> (bit_counter - max_len)) & ((1 << max_len)-1);
}

static __inline void writeBits(int codeLen, int code) {
	bit_holding_buffer64 <<= codeLen;
	bit_holding_buffer64 |= code;
	bit_counter += codeLen;
	if (bit_counter > 31) {
		unsigned char offset = bit_counter - 32;
		global_pointer_to_data_buffer[current_byte++] = (unsigned char)(bit_holding_buffer64 >> (offset + 24));
		global_pointer_to_data_buffer[current_byte++] = (unsigned char)(bit_holding_buffer64 >> (offset + 16));
		global_pointer_to_data_buffer[current_byte++] = (unsigned char)(bit_holding_buffer64 >> (offset + 8));
		global_pointer_to_data_buffer[current_byte++] = (unsigned char)(bit_holding_buffer64 >> offset);
		bit_counter -= 32;
	}
}
//end input output

class Prefixer {
public:
	Prefixer();
	~Prefixer();
	bool  Initialize   (short AlphabetSize, bool isDecoding);
	void  ReleaseMemory();
	void  EncodeSymbol (short value);
	short DecodeSymbol ();
	void  FlushBits    ();
private:
	short GetSymbol    (short value);
	short InvertSymbol (short value);
	void  UpdateTables (short value);
	void  BuildTree    (bool isDecoding);
	bool  allocateDecodingTables();
	void  freeDecodingTables();

	int *m_fragment_len, *m_shift, *m_lowest_code, *m_offset, *m_code;
	short *m_frequency, *m_symbol, *m_inverse; 
	unsigned char *m_codeLen;
	int m_alphabet_size, m_max_code_len, m_tableSize, m_counter, m_size_of_adaptation;
	int m_max_frequency_limit, m_min_size_of_adaptation, m_max_size_of_adaptation;	  
};

bool Prefixer::Initialize(short AlphabetSize, bool isDecoding) {
	m_alphabet_size = AlphabetSize;
	m_frequency = m_symbol = m_inverse = 0;
	m_frequency = (short*) malloc(m_alphabet_size * sizeof(*m_frequency));
	m_symbol    = (short*) malloc(m_alphabet_size * sizeof(*m_symbol));
	m_inverse   = (short*) malloc(m_alphabet_size * sizeof(*m_inverse));
	m_codeLen   = (unsigned char*) malloc(m_alphabet_size * sizeof(*m_codeLen));
	m_code      = (int*) malloc(m_alphabet_size * sizeof(*m_code));
	if (m_frequency == 0) return false;
	if (m_symbol    == 0) return false;
	if (m_inverse   == 0) return false;
	if (m_codeLen   == 0) return false;
	if (m_code      == 0) return false;
	for (int i=0; i<m_alphabet_size; ++i) {
		m_frequency[i] = 0; m_symbol[i] = i; m_inverse[i] = i;
	}

	m_fragment_len = m_shift = m_lowest_code = m_offset = 0;

	BuildTree(isDecoding);
	m_counter = 0;
	m_max_frequency_limit    = 800;	  
	m_min_size_of_adaptation = 16; 
	m_max_size_of_adaptation = 4096; //1024*16; //;
	m_size_of_adaptation = m_min_size_of_adaptation;

	//global parameters
	current_byte = 0, bit_counter  = 0, bit_holding_buffer64 = 0;
	return true;
}

Prefixer::Prefixer() {
}

void Prefixer::ReleaseMemory() {
	freeDecodingTables();
	if (m_code)      free (m_code);
	if (m_codeLen)   free (m_codeLen);
	if (m_inverse)   free (m_inverse);
	if (m_symbol)    free (m_symbol);
	if (m_frequency) free(m_frequency);
	m_frequency = m_symbol = m_inverse = 0;
}

Prefixer::~Prefixer() {
	ReleaseMemory();
}

bool Prefixer::allocateDecodingTables() {
	freeDecodingTables();
	m_fragment_len = (int*) malloc(m_tableSize * sizeof(*m_fragment_len));
	m_shift        = (int*) malloc(m_tableSize * sizeof(*m_shift));
	m_lowest_code  = (int*) malloc(m_tableSize * sizeof(*m_lowest_code));
	m_offset       = (int*) malloc(m_tableSize * sizeof(*m_offset));
	if (!m_fragment_len) return false;
	if (!m_shift)        return false;
	if (!m_lowest_code)  return false;
	if (!m_offset)       return false;
	return true;
}

void Prefixer::freeDecodingTables() {
	if (m_offset)       free(m_offset);
	if (m_lowest_code)  free(m_lowest_code);
	if (m_shift)        free(m_shift);
	if (m_fragment_len) free(m_fragment_len);
	m_lowest_code  = 0;
	m_shift        = 0;
	m_fragment_len = 0;
	m_offset       = 0;
}

void Prefixer::UpdateTables(short value) {
	++m_frequency[value];
	if (m_frequency[value] >= m_max_frequency_limit) {
		for (int j=0; j<m_alphabet_size; ++j) {
			m_frequency[j] >>= 1;
		}
	}
	if (m_symbol[value] == 0) {
		return;
	}
	short npos = -1;
	short pos  = m_symbol[value];
	short freq = m_frequency[value];
	for (short i=pos-1; i>=0; --i) {
		if (freq > m_frequency[m_inverse[i]]) {
			npos = i;
		}
		else {
			break;
		}
	}
	if (npos < 0) {
		return;
	}
	short buf = m_symbol[m_inverse[pos]];
	m_symbol[m_inverse[pos]] = m_symbol[m_inverse[npos]];
	m_symbol[m_inverse[npos]] = buf;
	buf = m_inverse[pos];
	m_inverse[pos]  = m_inverse[npos];
	m_inverse[npos] = buf;
}

short Prefixer::GetSymbol(short value) {
	return m_symbol[value];
}

short Prefixer::InvertSymbol(short value) {
	return m_inverse[value];
}

void Prefixer::BuildTree(bool isDecoding) {
	short* tmpFreq = (short*)malloc(m_alphabet_size * sizeof(*tmpFreq));
	for (int i=0; i<m_alphabet_size; ++i) {
		tmpFreq[i] = m_frequency[m_inverse[i]];
		if (tmpFreq[i] == 0) 
			tmpFreq[i] = 1;
	}

	int total = 0;
	int power2Total = 0;
	for (int i=0; i<m_alphabet_size; ++i) {
		total += tmpFreq[i];
		//we reduce frequency to the nearest power of 2
		int len = bitLen(tmpFreq[i]);
		tmpFreq[i] = 1 << (len-1);
		power2Total += tmpFreq[i];
	}
	int treeTotal = 1 << (bitLen(total) - 1);
	if (treeTotal < total)
		treeTotal <<= 1;

	//adjust all frequencies to match treeTotal
	bool corrected = false;
	int  next_starting_position = 0;
	do {
		corrected = false;
		for (int i=next_starting_position; i<m_alphabet_size; ++i) {
			if ((power2Total + tmpFreq[i]) <= treeTotal) {
				power2Total += tmpFreq[i];
				tmpFreq[i] <<= 1;
				corrected = true;
			}
			else {
				next_starting_position = i+1;
			}
		}
	} while (corrected == true);

	//find codelengthes
	m_max_code_len = 0;
	for (int i=0; i<m_alphabet_size; ++i) {
		m_codeLen[i] = 1;
		int tmp = treeTotal;
		while ((tmp >>= 1) > tmpFreq[i]) {
			++m_codeLen[i];
		}
		if (m_codeLen[i] > m_max_code_len) {
			m_max_code_len = m_codeLen[i];
		}
	}

	//find codes
	m_code[0] = 0;
	for (int i=1; i<m_alphabet_size; ++i) {
		m_code[i] = m_code[i-1] + 1;
		m_code[i] <<= (m_codeLen[i] - m_codeLen[i-1]);
	}

	if (tmpFreq)
		free(tmpFreq);

	if (!isDecoding)
		return;

	//decoding tables
	//find size for tables
	m_tableSize = 1;
	for (int i=1; i<m_alphabet_size; ++i) {
		if (m_codeLen[i] != m_codeLen[i-1])
			++m_tableSize;
	}
	
	//reallocate memory for tables, because they are different each time
	if (!allocateDecodingTables()) {
		printf("Failed to allocate decoding tables\n");
		return;
	}

	//these tables are to expedite search of symols, they are small 
	int cnt = 0, level = 0, prev_offset = 0;
	int prev_code_len = m_codeLen[0];
	int prev_code     = m_code[0];
	for (int i=0; i<m_alphabet_size; ++i) {
		if (m_codeLen[i] != prev_code_len) {
			m_fragment_len[level] = cnt;
			m_lowest_code [level] = prev_code;
			m_offset      [level] = prev_offset; 
			m_shift       [level] = m_max_code_len - m_codeLen[i-1];
			++level;

			prev_offset   = i;
			prev_code_len = m_codeLen[i];
			prev_code     = m_code[i];
			cnt = 1;
		}
		else {
			++cnt;
		}
	}
	m_fragment_len[level] = cnt;
	m_lowest_code [level] = prev_code;
	m_offset      [level] = prev_offset; 
	m_shift       [level] = m_max_code_len - m_codeLen[m_alphabet_size-1];
	//end building of lookup tables for decoding
}

void Prefixer::EncodeSymbol(short value) {
	short symbol = GetSymbol(value);
	UpdateTables(value);
	writeBits(m_codeLen[symbol], m_code[symbol]);
	++m_counter;
	if (m_counter >= m_size_of_adaptation) {
		BuildTree(false);
		m_counter = 0;
		if (m_size_of_adaptation < m_max_size_of_adaptation)
			m_size_of_adaptation <<= 1;
	}
}

void Prefixer::FlushBits() {
	if (bit_counter > 0) {
		writeBits(32-bit_counter, 0);
	}
}

short Prefixer::DecodeSymbol() {
	int address = readBits(m_max_code_len);
	short symbol = 0;
	bool  isFound = false;
	for (int i=0; i<m_tableSize; ++i) {
		int pos = (address >> m_shift[i]) - m_lowest_code[i];
		if (pos < m_fragment_len[i]) {
			symbol = InvertSymbol(pos + m_offset[i]);
			bit_counter -= m_max_code_len - m_shift[i];
			isFound = true;
			break;
		}
	}
	if (!isFound) {
		printf("Error decoding\n");
	}
	UpdateTables(symbol);
	++m_counter;
	if (m_counter >= m_size_of_adaptation) {
		BuildTree(true);
		m_counter = 0;
		if (m_size_of_adaptation < m_max_size_of_adaptation)
			m_size_of_adaptation <<= 1;
	}
	return symbol;
}

class ContextManager {
public:
	ContextManager(int alphabet_size, bool isDecoding);
	~ContextManager();
	void  EncodeSymbol(short symbol);
	void  CompleteAndFlushBits();
	short DecodeSymbol();
private:
	Prefixer* m_ptr_prefixer;
	int m_context_size, m_context_bit_len, m_alphabet_size;
	int m_context, m_shift, m_mask;
};

ContextManager::ContextManager(int alphabet_size, bool isDecoding) {
	m_alphabet_size = alphabet_size;
	m_context_bit_len = 11;
	m_context_size = 1<<m_context_bit_len;
	m_ptr_prefixer = (Prefixer*)malloc(m_context_size * sizeof(*m_ptr_prefixer));
	for (int i=0; i<m_context_size; ++i) {
		m_ptr_prefixer[i].Initialize(m_alphabet_size + 1, isDecoding);
	}
	m_context = 0;
	m_shift   = m_context_bit_len - 8;
	m_mask    = (0xff << m_shift) & 0xff00;
}

ContextManager::~ContextManager() {
	if (m_ptr_prefixer) {
		for (int i=0; i<m_context_size; ++i) {
			m_ptr_prefixer[i].ReleaseMemory();
		}
		free(m_ptr_prefixer);
	}
}

void ContextManager::EncodeSymbol(short symbol) {
	m_ptr_prefixer[0/*m_context*/].EncodeSymbol(symbol);
	/*m_context <<= m_shift;
	m_context &= m_mask;
	m_context |= symbol;*/
}

short ContextManager::DecodeSymbol() {
	short symbol = m_ptr_prefixer[0/*m_context*/].DecodeSymbol();
	/*m_context <<= m_shift;
	m_context &= m_mask;
	m_context |= symbol;*/
	return symbol;
}

void ContextManager::CompleteAndFlushBits() {
	EncodeSymbol(m_alphabet_size);
	m_ptr_prefixer[m_context].FlushBits();
}

#if 1
#include "polar.h"
int polarenc(unsigned char *in, int inlen, unsigned char *_dst) {  unsigned char *dst = _dst; int i; 
  current_byte = 0 ;
  global_pointer_to_data_buffer = _dst; //(unsigned char*)malloc((inlen + inlen/4) * sizeof(*global_pointer_to_data_buffer));
  ContextManager* coder = new ContextManager(256, false);
  for(i = 0; i < inlen; i++) coder->EncodeSymbol(in[i]);
  coder->CompleteAndFlushBits();
  delete coder;
  return current_byte;
}
 
int polardec(unsigned char *in, unsigned char *_dst, int outlen ) { unsigned char *src = (unsigned char *)in; int i;
  global_pointer_to_data_buffer = in;
  ContextManager* decoder = new ContextManager(256, true);
  for(i = 0; i < outlen; i++) _dst[i] = decoder->DecodeSymbol();;
  delete decoder;
}
#else
int main() {

	char fileName[] = "C:\\tmp\\c\\ubuntu500m";
	int data_size = getFileSize(fileName);
	if (data_size < 0) {
		printf("File not found\n");
		return 0;
	}

	unsigned char* data = (unsigned char*)malloc(data_size * sizeof(*data));
	FILE* f = fopen(fileName, "rb");
	fread(data, 1, data_size, f);
	fclose(f);

	//encoding
	clock_t start = clock();
	global_pointer_to_data_buffer = (unsigned char*)malloc((data_size + data_size/4) * sizeof(*global_pointer_to_data_buffer));
	ContextManager* coder = new ContextManager(256, false);
	for (int i=0; i<data_size; ++i) {
		coder->EncodeSymbol(data[i]);
	}
	coder->CompleteAndFlushBits();
	delete coder;
	clock_t end = clock();
	int encoded_size = current_byte;
	printf("Encoding, time    %2.3f s.%d\n", (double)(end - start)/CLOCKS_PER_SEC, encoded_size);
	//end encoding

	//decoding
	start = clock();
	unsigned char* test = (unsigned char*)malloc((data_size + data_size/4) * sizeof(*test));
	ContextManager* decoder = new ContextManager(256, true);
	int test_size = 0;
	while (true) {
		short value = decoder->DecodeSymbol();
		if (value == 256)
			break;
		test[test_size++] = (unsigned char)value;
	}
	delete decoder;
	end = clock();
	printf("Decoding, time    %2.3f s.\n", (double)(end - start)/CLOCKS_PER_SEC);
	//end decoding

	bool isOK = true;
	if (test_size != data_size) {
		printf("Data mismatch %d %d\n", test_size, data_size);
		isOK = false;
	}
	else {
		for (int i=0; i<test_size; ++i) {
			if (test[i] != data[i]) {
				printf("Data mismatch %d\n", i);
				isOK = false;
				break;
			}
		}
	}

	if (isOK == true)
		printf("Round trip OK\n");

	if (test)
		free(test);

	double entropy = calculate_entropy(data, data_size);
	printf("Entropy coder compression %f, actual compression %f\n", entropy/8.0,
		double(encoded_size)/double(data_size));

	if (global_pointer_to_data_buffer)
		free(global_pointer_to_data_buffer);

	if (data)
		free(data);

	return 0;
}
#endif