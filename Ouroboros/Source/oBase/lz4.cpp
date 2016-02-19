// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oCore/assert.h>
#include <oCore/byte.h>
#include <oBase/compression.h>

namespace ouro {

size_t compress_lz4(void* oRESTRICT dst, size_t dst_size, const void* oRESTRICT src, size_t src_size)
{
	oThrow(std::errc::invalid_argument, "not yet implemented");
}

size_t decompress_lz4(void* oRESTRICT dst, size_t dst_size, const void* oRESTRICT in_src, size_t src_size)
{
	oThrow(std::errc::invalid_argument, "not yet implemented");
}

}
