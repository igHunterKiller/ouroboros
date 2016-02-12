// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// djb2 hash

#pragma once
#include <oCore/uint128.h>

namespace ouro {

template<typename T> struct djb2_traits
{
	typedef T value_type;
	static const value_type seed;
};

template<> struct djb2_traits<uint32_t>
{
	typedef uint32_t value_type;
	static const value_type seed = 5381u;
};

template<> struct djb2_traits<unsigned long>
{
	typedef unsigned long value_type;
	static const value_type seed = 5381u;
};

template<> struct djb2_traits<uint64_t>
{
	typedef uint64_t value_type;
	static const value_type seed = 5381ull;
};

#ifdef oHAS_CONSTEXPR
	template<> struct djb2_traits<uint128>
	{
		typedef uint128 value_type;
		static constexpr uint128 seed = { 0ull, 5381ull };
	};
#endif

template<typename T>
T djb2(const void* buf, size_t buf_size, const T& seed = djb2_traits<T>::seed)
{
  // Ozan Yigit; public domain
  // http://www.cse.yorku.ca/~oz/hash.html

  const char* s = static_cast<const char*>(buf);
	djb2_traits<T>::value_type h = seed;
	while(buf_size--)
		h = (h + (h << 5)) ^ *s++;
	return h;
}

}
