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
#include <iostream>
#include "Doboz/Compressor.h"
#include "Doboz/Decompressor.h"
#include "Utils/Timer.h"

using namespace afra;
using namespace std;

const size_t MAX_BUFFER_SIZE = static_cast<size_t>(-1);
const double MEGABYTE = 1024.0 * 1024.0;
const size_t KILOBYTE = 1024;

char* inputBuffer = 0;
size_t inputSize;

char* outputBuffer = 0;
size_t outputSize;

#if defined(_WIN32)
#define FSEEK64 _fseeki64
#define FTELL64 _ftelli64
#else
#define FSEEK64 fseeko64
#define FTELL64 ftello64
#endif

bool loadInputFile(char* filename)
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
	inputSize = static_cast<size_t>(originalSize64);
	FSEEK64(file, 0, SEEK_SET);

	cout << "Loading file \"" << filename << "\"..." << endl;
	cout << "Size: " << static_cast<double>(inputSize) / MEGABYTE << " MB (" << inputSize / KILOBYTE << " KB)" << endl;

	inputBuffer = new char[inputSize];

	if (fread(inputBuffer, 1, inputSize, file) != inputSize)
	{
		cout << "ERROR: I/O error" << endl;
		fclose(file);
		return false;
	}
	fclose(file);
	return true;
}

bool saveOutputFile(char* filename)
{
	FILE* file = fopen(filename, "wb");
	if (file == 0)
	{
		cout << "ERROR: Cannot open file \"" << filename << "\"" << endl;
		return false;
	}

	cout << "Saving file \"" << filename << "\"..." << endl;

	if (fwrite(outputBuffer, 1, outputSize, file) != outputSize)
	{
		cout << "ERROR: I/O error" << endl;
		fclose(file);
		return false;
	}
	fclose(file);
	return true;
}

void cleanup()
{
	delete[] inputBuffer;
	delete[] outputBuffer;
}

void printUsage()
{
	cout << "Usage: doboz c|d input output" << endl;
}

int main(int argc, char* argv[])
{
	cout << "Doboz Data Compressor 0.1" << endl;
	cout << "Copyright (C) 2010-2011 Attila T. Afra <attila.afra@gmail.com>" << endl;
	cout << endl;

	if (argc != 4)
	{
		cout << "Usage: doboz c|d input output" << endl;
		return 0;
	}

	if (toupper(argv[1][0]) == 'C')
	{
		// Compress
		if (!loadInputFile(argv[2]))
		{
			cleanup();
			return 1;
		}

		size_t outputBufferSize = static_cast<size_t>(doboz::Compressor::getMaxCompressedSize(inputSize));
		outputBuffer = new char[outputBufferSize];

		cout << "Compressing..." << endl;
		doboz::Compressor compressor;
		Timer timer;
		doboz::Result result = compressor.compress(inputBuffer, inputSize, outputBuffer, outputBufferSize, outputSize);
		double compressionTime = timer.query();

		if (result != doboz::RESULT_OK)
		{
			cout << "ERROR: Compression failed" << endl;
			cleanup();
			return 1;
		}

		double mbps = static_cast<double>(inputSize) / MEGABYTE / compressionTime;
		double compressionRatio = static_cast<double>(outputSize) / static_cast<double>(inputSize) * 100.0;
		cout << "Compressed in " << compressionTime << " s, " << mbps << " MB/s" << endl;
		cout << "Compressed size: " << static_cast<double>(outputSize) / MEGABYTE << " MB (" << outputSize / KILOBYTE << " KB)" << endl;
		cout << "Compression ratio: " << compressionRatio << "%" << endl;

		if (!saveOutputFile(argv[3]))
		{
			cleanup();
			return 1;
		}

		cout << "Done" << endl;
	}
	else if (toupper(argv[1][0]) == 'D')
	{
		// Decompress
		if (!loadInputFile(argv[2]))
		{
			cleanup();
			return 1;
		}

		doboz::Decompressor decompressor;

		doboz::CompressionInfo compressionInfo;
		doboz::Result result = decompressor.getCompressionInfo(inputBuffer, inputSize, compressionInfo);
		if (result != doboz::RESULT_OK)
		{
			cout << "ERROR: Decompression failed" << endl;
			cleanup();
			return 1;
		}

		if (compressionInfo.uncompressedSize > MAX_BUFFER_SIZE)
		{
			cout << "ERROR: Decompressed data is too large" << endl;
			cleanup();
			return 1;
		}

		outputSize = static_cast<size_t>(compressionInfo.uncompressedSize);
		outputBuffer = new char[outputSize];

		cout << "Decompressing..." << endl;
		Timer timer;
		result = decompressor.decompress(inputBuffer, inputSize, outputBuffer, outputSize);
		double decompressionTime = timer.query();
		if (result != doboz::RESULT_OK)
		{
			cout << "ERROR: Decompression failed" << endl;
			cleanup();
			return 1;
		}

		double mbps = static_cast<double>(outputSize) / MEGABYTE / decompressionTime;
		cout << "Decompression speed: " << mbps << " MB/s" << endl;

		if (!saveOutputFile(argv[3]))
		{
			cleanup();
			return 1;
		}

		cout << "Done" << endl;
	}
	else
	{
		printUsage();
		return 0;
	}

	cleanup();
	return 0;
}