// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#pragma once
#include <oMath/hlsl.h>

namespace ouro {

// returns true if there is an intersection
// a0, a1: the line segment
// aabox_min, aabox_max: the axis-aligned box
// at, bt: defines the intersection points on the segment: a0 + at*(a1-a0)
bool seg_vs_aabb(const float3& a0, const float3& a1
	, const float3& aabb_min, const float3& aabb_max, float* at, float* bt);

}
