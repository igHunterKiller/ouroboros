// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oString/string.h>
#include <cassert>
#include <functional>

namespace ouro {

char* search_path(char* dst
	, size_t dst_size
	, const char* search_paths
	, const char* relative_path
	, const char* dot_path
	, const std::function<bool(const char* _Path)>& path_exists)
{
	if (!dst || !search_paths) return nullptr;
	const char* cur = search_paths;

	while (*cur)
	{
		cur += strspn(cur, oWHITESPACE);
		if (*cur == ';')
		{
			cur++;
			continue;
		}

		char* d = dst;
		char* end = d + dst_size - 1;
		if (*cur == '.' && dot_path && *dot_path)
		{
			size_t len = strlcpy(dst, dot_path, dst_size);
			if (len >= dst_size)
				return nullptr;
			if (dst[len-1] != '/' && dst[len-1] != '\\')
			{
				if ((len+1) >= dst_size)
					return nullptr;
				dst[len-1] = '/';
				dst[len++] = '\0';
			}
				
			d = dst + len;
		}

		while (d < end && *cur && *cur != ';')
			*d++ = *cur++;
		while (isspace(*(--d))); // empty
		if (*d == '/' || *d == '\\')
			*(++d) = '/';
		assert(d < end);
		*(++d) = 0;
		if (strlcat(dst, relative_path, dst_size) >= dst_size)
			return nullptr;
		if (path_exists(dst))
			return dst;
		if (*cur == 0)
			break;
		cur++;
	}

	*dst = 0;
	return nullptr;
}

}
