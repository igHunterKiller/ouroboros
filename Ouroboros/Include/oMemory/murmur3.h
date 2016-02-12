// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// murmur3 hash

#pragma once
#include <oCore/uint128.h>

namespace ouro {

uint128_t murmur3(const void* buf, size_t buf_size);

}
