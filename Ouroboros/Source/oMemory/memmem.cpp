// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oCore/assert.h>
#include <oMemory/memory.h>
#include <memory.h>
#include <stdexcept>

namespace ouro {

void* memmem(void* buf, size_t buf_size, const void* find, size_t find_size)
{
	// @tony: This could be parallel-for'ed, but you'd have to check any 
	// buffer that might straddle splits, including if splits are smaller than the 
	// sizeof find (where it straddles 3 or more splits).

	oCheck(find_size >= 4, std::errc::invalid_argument, "find buffer under 4 bytes is not yet implemented");

	int8_t* oRESTRICT end = buf_size + (int8_t*)buf;
	int8_t* oRESTRICT found = (int8_t*)memchr(buf, *(const int32_t*)find, buf_size);
	while (found)
	{
		if (size_t(end - found) < find_size)
			return nullptr;

		if (!memcmp(found, find, find_size))
			return found;

		else
			found = (int8_t*)memchr(found + 4, *(const int32_t*)find, buf_size);
	}

	return found;
}

}
