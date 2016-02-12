// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Thomas Wang's integer hash

#pragma once
#include <cstdint>

namespace ouro {

inline uint32_t wang_hash(uint32_t x)
{
	x = (x ^ 61) ^ (x >> 16);
	x = x + (x << 3);
	x = x ^ (x >> 4);
	x = x * 0x27d4eb2d;
	x = x ^ (x >> 15);
	return x;
}

inline uint64_t wang_hash(uint64_t x)
{
	x = (~x) + (x << 21); // x = (x << 21) - x - 1;
	x = x ^ (x >> 24);
	x = (x + (x << 3)) + (x << 8); // x * 265
	x = x ^ (x >> 14);
	x = (x + (x << 2)) + (x << 4); // x * 21
	x = x ^ (x >> 28);
	x = x + (x << 31);
	return x;
}

}
