// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <string.h>

namespace ouro {

const char* rstrstr(const char* str, const char* substring)
{
	if (*substring == '\0')
		return nullptr;

	const char* found = nullptr, *s = nullptr;
	do
	{
		found = s;
		s = strstr(str, substring);
		str = s + 1;
	} while (s);

	return found;
}

char* rstrstr(char* str, const char* substring)
{
	return const_cast<char*>(rstrstr(static_cast<const char*>(str), substring));
}

}
