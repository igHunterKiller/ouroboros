// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <ctype.h>

namespace ouro {

bool is_sep(wchar_t c) { return c == L'/' || c == L'\\'; }

size_t wcmnroot(const wchar_t* path1, const wchar_t* path2)
{
	size_t last_sep = 0;
	size_t len = 0;
	if (path1 && *path1 && path2 && *path2)
	{
		while (path1[len] && path2[len])
		{
			wchar_t a = towlower(path1[len]);
			wchar_t b = towlower(path2[len]);

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
