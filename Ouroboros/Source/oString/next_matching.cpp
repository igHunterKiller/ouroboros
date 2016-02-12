// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oString/string.h>
#include <memory.h>

namespace ouro {

const char* next_matching(const char* p_open_brace, char close_brace)
{
	int open = 1;
	char open_brace = *p_open_brace;
	const char* cur = p_open_brace + 1;
	while (*cur && open > 0)
	{
		if (*cur == close_brace)
			open--;
		else if (*cur == open_brace)
			open++;
		cur++;
	}

	if (open > 0)
		return nullptr;
	return cur - 1;
}

char* next_matching(char* p_open_brace, char close_brace)
{
	return const_cast<char*>(next_matching(static_cast<const char*>(p_open_brace), close_brace));
}

const char* next_matching(const char* p_open_brace, const char* open_brace, const char* close_brace)
{
	int open = 1;
	size_t lOpen = strlen(open_brace);
	size_t lClose = strlen(close_brace);
	const char* cur = p_open_brace + lOpen;
	while (*cur && open > 0)
	{
		if (!memcmp(cur, close_brace, lClose))
			open--;
		else if (!memcmp(cur, open_brace, lOpen))
			open++;
		cur++;
	}

	if (open > 0)
		return nullptr;
	return cur - 1;
}

char* next_matching(char* p_open_brace, const char* open_brace, const char* close_brace)
{
	return const_cast<char*>(next_matching(static_cast<const char*>(p_open_brace), open_brace, close_brace));
}

}
