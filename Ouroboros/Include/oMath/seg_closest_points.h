// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#pragma once
#include <oMath/hlsl.h>

namespace ouro {

// fills the time along each line segment when that point is closest to the
// closest point on the other line segment. This returns results for the
// full lines, but is valid specifically for the segments only when both 
// at and bt are on [0,1]
// a0, a1: end points of segment a
// b0, b1: end points of segment b
// at, bt: defines the point on the segment: a0 + at*(a1-a0) and b0 + bt(b1-b0)
void seg_closest_points(const float3& a0, const float3& a1
	, const float3& b0, const float3& b1
	, float* at, float* bt);

}
