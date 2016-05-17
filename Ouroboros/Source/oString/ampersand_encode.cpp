// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

//#include <oArch/arch.h>
#include <oString/string.h>
#include <iterator>

namespace ouro {

char* ampersand_encode(char* oRESTRICT dst, size_t dst_size, const char* oRESTRICT src)
{
	*dst = 0;
	char* d = dst;
	char* end = d + dst_size - 1;
	*(end-1) = 0; // don't allow read outside the buffer even in failure cases

	const char* xml_reserved_chars = "<>&'\"";
	struct SYM { const char* res; unsigned char len; };
	static SYM reserved[] = { { "&lt;", 4 }, { "&gt;", 4 }, { "&amp;", 5 }, { "&apos;", 6 }, { "&quot;", 6 }, };

	const char* s = src;
	while (*s)
	{
		const char* pos = strchr(xml_reserved_chars, *s);
		if (pos)
		{
			size_t reserved_idx = std::distance(xml_reserved_chars, pos);

			if ((d+reserved[reserved_idx].len) > end)
				return nullptr;

			snprintf(d, std::distance(d, end), "%s", reserved[reserved_idx].res);
			d += reserved[reserved_idx].len;
			s++;
		}

		else if (d >= end)
			return nullptr;
		else
			*d++ = *s++;
	}

	*d = 0;
	return dst;
}

}
