// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// a ring is a 2D torus (circle outline) in 3-space

#pragma once
#include <oMath/hlsl.h>

namespace ouro {

// returns true if there is an intersection
// a0, a1: the line segment
// center: location of the ring's center
// normal: normalized normal pointing out of the circle/plane of the ring
// radius: the radius of the ring - collision occurs only on the edge
// eps: epsilon for where collision occurs on the radius
// at0: defines the intersection points on the segment: lerp(a0, a1, *at0)
bool seg_v_ring(const float3& a0, const float3& a1, const float3& center, const float3& normal, float radius, float eps, float* at0);

inline bool seg_v_ring(const float3& a0, const float3& a1, const float3& center, const float3& normal, float radius, float eps, float3* out_intersection)
{
	float t0;
	if (seg_v_ring(a0, a1, center, normal, radius, eps, &t0))
	{
		*out_intersection = lerp(a0, a1, t0);
		return true;
	}
	return false;
}

}
