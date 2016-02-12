// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <string.h>

namespace ouro {

char* clean_whitespace(char* dst, size_t dst_size, const char* src, char replacement, const char* _ToPrune)
{
	char* w = dst;
	const char* r = src;
	dst_size--;
	while (static_cast<size_t>(w - dst) < dst_size && *r)
	{
		if (strchr(_ToPrune, *r))
		{
			*w++ = replacement;
			r += strspn(r, _ToPrune);
		}

		else
			*w++ = *r++;
	}

	*w = 0;

	return dst;
}

}
