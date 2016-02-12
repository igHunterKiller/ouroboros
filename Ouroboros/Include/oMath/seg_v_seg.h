// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#pragma once
#include <oMath/hlsl.h>

namespace ouro {

// returns true if the two segments are within epsilon distance of
// intersecting.
// a0, a1: end points of segment a
// b0, b1: end points of segment b
// epsilon: the fuzziness of the intersection
// at, bt: defines the point on the segment: a0 + at*(a1-a0) and b0 + bt(b1-b0)
// p0: point of intersection if this returns true
bool seg_vs_seg(const float3& a0, const float3& a1
	, const float3& b0, const float3& b1
	, float epsilon, float* at, float* bt, float3* p0);

}
