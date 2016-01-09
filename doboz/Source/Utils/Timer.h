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

#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#else
#include <ctime>
#endif

#include <cassert>

namespace afra {

class Timer
{
public:
	Timer()
	{
#ifdef _WIN32
		LARGE_INTEGER frequency;

		BOOL result = QueryPerformanceFrequency(&frequency);
		assert(result != 0 && "Timer is not supported.");
		
		invCountsPerSecond_ = 1.0 / static_cast<double>(frequency.QuadPart);
#endif

		reset();
	}

	void reset()
	{
#ifdef _WIN32
		BOOL result = QueryPerformanceCounter(&startCount_);
		assert(result != 0 && "Could not query counter.");
		
#else
		int result = clock_gettime(CLOCK_MONOTONIC, &startTime_);
		assert(result == 0 && "Could not get time.");
#endif
	}

	double query() const
	{
#ifdef _WIN32
		LARGE_INTEGER currentCount;

		BOOL result = QueryPerformanceCounter(&currentCount);
		assert(result != 0 && "Could not query counter.");
		
		return (currentCount.QuadPart - startCount_.QuadPart) * invCountsPerSecond_;
#else
		timespec currentTime;

		int result = clock_gettime(CLOCK_MONOTONIC, &currentTime);
		assert(result == 0 && "Could not get time.");
		
		return static_cast<double>(currentTime.tv_sec - startTime_.tv_sec) + (currentTime.tv_nsec - startTime_.tv_nsec) * 1e-9;
#endif
	}

private:
#ifdef _WIN32
	double        invCountsPerSecond_;
	LARGE_INTEGER startCount_;
#else
	timespec      startTime_;
#endif
};

} // namespace afra