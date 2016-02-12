// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.
#include <oBase/ip4.h>
#include <oString/string.h>

namespace ouro {

char* to_string(char* dst, size_t dst_size, const ip4& addr)
{
	int offset = snprintf(dst, dst_size, "%u.%u.%u.%u", 
		addr.octet(0), addr.octet(1), addr.octet(2), addr.octet(3));
	if (offset < 0)
		return nullptr;
	if (addr.port())
		offset = snprintf(dst + offset, dst_size - offset, ":%u", addr.port());
	return offset >= 0 ? dst : nullptr;
}

bool from_string(ip4* out_addr, const char* src)
{
	unsigned int x,y,z,w,p = 0;
	if (4 > sscanf_s(src, "%u.%u.%u.%u:%u", &x, &y, &z, &w, &p)
		&& ((x & 0xffffff00) || (y & 0xffffff00) || (z & 0xffffff00) || (w & 0xffffff00) || (p & 0xffff0000)))
		return false;

	*out_addr = ip4((unsigned char)x, (unsigned char)y, (unsigned char)z, (unsigned char)w, (unsigned short)p);
	return true;
}

}
