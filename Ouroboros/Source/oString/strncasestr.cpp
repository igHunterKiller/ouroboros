// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <string.h>

namespace ouro {

const char* strncasestr(const char* str, const char* find, size_t len)
{
	char c, sc;
	size_t sub_len;
	
	if ((c = *find++) != '\0')
	{
		sub_len = strlen(find);
		do
		{
			do
			{
				if (len-- < 1 || (sc = *str++) == '\0')
					return nullptr;
			} while (sc != c);

			if (sub_len > len)
				return nullptr;
		} while (_strnicmp(str, find, sub_len) != 0);
		str--;
	}
	
	return ((char *)str);
}

char* strncasestr(char* str, const char* substring, size_t sub_len)
{
	return const_cast<char*>(strncasestr(static_cast<const char*>(str), substring, sub_len));
}

}
