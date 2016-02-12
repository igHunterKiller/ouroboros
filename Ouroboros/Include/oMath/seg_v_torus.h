// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#pragma once
#include <oMath/hlsl.h>

namespace ouro {

// returns the number of valid at values
// a0, a1: the line segment
// center: torus center
// normalized_axis: the normal of the plane that splits the torus like a bagel
// major_radius: from center to the center of the torus tube
// minir_radius: from the center of the torus tube to its exterior
// at0, at1, at2, at3: defines the intersection points on the segment: lerp(a0, a1, *at0), etc.
int seg_v_torus(const float3& a0, const float3& a1, const float3& center, const float3& normalized_axis, float major_radius, float minor_radius, float* at0, float* at1, float* at2, float* at3);

inline bool seg_v_torus(const float3& a0, const float3& a1, const float3& center, const float3& normalized_axis, float major_radius, float minor_radius, float3* out_intersection)
{
	float t0, t1, t2, t3;
	if (seg_v_torus(a0, a1, center, normalized_axis, major_radius, minor_radius, &t0, &t1, &t2, &t3))
	{
		*out_intersection = a0 + normalize(a1-a0) * t0; //lerp(a0, a1, t0);
		return true;
	}
	return false;
}

}
