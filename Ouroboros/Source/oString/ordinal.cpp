// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oString/string.h>

namespace ouro {

const char* ordinal(size_t number)
{
	// In the teens, all numbers are suffixed with 'th', else be more selective
	const int n = number % 100;
	if (n < 11 || n > 13)
		switch (n % 10)
		{
			case 1: return "st";
			case 2: return "nd";
			case 3: return "rd";
			default: break;
		}
	return "th";
}

}
