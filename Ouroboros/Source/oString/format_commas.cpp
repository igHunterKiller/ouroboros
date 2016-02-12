// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <stdlib.h>
#include <oString/string.h>

namespace ouro {

char* format_commas(char* dst, size_t dst_size, unsigned int number)
{
	#ifdef _MSC_VER
		_itoa_s(number, dst, dst_size, 10);
	#else
		itoa(number, dst, 10);
	#endif
	size_t len = strlen(dst);

	size_t w = len % 3;
	if (!w)
		w = 3;

	while (w < len)
	{
		if (!insert(dst, dst_size, dst + w, 0, ","))
			return nullptr;
		w += 4;
	}

	return dst;
}

char* format_commas(char* dst, size_t dst_size, int number)
{
	if (number < 0)
	{
		if (dst_size < 1)
			return nullptr;

		number = -number;
		*dst++ = '-';
		dst_size--;
	}

	return format_commas(dst, dst_size, static_cast<unsigned int>(number));
}

}
