// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oCore/restrict.h>
#include <cstring>

namespace ouro {

char* ampersand_decode(char* oRESTRICT dst, size_t dst_size, const char* oRESTRICT src)
{
	if (!dst || !dst_size)
		return nullptr;

	char* d = dst;
	char* end = d + dst_size - 1;
	*(end-1) = 0; // don't allow read outside the buffer even in failure cases

	struct SYM { const char* res; unsigned char len; char c; };
	static SYM reserved[] = { { "&lt;", 4, '<' }, { "&gt;", 4, '>' }, { "&amp;", 5, '&' }, { "&apos;", 6, '\'' }, { "&quot;", 6, '\"' }, };

	const char* s = src;
	while (*s)
	{
		if (d >= end)
			return nullptr;

		else if (*s == '&')
		{
			bool found = false;
			for (const auto& sym : reserved)
			{
				if (0 == strcmp(sym.res, s))
				{
					*d++ = sym.c;
					s += sym.len;
					found = true;
					break;
				}
			}

			if (!found)
				*d++ = *s++;
		}

		else
			*d++ = *s++;
	}

	*d = 0;
	return dst;
}

}
