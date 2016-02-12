// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oString/string.h>

namespace ouro {

	char* relativize_path(char* dst, size_t dst_size, const char* base_path, const char* full_path)
	{
		// note: might needs traits for sep, .. or ../

		// advance till we find a segment that doesn't match
		const char* fullpath = full_path;
		const char* basepath = base_path;
		const char* fullslash = fullpath;
		const char* baseslash = basepath;

		while ((*fullpath == *basepath) && *fullpath)
		{
			if (*fullpath == '/')
			{
				fullslash = fullpath;
				baseslash = basepath;
			}
			fullpath++;
			basepath++;
		}

		if (fullslash == fullpath) // nothing is relative
			return nullptr;

		// Decide how many ../ segments are needed
		size_t segments = 0;
		baseslash++;
		while (*baseslash != 0)
		{
			if (*baseslash == '/')
				segments++;
			baseslash++;
		}
		fullslash++;

		if (dst_size < (segments * 3 + 1)) // strlen("../") + nul
			return nullptr;

		char* d = dst;
		size_t size = dst_size;
		for (size_t i = 0; i < segments; i++)
		{
			d += strlcpy(d, "../", size);
			size -= 3;
		}

		return strlcpy(d, fullslash, size) < dst_size ? dst : nullptr;
 	}

}
