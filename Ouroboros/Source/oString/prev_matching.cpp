// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oString/string.h>
#include <memory.h>

namespace ouro {

const char* prev_matching(const char* buffer_start, const char* p_close_brace, char open_brace)
{
	int close = 1;
	char close_brace = *p_close_brace;
	const char* cur = p_close_brace - 1;
	while (cur >= buffer_start && close > 0)
	{
		if (*cur == open_brace)
			close--;
		else if (*cur == close_brace)
			close++;
		cur--;
	}

	if (close > 0)
		return nullptr;
	return cur + 1;
}

char* prev_matching(char* buffer_start, char* p_close_brace, char open_brace)
{
	return const_cast<char*>(prev_matching(static_cast<const char*>(buffer_start), static_cast<const char*>(p_close_brace), open_brace));
}

const char* prev_matching(const char* buffer_start, const char* p_close_brace, const char* open_brace, const char* close_brace)
{
	int close = 1;
	size_t lOpen = strlen(open_brace);
	size_t lClose = strlen(close_brace);
	const char* cur = p_close_brace;
	while (cur >= buffer_start && close > 0)
	{
		if (!memcmp(cur, close_brace, lClose))
			close++;
		else if (!memcmp(cur, open_brace, lOpen))
			close--;
		cur--;
	}

	if (close > 0)
		return nullptr;
	return cur + 1;
}

char* prev_matching(char* buffer_start, char* p_close_brace, const char* open_brace, const char* close_brace)
{
	return const_cast<char*>(prev_matching(static_cast<const char*>(buffer_start), static_cast<const char*>(p_close_brace), open_brace, close_brace));
}

}
