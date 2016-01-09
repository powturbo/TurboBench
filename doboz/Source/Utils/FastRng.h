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

#include <stdint.h>

namespace afra {

class FastRng
{
public:
	FastRng(uint32_t seed = 1) : value_(seed) {}

	uint32_t getUint()
	{
		next();
		return value_;
	}

	int32_t getInt()
	{
		next();
		return static_cast<int32_t>(value_);
	}

	float getFloat()
	{
		const uint32_t signExponent = 0x3f800000;
		const uint32_t mask = 0x007fffff;
		const float offset = 1.0f;

		next();

		union
		{
			uint32_t ui;
			float f;
		}
		result;

		result.ui = (value_ & mask) | signExponent;
		return result.f - offset;
	}

private:
	uint32_t value_;

	void next()
	{
		const uint32_t multiplier = 1664525;
		const uint32_t increment = 1013904223;
		value_ = multiplier * value_ + increment;
	}
};

} // namespace afra
