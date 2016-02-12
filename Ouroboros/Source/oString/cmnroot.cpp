// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <ctype.h>

namespace ouro {

bool is_sep(char c) { return c == '/' || c == '\\'; }

size_t cmnroot(const char* path1, const char* path2)
{
	size_t last_sep = 0;
	size_t len = 0;
	if (path1 && *path1 && path2 && *path2)
	{
		while (path1[len] && path2[len])
		{
			char a = (char)tolower(path1[len]);
			char b = (char)tolower(path2[len]);

			if (a != b || is_sep(a) != is_sep(b))
				break;

			if (is_sep(a) || is_sep(b))
				last_sep = len;
			
			len++;
		}
	}

	return last_sep;
}

}
