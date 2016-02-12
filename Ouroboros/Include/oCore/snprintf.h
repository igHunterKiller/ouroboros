// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// snprintf without secure CRT complaints

#pragma once
#include <cstdarg>
#include <cstdio>

namespace ouro {

inline int vsnprintf(char* dst, size_t dst_size, const char* fmt, va_list args)
{
	#pragma warning(disable:4996) // secure CRT warning
	int l = ::vsnprintf(dst, dst_size, fmt, args);
	#pragma warning(default:4996)
	return l;
}
template<size_t size> int vsnprintf(char (&dst)[size], const char* fmt, va_list args) { return vsnprintf(dst, size, fmt, args); }

inline int vsnwprintf(wchar_t* dst, size_t num_chars, const wchar_t* fmt, va_list args)
{
	#pragma warning(disable:4996) // secure CRT warning
	int l = ::_vsnwprintf(dst, num_chars, fmt, args);
	#pragma warning(default:4996)
	return l;
}
template<size_t size> int vsnwprintf(wchar_t (&dst)[size], const wchar_t* fmt, va_list args) { return vsnwprintf(dst, size, fmt, args); }

inline int snprintf(char* dst, size_t dst_size, const char* fmt, ...)
{
	va_list args; va_start(args, fmt);
	int l = ouro::vsnprintf(dst, dst_size, fmt, args);
	va_end(args);
	return l;
}

template<size_t size> int snprintf(char (&dst)[size], const char* fmt, ...)
{
	va_list args; va_start(args, fmt);
	int l = ouro::vsnprintf(dst, size, fmt, args);
	va_end(args);
	return l;
}

}
