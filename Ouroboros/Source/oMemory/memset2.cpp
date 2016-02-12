// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include "memduff.h"
#include <oCore/byte.h>

namespace ouro {

void memset2(void* dst, int16_t value, size_t bytes)
{
	// Sets an int value at a time. This is probably slower than c's memset, but 
	// this sets a full int value rather than a char value.

	// First move dst up to alignment
	int16_t* body;
	int8_t* prefix, *postfix;
	size_t prefix_nbytes, postfix_nbytes;
	detail::init_duffs_device_pointers(dst, bytes, &prefix, &prefix_nbytes, &body, &postfix, &postfix_nbytes);

	byte_swizzle16 s;
	s.asint16 = value;

	// Duff's device up to alignment: http://en.wikipedia.org/wiki/Duff's_device
	switch (prefix_nbytes)
	{
		case 1: *prefix++ = s.asint8[1];
		default: break;
	}

	// Do aligned assignment
	while (body < (short*)postfix)
		*body++ = value;

	// Duff's device final bytes: http://en.wikipedia.org/wiki/Duff's_device
	switch (postfix_nbytes)
	{
		case 1: *postfix++ = s.asint8[1];
		default: break;
	}
}

}
