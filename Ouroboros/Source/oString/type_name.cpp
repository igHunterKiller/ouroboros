// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oString/string.h>

namespace ouro {

const char* type_name(const char* typeinfo_name)
{
	static struct { const char* s; size_t len; } sPrefixesToSkip[] = { { "enum ", 5 }, { "struct ", 7 }, { "class ", 6 }, { "interface ", 10 }, { "union ", 6 }, };
	const char* n = typeinfo_name;
	for (const auto& skip : sPrefixesToSkip)
		if (!memcmp(n, skip.s, skip.len))
			return n + skip.len;
	return n;
}

}
