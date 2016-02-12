// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#pragma once
#include <oMath/hlsl.h>

namespace ouro {

// returns true if there is an intersection
// a0, a1: the line segment
// rotation, center, extents: describes the oriented bounding box
// at, bt: defines the intersection points on the segment: a0 + at*(a1-a0)
bool seg_vs_obb(const float3& a0, const float3& a1
	, const float3x3& rotation, const float3& center, const float3& extents, float* at, float* bt);

}
