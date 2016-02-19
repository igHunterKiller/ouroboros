// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oArch/compiler.h>
#include <oCore/assert.h>
#include <oString/string.h>
#include <system_error>

#define oHEXDIGIT(d) ("0123456789abcdef"[(d)&0xf])

namespace ouro {

char* json_escape_encode(char* oRESTRICT dst, size_t dst_size, const char* oRESTRICT src)
{
	sncatf(dst, dst_size, "\"");
	while (*src)
	{
		if ((*src & 0x80) != 0)
		{
			// TODO: Support UTF-8 
			oThrow(std::errc::function_not_supported, "UTF-8 not yet supported");
			//sncatf(dst, dst_size, "_");
			//int unicode = oUTF8Decode(src);
			//sncatf(dst, dst_size, "\\u%c%c%c%c", oHEXDIGIT((unicode >> 12) & 0xf), oHEXDIGIT((unicode >> 8) & 0xf), oHEXDIGIT((unicode >> 4) & 0xf), oHEXDIGIT(unicode & 0xf));
		}
		if (*src == '\\' || *src == '\"')
		{
			sncatf(dst, dst_size, "\\%c", *src);
		}
		else if (*src <= 0x1f)
		{
			switch (*src)
			{
				case '\b': sncatf(dst, dst_size, "\\b"); break;
				case '\f': sncatf(dst, dst_size, "\\f"); break;
				case '\n': sncatf(dst, dst_size, "\\n"); break;
				case '\r': sncatf(dst, dst_size, "\\r"); break;
				case '\t': sncatf(dst, dst_size, "\\t"); break;
				default: sncatf(dst, dst_size, "\\u00%c%c", oHEXDIGIT(*src >> 4), oHEXDIGIT(*src)); break;
			}
		}
		else
			sncatf(dst, dst_size, "%c", *src);

		src++;
	}
	sncatf(dst, dst_size, "\"");
	return dst;
}

}
