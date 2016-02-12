// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <string.h>

namespace ouro {

const char* strnstr(const char* str, const char* substring, size_t len)
{
	char c, sc;
	size_t sub_len;

	if ((c = *substring++) != '\0')
	{
		sub_len = strlen(substring);
		do
		{
			do
			{
				if ((sc = *str++) == '\0' || len-- < 1)
					return nullptr;
			} while (sc != c);

			if (sub_len > len)
				return nullptr;
		} while (strncmp(str, substring, sub_len) != 0);
		str--;
	}

	return (const char *)str;
}

char* strnstr(char* str, const char* substring, size_t len)
{
	return const_cast<char*>(strnstr(static_cast<const char*>(str), substring, len));
}

}
