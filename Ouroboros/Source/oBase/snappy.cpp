// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oCore/assert.h>
#include <oBase/compression.h>
#include <snappy/snappy.h>

namespace ouro {

size_t compress_snappy(void* oRESTRICT dst, size_t dst_size, const void* oRESTRICT src, size_t src_size)
{
	size_t CompressedSize = snappy::MaxCompressedLength(src_size);
	if (dst)
	{
		oCheck(!dst || dst_size >= CompressedSize, std::errc::no_buffer_space, "");
		snappy::RawCompress(static_cast<const char*>(src), src_size, static_cast<char*>(dst), &CompressedSize);
	}
	return CompressedSize;
}

size_t decompress_snappy(void* oRESTRICT dst, size_t dst_size, const void* oRESTRICT src, size_t src_size)
{
	size_t UncompressedSize = 0;
	snappy::GetUncompressedLength(static_cast<const char*>(src), src_size, &UncompressedSize);
	oCheck(!dst || dst_size >= UncompressedSize, std::errc::no_buffer_space, "");
	oCheck(!dst || snappy::RawUncompress(static_cast<const char*>(src), src_size, static_cast<char*>(dst)), std::errc::protocol_error, "");
	return UncompressedSize;
}

}
