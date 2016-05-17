// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

//#include <oArch/arch.h>
#include <oString/string.h>

namespace ouro {

static void json_escape_decode_unicode(char* oRESTRICT dst, size_t dst_size, const char* oRESTRICT src)
{
	unsigned int UnicodeValue = 0;
	// Expects 4 digits from src and turns it into UTF-8
	for (int i = 0; i < 4; i++)
	{
		UnicodeValue <<= 4;
		char Digit = *src++;
		if (Digit >= '0' && Digit <= '9') UnicodeValue |= Digit - '0';
		else if (Digit >= 'a' && Digit <= 'f') UnicodeValue |= Digit - 'a' + 10;
		else if (Digit >= 'A' && Digit <= 'F') UnicodeValue |= Digit - 'A' + 10;
	}
	// TODO: Support UTF-8 and surrogate pair ranges
	if (!UnicodeValue || UnicodeValue > 255)
		sncatf(dst, dst_size, "_");
	else
		sncatf(dst, dst_size, "%c", (unsigned char)UnicodeValue);
}

char* json_escape_decode(char* oRESTRICT dst, size_t dst_size, const char* oRESTRICT src)
{
	if (-1 == snprintf(dst, dst_size, ""))
		return nullptr;

	if (*src++ != '\"')
		return nullptr;

	while (*src)
	{
		if (*src == '\\')
		{
			switch (*++src)
			{
				case '\\': sncatf(dst, dst_size, "\\"); break;
				case '\"': sncatf(dst, dst_size, "\""); break;
				case 'b': sncatf(dst, dst_size, "\b"); break;
				case 'f': sncatf(dst, dst_size, "\f"); break;
				case 'n': sncatf(dst, dst_size, "\n"); break;
				case 'r': sncatf(dst, dst_size, "\r"); break;
				case 't': sncatf(dst, dst_size, "\t"); break;
				case 'u': json_escape_decode_unicode(dst, dst_size, ++src); src += 3; break;
				default: return false;
			}
		}
		else if (*src != '\"')
			sncatf(dst, dst_size, "%c", *src);

		src++;
	}
	return dst;
}

}
