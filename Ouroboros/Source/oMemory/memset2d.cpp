// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oCore/byte.h>
#include <memory.h>

namespace ouro {

void memset2d(void* dst, size_t row_pitch, int value, size_t set_pitch, size_t num_rows)
{
	int8_t* d = (int8_t*)dst;
	for (const int8_t* end = row_pitch * num_rows + d; d < end; d += row_pitch)
		memset(d, value, set_pitch);
}

}
