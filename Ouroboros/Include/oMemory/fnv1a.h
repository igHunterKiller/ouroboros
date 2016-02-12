// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// fnva1 hash

#pragma once
#include <oCore/uint128.h>
#include <cstdint>
#include <ctype.h>

namespace ouro {

template<typename T> struct fnv1a_traits
{
	typedef T value_type;
	static const value_type seed;
	static const value_type value;
};

template<> struct fnv1a_traits<uint32_t>
{
	typedef uint32_t value_type;
	static const uint32_t seed = 2166136261u;
	static const uint32_t value = 16777619u;
};

template<> struct fnv1a_traits<unsigned long>
{
	typedef unsigned long value_type;
	static const unsigned long seed = 2166136261u;
	static const unsigned long value = 16777619u;
};

template<> struct fnv1a_traits<uint64_t>
{
	typedef uint64_t value_type;
	static const uint64_t seed = 14695981039346656037ull;
	static const uint64_t value = 1099511628211ull;
};

#ifdef oHAS_CONSTEXPR
	template<> struct fnv1a_traits<uint128_t>
	{
		static constexpr uint128_t seed = { 0x6c62272e07bb0142ull, 0x62b821756295c592ull }; // 144066263297769815596495629667062367629
		static constexpr uint128_t value = { 0x0000000001000000ull, 0x000000000000013bull }; // 309485009821345068724781371
	};
#endif

template<typename T>		
inline T fnv1a(const void* buf, size_t buf_size, const T& seed = fnv1a_traits<T>::seed)
{
	fnv1a_traits<T>::value_type h = seed;
	const char* s = static_cast<const char*>(buf);
	while (buf_size--)
	{
		h ^= *s++;
		h *= fnv1a_traits<T>::value;
	}
	return h;
}

template<typename T>		
inline T fnv1a(const char* str, const T& seed = fnv1a_traits<T>::seed)
{
	fnv1a_traits<T>::value_type h = seed;
	while (*str)
	{
		h ^= *str++;
		h *= fnv1a_traits<T>::value;
	}
	return h;
}

template<typename T>		
inline T fnv1ai(const char* str, const T& seed = fnv1a_traits<T>::seed)
{
	fnv1a_traits<T>::value_type h = seed;
	while (*str)
	{
		char c = *str++;
		h ^= tolower(c);
		h *= fnv1a_traits<T>::value;
	}
	return h;
}

namespace detail { union wchar_bytes { wchar_t wc; char c[2]; }; }

template<typename T>		
inline T fnv1a(const wchar_t* str, const T& seed = fnv1a_traits<T>::seed)
{
	fnv1a_traits<T>::value_type h = seed;
	while (*str)
	{
		detail::wchar_bytes ch;
		ch.wc = *str++;
		h ^= ch.c[0];
		h *= fnv1a_traits<T>::value;
		h ^= ch.c[1];
		h *= fnv1a_traits<T>::value;
	}
	return h;
}

template<typename T>		
inline T fnv1ai(const wchar_t* str, const T& seed = fnv1a_traits<T>::seed)
{
	fnv1a_traits<T>::value_type h = seed;
	while (*str)
	{
		detail::wchar_bytes ch;

#ifdef _MSC_VER
		wchar_t s[2] = { *str, 0 };
		_wcslwr_s(s);
		ch.wc = s[0];
		str++;
#else
#error unsupported platform: tolower(wchar_t)
#endif

		h ^= ch.c[0];
		h *= fnv1a_traits<T>::value;
		h ^= ch.c[1];
		h *= fnv1a_traits<T>::value;
	}
	return h;
}

// npot bitcount hashes: http://www.isthe.com/chongo/tech/comp/fnv/
// (use next-higher type and xor reduce)
template<typename T, typename U>
T fnv1a_reduced(const U& fnv1a_hash, int num_bits) { return static_cast<T>((fnv1a_hash >> num_bits) ^ (U(1) << num_bits)); }

}
