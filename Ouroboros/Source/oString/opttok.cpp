// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oString/opttok.h>
#include <oString/string.h>
#include <oArch/compiler.h>
#include <oCore/snprintf.h>
#include <algorithm>

namespace ouro {

static inline bool isopt(const char* arg) { return *arg == '-'; }
static inline bool islongopt(const char* arg) { return *arg == '-' && *(arg+1) == '-'; }

char opttok(const char** out_value, int argc, const char* argv[], const option* options, size_t num_options)
{
	oTHREAD_LOCAL static const char**s_local_argv = 0;
	oTHREAD_LOCAL static int s_local_argc = 0;
	oTHREAD_LOCAL static const option* s_local_options = 0;
	oTHREAD_LOCAL static size_t s_num_options = 0;

	oTHREAD_LOCAL static int i = 0;

	*out_value = "";

	if (argv)
	{
		s_local_argv = argv;
		s_local_argc = argc;
		s_local_options = options;
		s_num_options = num_options;
		i = 1; // skip exe name
	}

	else if (i >= s_local_argc)
		return 0;

	const char* currarg = s_local_argv[i];
	const char* nextarg = s_local_argv[i+1];

	if (!currarg)
		return 0;

	const option* oend = s_local_options + s_num_options;

	if (islongopt(currarg))
	{
		currarg += 2;
		for (const option* o = s_local_options; o < oend; o++)
		{
			if (!strcmp(o->fullname, currarg))
			{
				if (o->argname)
				{
					if (i == argc-1 || isopt(nextarg))
					{
						i++;
						*out_value = (const char*)i;
						return ':';
					}

					*out_value = nextarg;
				}

				else
					*out_value = currarg;

				i++;
				if (o->argname)
					i++;

				return o->abbrev;
			}

			o++;
		}

		i++; // skip unrecognized opt
		*out_value = currarg;
		return '?';
	}

	else if (isopt(s_local_argv[i]))
	{
		currarg++;
		for (const option* o = s_local_options; o < oend; o++)
		{
			if (*currarg == o->abbrev)
			{
				if (o->argname)
				{
					if (*(currarg+1) == ' ' || *(currarg+1) == 0)
					{
						if (i == argc-1 || isopt(nextarg))
						{
							i++;
							return ':';
						}

						*out_value = nextarg;
						i += 2;
					}

					else
					{
						*out_value = currarg + 1;
						i++;
					}
				}

				else
				{
					*out_value = currarg;
					i++;
				}

				return o->abbrev;
			}
		}

		i++; // skip unrecognized opt
		*out_value = currarg;
		return '?';
	}

	*out_value = s_local_argv[i++]; // skip to next arg
	return ' ';
}

char* optdoc(char* dst, size_t dst_size, const char* app_name, const option* options, size_t num_options, const char* loose_parameters)
{
	*dst = '\0';
	char* cur = dst;

	int w = snprintf(cur, dst_size, "%s [options] ", app_name);
	if (w == -1)
		return nullptr;
	cur += w;
	dst_size -= w;

	const option* oend = options + num_options;
	for (const option* o = options; o < oend; o++)
	{
		w = snprintf(cur, dst_size, "-%c%s%s ", o->abbrev, o->argname ? " " : "", o->argname ? o->argname : "");
		if (w == -1)
			return nullptr;
		cur += w;
		dst_size -= w;
	}

	if (loose_parameters && *loose_parameters)
	{
		w = snprintf(cur, dst_size, "%s", loose_parameters);
		cur += w;
		dst_size -= w;
	}

	w = snprintf(cur, dst_size, "\n\n");
	if (w == -1)
		return nullptr;
	cur += w;
	dst_size -= w;

	size_t max_len_fullname = 0;
	size_t max_len_arg_name = 0;
	for (const option* o = options; o < oend && dst_size > 0; o++)
	{
		max_len_fullname = __max(max_len_fullname, strlen(o->fullname ? o->fullname : ""));
		max_len_arg_name = __max(max_len_arg_name, strlen(o->argname ? o->argname : ""));
	}

	static const size_t MaxLen = 80;
	const size_t descStartCol = 3 + 1 + max_len_fullname + 5 + max_len_arg_name + 4;

	for (const option* o = options; o < oend && dst_size > 0; o++)
	{
		char fullname[64];
		*fullname = '\0';
		if (o->fullname)
			snprintf(fullname, " [--%s]", o->fullname);

		const char* odesc = o->desc ? o->desc : "";
		size_t newline = strcspn(odesc, oNEWLINE);
		char descline[128];
		if (newline > sizeof(descline))
			return nullptr;
		
		strlcpy(descline, odesc);
		bool do_more_lines = descline[newline] != '\0';
		descline[newline] = '\0';

		w = snprintf(cur, dst_size, "  -%c%- *s %- *s : %s\n"
			, o->abbrev
			, max_len_fullname + 5
			, fullname
			, max_len_arg_name
			, o->argname ? o->argname : ""
			, descline);
		if (w == -1)
			return nullptr;
		cur += w;
		dst_size -= w;

		while (do_more_lines)
		{
			odesc += newline + 1;
			newline = strcspn(odesc, oNEWLINE);
			if (newline > sizeof(descline))
				return nullptr;
		
			strlcpy(descline, odesc);
			do_more_lines = descline[newline] != '\0';
			descline[newline] = '\0';

			w = snprintf(cur, dst_size, "% *s%s\n"
				, descStartCol
				, " "
				, descline);
			if (w == -1)
				return dst;
			cur += w;
			dst_size -= w;
		}
	}

	ellipsize(dst, dst_size);
	return dst;
}

}
