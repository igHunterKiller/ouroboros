// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <stdexcept>
#include <cstdint>

namespace ouro {

void memset8(void* dst, int64_t value, size_t bytes);
void memset2d8(void* dst, size_t row_pitch, int64_t value, size_t set_pitch, size_t num_rows)
{
	if (set_pitch & 7)
		throw std::invalid_argument("set_pitch must be aligned to 8 bytes");
	int8_t* d = (int8_t*)dst;
	for (const int8_t* end = row_pitch * num_rows + d; d < end; d += row_pitch)
		memset8(d, value, set_pitch);
}

}
