// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oString/stringize.h>
#include <oCore/fourcc.h>
#include <oCore/endian.h>
#include <oCore/guid.h>
#include <oBase/date.h>

namespace ouro {

char* to_string(char* dst, size_t dst_size, const fourcc_t& value)
{
	if (dst_size < 5) return nullptr;
	unsigned int fcc = from_big_endian((unsigned int)value);
	memcpy(dst, &fcc, sizeof(unsigned int));
	dst[4] = 0;
	return dst;
}

char* to_string(char* dst, size_t dst_size, const guid_t& value)
{
	if (dst_size <= 38) return nullptr;
	return -1 != snprintf(dst, dst_size, oGUID_FMT, oGUID_DATA_IN(value)) ? dst : nullptr;
}

bool from_string(fourcc_t* out_value, const char* src) { *out_value = fourcc_t(src); return true; }
bool from_string(guid_t* out_value, const char* src) { return 11 == sscanf_s(src, oGUID_FMT, oGUID_DATA_OUT(*out_value)); }

}
