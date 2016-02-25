// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/type_info.h>
#include <oString/string.h>

namespace ouro {

char* struct_to_string(char* dst, size_t dst_size, const char* label, const char* tag_string, const char** field_names, const type_info_as_string* as_strings, const void* src)
{
	// @tony: is there a way to specify decimal v. hex?
	// @tony: is there a way to specify float sigfigs?

	// Maybe change the as_strings to to_strings and fold the strlcpys into there...
	// then pass as_string or as_string_hex.

	// Still support default fallback, just need overrides occasionally.

	auto base = (const char*)src;

	size_t offset = 0;

	offset += snprintf(dst + offset, dst_size - offset, "%s 0x%p\n{\n", label ? label : tag_string, base);

	int i = 0;
	int count = 0;
	for (const char* tag = tag_string + 1; *tag; tag++)
	{
		if (isdigit(*tag))
		{
			count *= 10;
			count += *tag - '0';
			continue;
		}
		
		count      = __max(1, count);
		auto type  = from_code(*tag);
		auto sz    = fundamental_size(type);
		auto asstr = as_strings[i];

		offset += snprintf(dst + offset, dst_size - offset, "\t%-20s = ", field_names[i]);

		if (asstr)
		{
			const char* str = asstr(base);

			for (int j = 0; j < count; )
			{
				offset += strlcpy(dst + offset, str, dst_size - offset);
				
				j++;
				if (j != count)
				{
					dst[offset++] = ' ';
					if (offset == dst_size)
						return nullptr;
					dst[offset] = '\0';
				}
			}
		}

		else
		{
			offset += snprintf(dst + offset, dst_size - offset, type, count, base);
		}

		base += sz * count;
		count = 0;

		dst[offset++] = '\n';
		if (offset == dst_size)
			return nullptr;
		dst[offset] = '\0';

		i++;
	}

	offset += snprintf(dst + offset, dst_size - offset, "}\n");
	
	return offset < dst_size ? dst : nullptr;
}

}
