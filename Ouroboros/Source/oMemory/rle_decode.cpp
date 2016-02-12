// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oMemory/memory.h>
#include <stdexcept>

namespace ouro {

void rle_decode(void* oRESTRICT dst, size_t dst_size, size_t element_size, const void* oRESTRICT src)
{
	int8_t* oRESTRICT d = (int8_t*)dst;
	const int8_t* oRESTRICT end = d + dst_size;
	int8_t* oRESTRICT s = (int8_t*)src;

	while (d < end)
	{
    int8_t count = *s++;
    if (count >= 0)
    {
			size_t bytes = (1 + count) * element_size;
			int8_t* oRESTRICT new_d = d + bytes;
			if (new_d > end)
				throw std::length_error("buffer overrun");
			memcpy(d, s, count);
			d = new_d;
			s += bytes;
    }
    
    else
    {
			size_t bytes = (1 - count) * element_size;
			int8_t* oRESTRICT new_d = d + bytes;
			if (new_d > end)
				throw std::length_error("buffer overrun");
			memnset(d, s, element_size, bytes);
			d = new_d;
			s += element_size;
    }
  }
}

}
