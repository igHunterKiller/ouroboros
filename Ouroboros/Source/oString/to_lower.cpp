// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <string.h>

namespace ouro {

wchar_t to_lower(wchar_t wc)
{
	wchar_t s[2] = { wc, 0 };
	_wcslwr_s(s);
	return s[0];
}

}
