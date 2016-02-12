// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// a hash of a 3D position
// http://www.beosil.com/download/CollisionDetectionHashing_VMV03.pdf

#pragma once
#include <cstdint>

namespace ouro {

inline uint64_t spatial_hash(float x, float y, float z)
{
	uint64_t ix = *(uint32_t*)&x;
	uint64_t iy = *(uint32_t*)&y;
	uint64_t iz = *(uint32_t*)&z;
	return (ix * 73856093) ^ (iy * 19349663) ^ (iz * 83492791);
}

}
