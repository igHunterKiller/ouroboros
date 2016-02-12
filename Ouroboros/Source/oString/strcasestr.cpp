// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <string.h>
#include <ctype.h>

namespace ouro {

const char* strcasestr(const char* str, const char* substring)
{
	char c, sc;
	size_t len;

	if ((c = *substring++) != 0)
	{
		c = (char)tolower(c);
		len = strlen(substring);
		do
		{
			do
			{
				if ((sc = *str++) == 0)
					return nullptr;
			} while ((char)tolower(sc) != c);
		} while (_strnicmp(str, substring, len) != 0);
		str--;
	}
	return ((const char *)str);
}

char* strcasestr(char* str, const char* substring)
{
	return const_cast<char*>(strcasestr(static_cast<const char*>(str), substring));
}

}
