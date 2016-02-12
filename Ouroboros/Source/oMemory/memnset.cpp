// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oCore/byte.h>
#include <oMemory/memory.h>

namespace ouro {

void memnset(void* oRESTRICT dst, const void* oRESTRICT src, size_t src_size, size_t copy_size)
{
	size_t alignedSize = copy_size / src_size;
	size_t unalignedSize = copy_size - (alignedSize * src_size);

	switch (src_size)
	{
		case 8: memset8(dst, *(const int64_t*)src, copy_size); break;
		case 4: memset4(dst, *(const int32_t*)src, copy_size); break;
		case 2: memset2(dst, *(const int16_t*)src, copy_size); break;
		case 1: memset(dst, *(const int8_t*)src, copy_size); break;
		default:
		{
			if (src_size & 0xf)
			{
				{
					int64_t* oRESTRICT d = (int64_t*)dst;
					const int64_t* oRESTRICT s = (int64_t*)src;

					while (alignedSize--)
					{
						*d++ = *s++;
						*d++ = *s++;
					}
				}

				int8_t* oRESTRICT d = (int8_t*)dst;
				const int8_t* oRESTRICT s = (int8_t*)src;
				while (unalignedSize)
					*d++ = *s++;
			}

			else
			{
				int8_t* oRESTRICT d = (int8_t*)dst;
				while (copy_size >= src_size)
				{
					memcpy(d, src, src_size);
					d += src_size;
					copy_size -= src_size;
				}

				if (copy_size)
					memcpy(d, src, copy_size);
			}
			break;
		}
	}
}

}
