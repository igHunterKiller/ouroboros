// Copyright (c) 2015 Antony Arciuolo. See License.txt regarding use.

// Enumeration of common compression algorithms

#pragma once
#include <cstdint>

namespace ouro {

enum class compression_type : uint8_t
{
	none,
	gzip,
	lz4,
	lzma,
	snappy,

	count,
};

}
