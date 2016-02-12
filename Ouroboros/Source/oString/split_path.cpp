// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <string.h>

namespace ouro {

static bool is_sep(char c, bool posix) { return c == '/' || (!posix && c == '\\'); }
static bool is_unc(const char* path, bool posix) { return path && is_sep(path[0], posix) && is_sep(path[1], posix) && !is_sep(path[2], posix) && path[0] == path[1]; }
static bool has_vol(const char* path, bool posix) { return !posix && path && path[0] && path[1] == ':';  }

static const char* find_parentend(const char* path, bool posix, const char* _Basename, bool _UNC)
{
	const char* end = _Basename;
	while (end >= path && is_sep(*end, posix)) end--;
	if (!_UNC && end > (path+1)) end--;
	while (end > (path+1) && is_sep(*end, posix) && is_sep(*(end-1), posix)) end--;
	return end;
}

size_t split_path(const char* path
	, bool posix
	, const char** out_root
	, const char** out_path
	, const char** out_parent_path_end
	, const char** out_basename
	, const char** out_ext)
{
	*out_root = *out_path = *out_parent_path_end = *out_basename = *out_ext = nullptr;
	const size_t len = strlen(path);
	const bool unc = is_unc(path, posix);
	const char* NonUNCPath = path;
	if (unc)
	{
		*out_root = path;
		NonUNCPath = path + 2;
	}

	else if (has_vol(path, posix))
		*out_root = path;

	const char* cur = path + len - 1;
	while (cur >= NonUNCPath)
	{
		if (is_sep(*cur, posix))
		{
			// *(cur+1) fails with foo/ because it's a valid null basename
			// cur != path fails with "/" because there is no parent
			if (!*out_basename)
			{
				if (is_sep(NonUNCPath[0], posix) && !NonUNCPath[1])
					*out_basename = NonUNCPath;
				else
					*out_basename = cur + 1;
				if (*out_ext)
				{
					*out_parent_path_end = find_parentend(path, posix, *out_basename, unc);
					return len;
				}
			}
		}

		else if (*cur == '.' && !*out_ext)
			*out_ext = cur;

		cur--;
	}

	if (*out_ext && !*out_basename && (!(*out_ext)[1] || (*out_ext)[1] == '.')) // "." or ".."
		*out_ext = nullptr;

	if (*out_basename)
	{
		*out_parent_path_end = find_parentend(path, posix, *out_basename, unc);
		if (!**out_basename)
			*out_basename = nullptr;
	}

	return len;
}

}
