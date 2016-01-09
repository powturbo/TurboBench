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
#include "qlz/quicklz.h"
#include "zlib/zlib.h"

using namespace std;
using namespace afra;

const size_t MAX_BUFFER_SIZE = static_cast<size_t>(-1);
const double MEGABYTE = 1024.0 * 1024.0;
const size_t KILOBYTE = 1024;

char* originalBuffer = 0;
size_t originalSize;

char* compressedBuffer = 0;
size_t compressedSize;
size_t compressedBufferSize;

char* decompressedBuffer = 0;


struct DobozCodec
{
	doboz::Compressor compressor;
	doboz::Decompressor decompressor;

	const char* getName()
	{
		return "Doboz";
	}
	
	size_t getMaxCompressedSize()
	{
		return static_cast<size_t>(doboz::Compressor::getMaxCompressedSize(originalSize));
	}

	bool compress()
	{
		doboz::Result result = compressor.compress(originalBuffer, originalSize, compressedBuffer, compressedBufferSize, compressedSize);
		return result == doboz::RESULT_OK;
	}

	bool decompress()
	{
		doboz::Result result = decompressor.decompress(compressedBuffer, compressedSize, decompressedBuffer, originalSize);
		return result == doboz::RESULT_OK;
	}
};


struct QlzCodec
{
	static qlz_state_compress compressState;
	static qlz_state_decompress decompressState;

	const char* getName()
	{
		return "QuickLZ(3)";
	}
	
	size_t getMaxCompressedSize()
	{
		return originalSize + 1024;
	}

	bool compress()
	{
		compressedSize = qlz_compress(originalBuffer, compressedBuffer, originalSize, &compressState);
		return compressedSize > 0;
	}

	bool decompress()
	{
		size_t decompressedSize = qlz_decompress(compressedBuffer, decompressedBuffer, &decompressState);
		return decompressedSize == originalSize;
	}
};

qlz_state_compress QlzCodec::compressState;
qlz_state_decompress QlzCodec::decompressState;


struct ZlibCodec
{
	const char* getName()
	{
		return "zlib(9)";
	}
	
	size_t getMaxCompressedSize()
	{
		int ret;
		z_stream strm;

		strm.zalloc = Z_NULL;
		strm.zfree = Z_NULL;
		strm.opaque = Z_NULL;
		strm.avail_in = static_cast<uInt>(originalSize);
        strm.next_in = reinterpret_cast<Bytef*>(originalBuffer);
		strm.avail_out = static_cast<uInt>(compressedBufferSize);
		strm.next_out = reinterpret_cast<Bytef*>(compressedBuffer);
		ret = deflateInit(&strm, 9);
		if (ret != Z_OK)
			return 0;

		size_t result = deflateBound(&strm, static_cast<uLong>(originalSize));
		deflateEnd(&strm);
		return result;
	}

	bool compress()
	{
		int ret;
		z_stream strm;

		strm.zalloc = Z_NULL;
		strm.zfree = Z_NULL;
		strm.opaque = Z_NULL;
		strm.avail_in = static_cast<uInt>(originalSize);
        strm.next_in = reinterpret_cast<Bytef*>(originalBuffer);
		strm.avail_out = static_cast<uInt>(compressedBufferSize);
		strm.next_out = reinterpret_cast<Bytef*>(compressedBuffer);
		ret = deflateInit(&strm, 9);
		if (ret != Z_OK)
			return false;

		ret = deflate(&strm, Z_FINISH);
		compressedSize = compressedBufferSize - strm.avail_out;
		deflateEnd(&strm);
		if (ret != Z_STREAM_END)
			return false;

		return true;
	}

