// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oString/string.h>

namespace ouro {

char* strbitmask(char* dst, size_t dst_size, int flags, const char* all_zeros_value, asstr_fn as_string)
{
	if (flags)
	{
		*dst = 0;
		unsigned int Mask = 0x80000000;
		while (Mask)
		{
			int v = flags & Mask;
			if (v)
				sncatf(dst, dst_size, "%s%s", *dst == 0 ? "" : "|", as_string(v));
			Mask >>= 1;
		}
	}

	else if (strlcpy(dst, all_zeros_value, dst_size) >= dst_size)
		return nullptr;
	
	return dst;
}

}
