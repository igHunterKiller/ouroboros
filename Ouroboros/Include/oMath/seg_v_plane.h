// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#pragma once
#include <oMath/hlsl.h>

namespace ouro {

// returns true if there is an intersection
// a0, a1: the line segment
// plane: normal is assumed to be normalized
// at0: defines the intersection points on the segment: lerp(a0, a1, *at0)
bool seg_v_plane(const float3& a0, const float3& a1, const float4& plane, float* at0);

inline bool seg_v_plane(const float3& a0, const float3& a1, const float4& plane, float3* out_intersection)
{
	float t0;
	if (seg_v_plane(a0, a1, plane, &t0))
	{
		*out_intersection = lerp(a0, a1, t0);
		return true;
	}
	return false;
}

}
