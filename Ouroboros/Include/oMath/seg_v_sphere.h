// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#pragma once
#include <oMath/hlsl.h>

namespace ouro {

// returns true if there is an intersection
// a0, a1: the line segment
// sphere: the sphere
// at0, at1: defines the intersection points on the segment: lerp(a0, a1, *at0) and lerp(a0, a1, *at1)
bool seg_v_sphere(const float3& a0, const float3& a1, const float4& sphere, float* at0, float* at1);

inline bool seg_v_sphere(const float3& a0, const float3& a1, const float4& sphere, float3* out_intersection)
{
	float t0, t1;
	if (seg_v_sphere(a0, a1, sphere, &t0, &t1))
	{
		*out_intersection = lerp(a0, a1, t0);
		return true;
	}
	return false;
}

// returns true if there is an intersection
// pos, dir: ray's position and normalized direction
// sphere: the sphere
// at0, at1: defines the length from pos in dir to the collisions: col = pos + *at0 * normalized_dir
bool ray_v_sphere(const float3& pos, const float3& dir, const float4 sphere, float* at0, float* at1);

}
