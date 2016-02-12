// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oMath/seg_v_plane.h>
#include <oMath/equal.h>

namespace ouro {

bool seg_v_plane(const float3& a0, const float3& a1, const float4& plane, float* at0)
{
	float d0 = dot(plane.xyz(), a0) + plane.w;
	float d1 = dot(plane.xyz(), a1) + plane.w;
	bool in0 = 0.0f > d0;
	bool in1 = 0.0f > d1;

	if ((in0 && in1) || (!in0 && !in1)) // totally in or totally out
		return false;
	
	// partial
	// the intersection point is along p0,p1, so p(t) = p0 + t(p1 - p0)
	// the intersection point is on the plane, so (p(t) - C) . N = 0
	// with above distance function, C is 0,0,0 and the offset along 
	// the normal is considered. so (pt - c) . N is distance(pt)

	// (p0 + t ( p1 - p0 ) - c) . n = 0
	// p0 . n + t (p1 - p0) . n - c . n = 0
	// t (p1 - p0) . n = c . n - p0 . n
	// ((c - p0) . n) / ((p1 - p0) . n)) 
	//  ^^^^^^^ (-(p0 -c)) . n: this is why -distance

	float3 vec = a1 - a0;
	float denom = dot(vec, plane.xyz());

	if (equal(denom, 0.0f))
		return false;

	*at0 = abs((dot(plane.xyz(), a0) + plane.w) / denom);
	return true;
}

}
