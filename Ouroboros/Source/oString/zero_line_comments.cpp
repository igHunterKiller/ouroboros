// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oArch/compiler.h>
#include <string.h>

namespace ouro {

char* zero_line_comments(char* oRESTRICT str, const char* oRESTRICT comment_prefix, char replacement)
{
	if (str)
	{
		size_t l = strlen(comment_prefix);
		while (*str)
		{
			if (!memcmp(comment_prefix, str, l))
				while (*str && *str != '\n')
					*str++ = replacement;
			str++;
		}
	}

	return str;
}

}
