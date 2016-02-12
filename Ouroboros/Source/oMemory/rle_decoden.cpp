// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oMemory/memory.h>

namespace ouro {

const void* rle_decoden(void* oRESTRICT dst, size_t dst_size, 
	size_t element_stride, size_t rle_element_size, const void* oRESTRICT src)
{
	int8_t* d = (int8_t*)dst;
	const int8_t* oRESTRICT end = d + dst_size;
	int8_t* oRESTRICT s = (int8_t*)src;
	size_t dstep = element_stride - rle_element_size;

	while (d < end)
	{
		int8_t count = *s++;
		if (count >= 0)
		{
			count = 1 + count;
			while (count--)
			{
				size_t bytes = rle_element_size;
				while (bytes--)
					*d++ = *s++;
				d += dstep;
			}
		}

		else
		{
			count = 1 - count;
			while (count--)
			{
				for (size_t byte = 0; byte < rle_element_size; byte++)
					*d++ = *(s + byte);
				d += element_stride;
			}

			s += rle_element_size;
		}
	}

	return s;
}

}
