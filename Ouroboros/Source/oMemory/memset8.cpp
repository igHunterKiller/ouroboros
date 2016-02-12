// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include "memduff.h"
#include <oCore/byte.h>

namespace ouro {

void memset8(void* dst, int64_t value, size_t bytes)
{
	// Probably slower than stdc's memset but sets a full int64_t at a time.

	// First move dst up to alignment
	int64_t* body;
	int8_t* prefix, *postfix;
	size_t prefix_nbytes, postfix_nbytes;
	detail::init_duffs_device_pointers(dst, bytes, &prefix, &prefix_nbytes, &body, &postfix, &postfix_nbytes);

	byte_swizzle64 s;
	s.asint64 = value;

	// Duff's device up to alignment: http://en.wikipedia.org/wiki/Duff's_device
	switch (prefix_nbytes)
	{
		case 7: *prefix++ = s.asint8[7];
		case 6: *prefix++ = s.asint8[6];
		case 5: *prefix++ = s.asint8[5];
		case 4: *prefix++ = s.asint8[4];
		case 3: *prefix++ = s.asint8[3];
		case 2: *prefix++ = s.asint8[2];
		case 1: *prefix++ = s.asint8[1];
		default: break;
	}

	// Do aligned assignment
	while (body < (int64_t*)postfix)
		*body++ = value;

	// Duff's device final bytes: http://en.wikipedia.org/wiki/Duff's_device
	switch (postfix_nbytes)
	{
		case 7: *postfix++ = s.asint8[7];
		case 6: *postfix++ = s.asint8[6];
		case 5: *postfix++ = s.asint8[5];
		case 4: *postfix++ = s.asint8[4];
		case 3: *postfix++ = s.asint8[3];
		case 2: *postfix++ = s.asint8[2];
		case 1: *postfix++ = s.asint8[1];
		default: break;
	}
}

}
