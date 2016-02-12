// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

namespace ouro {

char* ellipsize(char* dst, size_t dst_size)
{
	switch (dst_size - 4)
	{ // Duff's device (no breaks)
		default: dst[dst_size-4] = '.';
		case 3: dst[dst_size-3] = '.';
		case 2: dst[dst_size-2] = '.';
		case 1: dst[dst_size-1] = 0;
		case 0: break;
	}

	return dst;
}

}
