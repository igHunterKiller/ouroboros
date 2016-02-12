// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <codecvt>

size_t wcsltombs(char* dst, const wchar_t* src, size_t dst_size)
{
#ifdef _MSC_VER
#pragma warning(disable:4996) // use wcsrtombs_s instead
#endif

	std::mbstate_t state = std::mbstate_t();
	const wchar_t* s = src;
	size_t len = 1 + std::wcsrtombs(nullptr, &s, 0, &state);
	const size_t CopyLen = __min(len, dst_size);
	std::wcsrtombs(dst, &s, CopyLen, &state);
	return len;

#ifdef _MSC_VER
#pragma warning(default:4996) // use wcsrtombs_s instead
#endif
}
