// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#pragma once
#include <oMath/hlsl.h>

namespace ouro {

// returns true if there is an intersection
// a0, a1: the line segment
// cap_center0, cap_center1: points at the center of the cylinder's endpoints
// radius: radius of cylinder
// at: defines the intersection point on the segment: a0 + at*(a1-a0)
bool seg_vs_cylinder(const float3& a0, const float3& a1
	, const float3& cap_center0, const float3& cap_center1, float radius, float* at);

}
