// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oArch/compiler.h>
#include <oString/string.h>
#include <system_error>

namespace ouro {

char* percent_encode(char* oRESTRICT dst, size_t dst_size, const char* oRESTRICT src, const char* oRESTRICT reserved_chars)
{
	*dst = 0;
	char* d = dst;
	char* end = d + dst_size - 1;
	*(end-1) = 0; // don't allow read outside the buffer even in failure cases

	const char* s = src;
	while (*s)
	{
		if ((*s & 0x80) != 0)
		{
			// TODO: Support UTF-8 
			throw std::system_error(std::errc::function_not_supported, std::system_category(), "UTF-8 not yet supported");
		}

		if (strchr(reserved_chars, *s))
		{
			if ((d+3) > end)
				throw std::system_error(std::errc::no_buffer_space, std::system_category());
			*d++ = '%';
			snprintf(d, std::distance(d, end), "%02x", *s++); // use lower-case escaping http://www.textuality.com/tag/uri-comp-2.html
			d += 2;
		}

		else if (d >= end)
			throw std::system_error(std::errc::no_buffer_space, std::system_category());
		else
			*d++ = *s++;
	}

	*d = 0;
	return dst;
}

}
