// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/type_info.h>
#include <oString/string.h>

namespace ouro {

size_t field_snprintf(char* dst, size_t dst_size, const char* label, size_t indent, type_info_to_string_fn to_string, const void* data)
{
	size_t final_length = 0;
	size_t spacing      = 2 * indent;

	// append indent
	final_length += spacing;
	if (final_length < dst_size)
		memset(dst, ' ', spacing);
	dst += final_length;

	// append label
	size_t len = strlen(label);
	final_length += len;
	if (final_length < dst_size)
	{
		strlcpy(dst, label, dst_size); // dst size pre-checked, so dst_size doesn't control anything
		dst += len;
	}

	// append equal sign
	final_length += 1;
	if (final_length < dst_size)
		*dst++ = '=';

	// append value
	len = to_string(dst, dst_size - final_length, data);
	dst += len;
	final_length += len;

	// append newline
	final_length++;
	if (final_length < dst_size)
	{
		*dst++ = '\n';
		*dst++ = '\0';
	}

	return final_length;
}

}
