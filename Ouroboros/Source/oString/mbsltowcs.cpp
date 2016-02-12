// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <codecvt>

size_t mbsltowsc(wchar_t* dst, const char* src, size_t dst_size) 
{
#ifdef _MSC_VER
#pragma warning(disable:4996) // use mbsrtowcs_s instead
#endif

	std::mbstate_t state = std::mbstate_t();
	const char* s = src;
	size_t len = 1 + std::mbsrtowcs(nullptr, &s, 0, &state);
	const size_t CopyLen = __min(len, dst_size);
	std::mbsrtowcs(dst, &s, CopyLen, &state);
	return len;

#ifdef _MSC_VER
#pragma warning(default:4996)
#endif
}
