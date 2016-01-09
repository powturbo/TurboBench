/*
 * Doboz Data Compression Library
 * Copyright (C) 2010-2011 Attila T. Afra <attila.afra@gmail.com>
 * 
 * This software is provided 'as-is', without any express or implied warranty. In no event will
 * the authors be held liable for any damages arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose, including commercial
 * applications, and to alter it and redistribute it freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not claim that you wrote the
 *    original software. If you use this software in a product, an acknowledgment in the product
 *    documentation would be appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be misrepresented as
 *    being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#include <cstdio>
#include <cstring>
#include <iostream>
#include <algorithm>
#include "Doboz/Compressor.h"
#include "Doboz/Decompressor.h"
#include "Utils/Timer.h"
#include "Utils/FastRng.h"

using namespace std;
using namespace afra;

const size_t MAX_BUFFER_SIZE = static_cast<size_t>(-1);
const double MEGABYTE = 1024.0 * 1024.0;
const size_t KILOBYTE = 1024;

char* originalBuffer = 0;
size_t originalSize;

char* compressedBuffer = 0;
char* tempCompressedBuffer = 0;
size_t compressedSize;
size_t compressedBufferSize;

char* decompressedBuffer = 0;

#if defined(_WIN32)
#define FSEEK64 _fseeki64
#define FTELL64 _ftelli64
#else
#define FSEEK64 fseeko64
#define FTELL64 ftello64
#endif

bool loadFile(char* filename)
{
	FILE* file = fopen(filename, "rb");
	if (file == 0)
	{
		cout << "ERROR: Cannot open file \"" << filename << "\"" << endl;
		return false;
	}

	FSEEK64(file, 0, SEEK_END);
	uint64_t originalSize64 = FTELL64(file);
	if (doboz::Compressor::getMaxCompressedSize(originalSize64) > MAX_BUFFER_SIZE)
	{
		cout << "ERROR: File \"" << filename << "\" is too large" << endl;
		fclose(file);
		return false;
	}
	originalSize = static_cast<size_t>(originalSize64);
	FSEEK64(file, 0, SEEK_SET);

	cout << "Loading file \"" << filename << "\"..." << endl;
	cout << "Size: " << static_cast<double>(originalSize) / MEGABYTE << " MB (" << originalSize / KILOBYTE << " KB)" << endl;

	originalBuffer = new char[originalSize];

	if (fread(originalBuffer, 1, originalSize, file) != originalSize)
	{
		cout << "ERROR: I/O error" << endl;
		fclose(file);
		return false;
	}
	fclose(file);
	return true;
}

void initializeTests()
{
	compressedBufferSize = static_cast<size_t>(doboz::Compressor::getMaxCompressedSize(originalSize));
	compressedBuffer = new char[compressedBufferSize];
	tempCompressedBuffer = new char[compressedBufferSize];

	decompressedBuffer = new char[originalSize];
}

void cleanup()
{
	delete[] compressedBuffer;
	delete[] tempCompressedBuffer;
	delete[] decompressedBuffer;

	delete[] originalBuffer;
}

bool verifyDecompressed()
{	
	for (size_t i = 0; i < originalSize; ++i)
	{
		if (decompressedBuffer[i] != originalBuffer[i])
		{
			return false;
		}
	}

	return true;
}

void prepareDecompression()
{
	memcpy(tempCompressedBuffer, compressedBuffer, compressedSize);
	memset(decompressedBuffer, 0, originalSize);
}

// Decompresses from tempCompressedBuffer!
bool decompress()
{
	doboz::Decompressor decompressor;

	doboz::Result result = decompressor.decompress(tempCompressedBuffer, compressedSize, decompressedBuffer, originalSize);
	if (result != doboz::RESULT_OK)
		return false;

	return verifyDecompressed();
}

bool basicTest()
{
	doboz::Compressor compressor;
	doboz::Result result;

	cout << "Basic test" << endl;
	cout << "Encoding..." << endl;
	memset(compressedBuffer, 0, compressedBufferSize);
	result = compressor.compress(originalBuffer, originalSize, compressedBuffer, compressedBufferSize, compressedSize);
	if (result != doboz::RESULT_OK)
	{
		cout << "Encoding FAILED" << endl;
		return false;
	}

	cout << "Decoding..." << endl;
	prepareDecompression();
	if (!decompress())
	{
		cout << "Decoding/verification FAILED" << endl;
		return false;
	}

	cout << "Decoding and verification successful" << endl;
	return true;
}

bool corruptionTest()
{
	FastRng rng;

	cout << "Corruption test" << endl;

	int maxErrorCount = 100;

	int testCount = 1000;
	int failedDecodingCount = 0;
	for (int i = 0; i < testCount; ++i)
	{
		cout << "\r" << (i + 1) << "/" << testCount;
		prepareDecompression();

		// Corrupt compressed data
		int errorCount = (rng.getUint() % maxErrorCount) + 1;
		for (int j = 0; j < errorCount; ++j)
		{
			int errorPosition = rng.getUint() % static_cast<uint32_t>(compressedSize);
			tempCompressedBuffer[errorPosition] = rng.getUint() % 256;
		}

		// Try to decompress
		doboz::Decompressor decompressor;
		doboz::Result result = decompressor.decompress(tempCompressedBuffer, compressedSize, decompressedBuffer, originalSize);
		if (result != doboz::RESULT_OK)
		{
			++failedDecodingCount;
		}
	}

	cout << endl;
	cout << "Decoding errors: " << failedDecodingCount << "/" << testCount << endl;
	return true;
}

bool incrementalTest()
{
	doboz::Compressor compressor;
	doboz::Result result;

	cout << "Incremental input size test" << endl;
	size_t totalOriginalSize = originalSize;

	size_t testCount = min(static_cast<size_t>(512), originalSize);
	for (size_t i = 1; i <= testCount; ++i)
	{
		cout << "\r" << i << "/" << testCount;

		originalSize = i;
		memset(compressedBuffer, 0, compressedBufferSize);
		result = compressor.compress(originalBuffer, originalSize, compressedBuffer, compressedBufferSize, compressedSize);
		if (result != doboz::RESULT_OK)
		{
			cout << endl << "Encoding FAILED" << endl;
			originalSize = totalOriginalSize;
			return false;
		}

		prepareDecompression();
		if (!decompress())
		{
			cout << endl << "Decoding/verification FAILED" << endl;
			originalSize = totalOriginalSize;
			return false;
		}
	}

	cout << endl;
	originalSize = totalOriginalSize;
	return true;
}

int main(int argc, char* argv[])
{
	cout << "Doboz Data Compression Library - TEST" << endl;
	cout << "Copyright (C) 2010-2011 Attila T. Afra <attila.afra@gmail.com>" << endl;
	cout << endl;

	if (argc != 2)
	{
		cout << "Usage: Benchmark file" << endl;
		return 0;
	}

	bool ok = loadFile(argv[1]);
	if (!ok)
	{
		return 1;
	}

	cout << endl;

	// Basic test
	initializeTests();
	cout << "1. ";
	ok = basicTest();
	if (!ok)
	{
		cleanup();
		return 1;
	}
	cout << endl;
	bool allOk = true;

	// Corruption test
	cout << "2. ";
	allOk = allOk && corruptionTest();
	cout << endl;

	// Incremental input size test
	cout << "3. ";
	allOk = allOk && incrementalTest();

	cleanup();

	cout << endl;
	if (allOk)
	{
		cerr << "OK" << endl;
	}
	else
	{
		cerr << "ERROR" << endl;
	}

	return 0;
}