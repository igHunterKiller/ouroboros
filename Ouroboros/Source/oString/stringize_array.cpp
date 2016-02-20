// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oString/stringize_array.h>
#include <oString/atof.h>
#include <oString/string_fast_scan.h>

oDEFINE_WHITESPACE_PARSING();

namespace ouro {

bool from_string_float_array(float* out_values, size_t num_values, const char* src)
{
	if (!out_values || !src) return false;
	move_past_line_whitespace(&src);
	while (num_values--)
	{
		if (!*src) return false;
		if (!atof(&src, out_values++)) return false;
	}
	return true;
}

bool from_string_double_array(double* out_values, size_t num_values, const char* src)
{
	if (!out_values || !src) return false;
	while (num_values--)
	{
		move_past_line_whitespace(&src);
		if (!*src) return false;
		if (1 != sscanf_s(src, "%lf", out_values++)) return false;
		move_to_whitespace(&src);
	}
	return true;
}

}
