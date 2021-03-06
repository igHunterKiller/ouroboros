// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oCore/assert.h>
#include <stdexcept>
#include <cstdint>

namespace ouro {

void memset4(void* dst, int32_t value, size_t bytes);
void memset2d4(void* dst, size_t row_pitch, int32_t value, size_t set_pitch, size_t num_rows)
{
	oCheck((set_pitch & 3) == 0, std::errc::invalid_argument, "set_pitch must be 4-byte aligned");
	int8_t* d = (int8_t*)dst;
	for (const int8_t* end = row_pitch * num_rows + d; d < end; d += row_pitch)
		memset4(d, value, set_pitch);
}

}
