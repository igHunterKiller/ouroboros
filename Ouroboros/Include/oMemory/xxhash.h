// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// xxhash hash r36
// https://code.google.com/p/xxhash/

#pragma once
#include <cstdint>

namespace ouro {

uint32_t xxhash32(const void* buf, size_t buf_size, uint32_t seed = 0);
uint64_t xxhash64(const void* buf, size_t buf_size, uint64_t seed = 0);

}
