// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.
#include <oBase/vendor.h>

namespace ouro {

const char* as_string(const vendor& _Vendor)
{
	switch (_Vendor)
	{
		case vendor::unknown: return "unknown";
		case vendor::amd: return "amd";
		case vendor::apple: return "Apple";
		case vendor::arm: return "ARM";
		case vendor::intel: return "Intel";
		case vendor::internal: return "internal";
		case vendor::lg: return "LG";
		case vendor::maxtor: return "Maxtor";
		case vendor::microsoft: return "Microsoft";
		case vendor::nintendo: return "Nintendo";
		case vendor::nvidia: return "NVIDIA";
		case vendor::sandisk: return "SanDisk";
		case vendor::samsung: return "Samsung";
		case vendor::sony: return "Sony";
		case vendor::vizio: return "Vizio";
		case vendor::western_digital: return "Western Digital";
		default: break;
	}
	return "?";
}

}
