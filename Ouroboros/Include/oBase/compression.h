// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// standard interface for any compression algorithm

#pragma once
#include <oCore/restrict.h>
#include <cstdint>

namespace ouro {

enum class compression : uint8_t
{
	none,
	gzip,
	lz4,
	lzma,
	snappy,

	count,
};

// If dst is nullptr return an estimated size requirement for dst. If dst is 
// valid return the actual compressed size. In failure in either case, throw.
size_t compress(const compression& type, void* oRESTRICT dst, size_t dst_size, const void* oRESTRICT src, size_t src_size);

// Returns the uncompressed size of src. Call this first with dst = nullptr to 
// get the size value to properly allocate a destination buffer then pass that 
// as dst to finish the decompression. NOTE: src must have been a buffer produced 
// with compress because its implementation may have tacked on extra information 
// to the buffer and thus only the paired implementation of decompress knows how 
// to access such a buffer correctly. (Implementation note: if an algorithm doesn't 
// store the uncompressed size, then it should be tacked onto the compressed buffer 
// and accessible by this function to comply with the API specification.)
size_t decompress(const compression& type, void* oRESTRICT dst, size_t dst_size, const void* oRESTRICT src, size_t src_size);

}
