// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include "memduff.h"
#include <oCore/byte.h>

namespace ouro {

bool memcmp4(const void* mem, long value, size_t bytes)
{
	// Compares a run of memory against a constant value.
	// this compares a full int value rather than a char value.

	// First move mem up to long alignment

	const int32_t* body;
	const int8_t* prefix, *postfix;
	size_t prefix_nbytes, postfix_nbytes;
	detail::init_duffs_device_pointers_const(mem, bytes, &prefix, &prefix_nbytes, &body, &postfix, &postfix_nbytes);

	byte_swizzle32 s;
	s.asint32 = value;

	// Duff's device up to alignment: http://en.wikipedia.org/wiki/Duff's_device
	switch (prefix_nbytes)
	{
		case 3: if (*prefix++ != s.asint8[3]) return false;
		case 2: if (*prefix++ != s.asint8[2]) return false;
		case 1: if (*prefix++ != s.asint8[1]) return false;
		default: break;
	}

	// Do aligned assignment
	while (body < (int32_t*)postfix)
		if (*body++ != value)
			return false;

	// Duff's device final bytes: http://en.wikipedia.org/wiki/Duff's_device
	switch (postfix_nbytes)
	{
		case 3: if (*postfix++ != s.asint8[3]) return false;
		case 2: if (*postfix++ != s.asint8[2]) return false;
		case 1: if (*postfix++ != s.asint8[1]) return false;
		default: break;
	}

	return true;
}

}