	bool decompress()
	{
		int ret;
		z_stream strm;

		strm.zalloc = Z_NULL;
		strm.zfree = Z_NULL;
		strm.opaque = Z_NULL;
		strm.avail_in = static_cast<uInt>(compressedSize);
        strm.next_in = reinterpret_cast<Bytef*>(compressedBuffer);
		strm.avail_out = static_cast<uInt>(originalSize);
		strm.next_out = reinterpret_cast<Bytef*>(decompressedBuffer);
		ret = inflateInit(&strm);
		if (ret != Z_OK)
			return false;
		
		ret = inflate(&strm, Z_FINISH);
		inflateEnd(&strm);
		if (ret != Z_STREAM_END)
			return false;

		return true;
	}
};


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
	uint64_t maxCompressedSize64 = originalSize64 + max(originalSize64 / 4, static_cast<uint64_t>(1024));
	if (maxCompressedSize64 > MAX_BUFFER_SIZE)
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

void cleanup()
{
	delete[] compressedBuffer;
	compressedBuffer = 0;

	delete[] decompressedBuffer;
	decompressedBuffer = 0;
}

template <class Codec>
bool compress(Codec& codec)
{
	cout << "Compressing with " << codec.getName() << "..." << endl;

	compressedBufferSize = codec.getMaxCompressedSize();
	compressedBuffer = new char[compressedBufferSize];

	Timer timer;
	bool ok = codec.compress();
	double compressionTime = timer.query();

	if (!ok)
	{
		cout << "ERROR: Could not compress" << endl;
		return false;
	}

	double mbps = static_cast<double>(originalSize) / MEGABYTE / compressionTime;
	double compressionRatio = static_cast<double>(compressedSize) / static_cast<double>(originalSize) * 100.0;
	cout << "Compressed in " << compressionTime << " s, " << mbps << " MB/s" << endl;
	cout << "Compressed size: " << static_cast<double>(compressedSize) / MEGABYTE << " MB (" << compressedSize / KILOBYTE << " KB)" << endl;
	cout << "Compression ratio: " << compressionRatio << "%" << endl;
	return true;
}

template <class Codec>
bool decompress(Codec& codec)
{
	cout << "Decompressing with " << codec.getName() << " (multiple times)..." << endl;

	decompressedBuffer = new char[originalSize];

	Timer timer;

	const int repeatCount = 50;
	double decompressionTime = FLT_MAX;

	for (int i = 0; i < repeatCount; ++i)
	{
		timer.reset();
		bool ok = codec.decompress();
		double currentDecompressionTime = timer.query();

		if (!ok)
		{
			cout << "ERROR: Could not decompress" << endl;
			return false;
		}

		decompressionTime = min(decompressionTime, currentDecompressionTime);
	}

	double mbps = static_cast<double>(originalSize) / MEGABYTE / decompressionTime;
	cout << "Decompression speed: " << mbps << " MB/s" << endl;

	return true;
}

bool verifyDecompressed()
{
	bool ok = true;

	for (size_t i = 0; i < originalSize; ++i)
	{
		if (decompressedBuffer[i] != originalBuffer[i])
		{
			ok = false;
			break;
		}
	}

	if (ok)
	{
		cout << "Verification successful" << endl;
		return true;
	}
	
	cout << "Verification FAILED" << endl;
	return false;
}

template <class Codec>
void benchmarkCodec(Codec& codec)
{
	bool ok = compress(codec);
	if (!ok)
	{
		cleanup();
		return;
	}

	ok = decompress(codec);
	if (!ok)
	{
		cleanup();
		return;
	}

	verifyDecompressed();
	cleanup();
}

int main(int argc, char* argv[])
{
	cout << "Doboz Data Compression Library - BENCHMARK" << endl;
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

	// Doboz
	cout << endl;
	DobozCodec dobozCodec;
	benchmarkCodec(dobozCodec);
	
	// QuickLZ
	cout << endl;
	QlzCodec qlzCodec;
	benchmarkCodec(qlzCodec);

	// zlib
	cout << endl;
	ZlibCodec zlibCodec;
	benchmarkCodec(zlibCodec);

	delete[] originalBuffer;
	return 0;
}