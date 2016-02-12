// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oString/string.h>
#include <memory.h>

namespace ouro {

size_t rstrcspn(const char* buffer_start, const char* str1, const char* str2)
{
	size_t n = 0;
	while (str1 >= buffer_start)
	{
		if (strchr(str2, *str1))
			return n - 1;
		n++;
		str1--;
	}
	return n - 1;
}

size_t rstrcspn(char* buffer_start, char* str1, const char* str2)
{
	return rstrcspn(static_cast<const char*>(buffer_start), static_cast<const char*>(str1), str2);
}

}
