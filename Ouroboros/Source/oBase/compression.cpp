// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/compression.h>

#define FOREACH_EXT(macro) macro(gzip) macro(lz4) macro(lzma) macro(snappy)

#define DECLARE_CODEC(codec) \
	size_t compress_##codec(void* oRESTRICT dst, size_t dst_size, const void* oRESTRICT src, size_t src_size); \
	size_t decompress_##codec(void* oRESTRICT dst, size_t dst_size, const void* oRESTRICT src, size_t src_size);

#define COMPRESS(codec)   case compression::##codec: return   compress_##codec(dst, dst_size, src, src_size);
#define DECOMPRESS(codec) case compression::##codec: return decompress_##codec(dst, dst_size, src, src_size);

namespace ouro {

FOREACH_EXT(DECLARE_CODEC)

size_t compress(const compression& type, void* oRESTRICT dst, size_t dst_size, const void* oRESTRICT src, size_t src_size)
{
	switch (type)
	{
		FOREACH_EXT(COMPRESS)
		default: break;
	}

	throw std::invalid_argument("unknown compression format");
}

size_t decompress(const compression& type, void* oRESTRICT dst, size_t dst_size, const void* oRESTRICT src, size_t src_size)
{
	switch (type)
	{
		FOREACH_EXT(DECOMPRESS)
		default: break;
	}

	throw std::invalid_argument("unknown compression format");
}

}
