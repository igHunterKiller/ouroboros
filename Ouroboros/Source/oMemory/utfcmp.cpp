// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oMemory/memory.h>

namespace ouro {

utf_type utfcmp(const void* buf, size_t buf_size)
{
	const uint8_t* b = static_cast<const uint8_t*>(buf);
	if (b[0] == 0xEF && b[1] == 0xBB && b[2] == 0xBF) return utf_type::utf8;
	if (b[0] == 0x00 && b[1] == 0x00 && b[2] == 0xFE && b[3] == 0xFF) return utf_type::utf32be;
	if (b[0] == 0xFF && b[1] == 0xFE && b[2] == 0x00 && b[3] == 0x00) return utf_type::utf32le;
	if (b[0] == 0xFE && b[1] == 0xFF) return utf_type::utf16be;
	if (b[0] == 0xFF && b[1] == 0xFE) return utf_type::utf16le;
	return is_ascii(buf, buf_size) ? utf_type::ascii : utf_type::binary;
}

}
