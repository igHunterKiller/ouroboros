// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// efficient bit manipulation using intrinsics where possible

#pragma once
//#include <oArch/arch.h>
#include <oArch/intrin.h>
#include <cstdint>

inline bool ispow2(uint32_t x)
{
	return x ? ((x & (x-1)) == 0) : false;
}

inline bool ispow2(uint64_t x)
{
	return x ? ((x & (x-1)) == 0) : false;
}

inline uint32_t log2i(uint32_t x)
{
#if defined(_M_X64) || defined(_M_IX86)
	unsigned long index;
	return _BitScanReverse(&index, x) == 0 ? -1 : index;
#else
	uint32_t r, s;
	r = (x > 0xFFFF) << 4; x >>= r;
	s = (x > 0xFF  ) << 3; x >>= s; r |= s;
	s = (x > 0xF   ) << 2; x >>= s; r |= s;
	s = (x > 0x3   ) << 1; x >>= s; r |= s;
                                  r |= (x >> 1);
	return r;
#endif
}

inline uint32_t log2i(uint64_t x)
{
#if defined(_M_X64) || defined(_M_IX86)
	unsigned long index;
	return _BitScanReverse64(&index, x) == 0 ? -1 : index;
#else
  x |= (x >> 1);
  x |= (x >> 2);
  x |= (x >> 4);
  x |= (x >> 8);
  x |= (x >> 16);
  x |= (x >> 32);
  x >>= 1;
  x -= (x >> 1) & 0x5555555555555555ull;
  x = ((x >> 2) & 0x3333333333333333ull) + (x & 0x3333333333333333ull);
  x = ((x >> 4) + x) & 0x0f0f0f0f0f0f0f0full;
  x += (x >> 8);
  x += (x >> 16);
  x += (x >> 32);
	return (uint32_t)x & 0x3f;
#endif
}

inline uint32_t prevpow2(uint32_t x)
{
	return 1 << log2i(x);
}

inline uint64_t prevpow2(uint64_t x)
{
	return 1ull << log2i(x);
}

inline uint32_t nextpow2(uint32_t x)
{
	// http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
	x--; 
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	return ++x;
}

inline uint64_t nextpow2(uint64_t x)
{
	// http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
	x--; 
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	x |= x >> 32;
	return ++x;
}

inline uint32_t bitcount(uint32_t x)
{
#if defined(_M_X64) || defined(_M_IX86)
  return __popcnt(x);
#else
	// http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
	x = x - ((x >> 1) & 0x55555555);
	x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
	return ((x + (x >> 4) & 0xF0F0F0F) * 0x1010101) >> 24;
#endif
}

inline uint32_t bitcount(unsigned long x)
{
	return bitcount((uint32_t)x);
}

inline uint32_t bitcount(uint64_t x)
{
#ifdef _M_X64
  return (uint32_t)__popcnt64(x);
#else
	// http://stackoverflow.com/questions/2709430/count-number-of-bits-in-a-64-bit-long-big-integer
	x = x - ((x >> 1) & 0x5555555555555555); 
	x = (x & 0x3333333333333333) + ((x >> 2) & 0x3333333333333333); 
	return uint32_t((((x + (x >> 4)) & 0xF0F0F0F0F0F0F0F) * 0x101010101010101) >> 56); 
#endif
}

inline uint32_t bitreverse(uint32_t x)
{
	// http://graphics.stanford.edu/~seander/bithacks.html#ReverseParallel
	x = ((x >>  1) & 0x55555555) | ((x & 0x55555555) <<   1); // swap odd and even bits
	x = ((x >>  2) & 0x33333333) | ((x & 0x33333333) <<   2); // swap consecutive pairs
	x = ((x >>  4) & 0x0F0F0F0F) | ((x & 0x0F0F0F0F) <<   4); // swap nibbles ... 
	x = ((x >>  8) & 0x00FF00FF) | ((x & 0x00FF00FF) <<   8); // swap bytes
	x = ((x >> 16)             ) | ((x             )  << 16); // swap 2-byte long pairs
	return x;
}

inline uint64_t bitreverse(uint64_t x)
{
	// http://graphics.stanford.edu/~seander/bithacks.html#ReverseParallel
	x = ((x >>  1) & 0x5555555555555555ull) | ((x & 0x5555555555555555ull) <<  1); // swap odd and even bits
	x = ((x >>  2) & 0x3333333333333333ull) | ((x & 0x3333333333333333ull) <<  2); // swap consecutive pairs
	x = ((x >>  4) & 0x0F0F0F0F0F0F0F0Full) | ((x & 0x0F0F0F0F0F0F0F0Full) <<  4); // swap nibbles ... 
	x = ((x >>  8) & 0x00FF00FF00FF00FFull) | ((x & 0x00FF00FF00FF00FFull) <<  8); // swap bytes
	x = ((x >> 16) & 0x0000FFFF0000FFFFull) | ((x & 0x0000FFFF0000FFFFull) << 16); // swap 2-byte long pairs
	x = ((x >> 32)                        ) | ((x                        ) << 32); // swap 4-byte long pairs
  return x;
}

inline int32_t bithigh(uint32_t x)
{
	unsigned long index;
	return _BitScanReverse(&index, x) == 0 ? -1 : index;
}

inline int32_t bitlow(uint32_t x)
{
	unsigned long index;
	return _BitScanForward(&index, x) == 0 ? -1 : index;
}

#ifdef _M_X64
	inline int32_t bithigh(uint64_t x)
	{
		unsigned long index;
		return _BitScanReverse64(&index, x) == 0 ? -1 : index;
	}

	inline int32_t bitlow(uint64_t x)
	{
		unsigned long index;
		return _BitScanForward64(&index, x) == 0 ? -1 : index;
	}

#else
	inline int32_t bithigh(uint64_t x)
	{
		unsigned long index;
		if (_BitScanReverse(&index, (x>>32)) == 0)
		{
			if (_BitScanReverse(&index, static_cast<unsigned long>(x)) == 0)
				return -1;
		}
		else
			index += 32;
		return index;
	}

	inline int32_t bitlow(uint64_t x)
	{
		unsigned long index;
		if (_BitScanForward(&index, static_cast<unsigned long>(x)) == 0)
		{
			if (_BitScanForward(&index, (x>>32)) == 0)
				return -1;
			else
				index += 32;
		}
		return index;
	}
#endif
