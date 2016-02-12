// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oString/string.h>
#include <memory.h>

namespace ouro {

char* trim_left(char* trimmed, size_t trimmed_size, const char* src, const char* to_trim)
{
	src += strspn(src, to_trim);
	strlcpy(trimmed, src, trimmed_size);
	return trimmed;
}

char* trim_right(char* trimmed, size_t trimmed_size, const char* src, const char* to_trim)
{
	const char* end = &src[strlen(src)-1];
	while (strchr(to_trim, *end) && end > src)
		end--;

	strncpy_s(trimmed, trimmed_size, src, end+1-src);
	return trimmed;
}

}
