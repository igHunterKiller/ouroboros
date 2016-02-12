// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

namespace ouro {

static inline bool issep(int c) { return c == '\\' || c == '/'; }
static inline bool isdotdir(const char* s) { return s[0] == '.' && issep(s[1]); }
static inline bool isdotend(const char* s) { return s[0] == '.' && !s[1]; }
static inline bool isdotdotdir(const char* s) { return s[0] == '.' && s[1] == '.' && issep(s[2]); }
static inline bool isdotdotend(const char* s) { return s[0] == '.' && s[1] == '.' && !s[2]; }
static inline bool isunc(const char* s) { return issep(s[0]) && issep(s[1]); }

// back up write head to last dir
#define BACK_UP() do \
{	while (w != dst && issep(*(w-1))) w--; \
	while (w != dst && !issep(*(w-1))) w--; \
	if (w == dst && issep(*w)) w++; \
} while(false)

char* clean_path(char* dst, size_t dst_size, const char* src_path, char separator, bool zero_buffer)
{
	if (!dst || !src_path)
		return nullptr;

	char* w = dst;
	char* wend = w + dst_size;
	char* wendm1 = wend - 1; // save room for nul
	*wendm1 = 0; // be safe
	const char* r = src_path;

	// preserve UNC
	if (isunc(r))
	{
		if (dst_size <= 2)
			return nullptr;
		*w++ = *r++;
		*w++ = *r++;
	}

	// preserve leading dots
	while (*r)
	{
		if (isdotdir(r))
		{
			if (dst_size <= 2)
				return nullptr;
			*w++ = *r++;
			*w++ = *r++;
			dst_size -= 2;
		}
		else if (isdotdotdir(r))
		{
			if (dst_size <= 3)
				return nullptr;
			*w++ = *r++;
			*w++ = *r++;
			*w++ = *r++;
			dst_size -= 3;
		}
		else
			break;
	}

	while (*r && w < wendm1)
	{
		if (isdotdir(r))
			r += 2;

		else if (isdotend(r))
			break;

		else if (isdotdotdir(r))
		{
			BACK_UP();
			// skip the read head
			r += 3;
		}

		else if (isdotdotend(r))
		{
			BACK_UP();
			break;
		}

		else if (issep(*r))
		{
			*w++ = separator;
			do { r++; } while (*r && issep(*r));
		}

		else
			*w++ = *r++;
	}

	*w = 0;
	if (zero_buffer)
		while (w < wend)
			*w++ = 0;
	return dst;
}

}
