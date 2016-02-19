// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oCore/assert.h>
#include <stdexcept>
#include <cstdint>

namespace ouro {

void memset2(void* dst, int16_t value, size_t bytes);
void memset2d2(void* dst, size_t row_pitch, int16_t value, size_t set_pitch, size_t num_rows)
{
	oCheck((set_pitch & 1) == 0, std::errc::invalid_argument, "set_pitch must be 2-byte aligned");
	int8_t* d = (int8_t*)dst;
	for (const int8_t* end = row_pitch * num_rows + d; d < end; d += row_pitch)
		memset2(d, value, set_pitch);
}

}
