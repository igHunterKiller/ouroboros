// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oMath/seg_v_plane.h>
#include <oMath/seg_v_sphere.h>
#include <oMath/equal.h>
#include <oMath/hlslx.h>

namespace ouro {

bool seg_v_ring(const float3& a0, const float3& a1, const float3& center, const float3& normal, float radius, float eps, float* at0)
{
	const float4 ring_plane = plane(normal, center);

	// handle general case: intersect ray with plane of the ring, 
	// then test if it's close to being on the radius of the ring
	float t0, t1;
	if (seg_v_plane(a0, a1, ring_plane, &t0))
	{
		float3 pt = lerp(a0, a1, t0);
		float dist = distance(pt, center);
		if (equal_eps(dist, radius, eps))
		{
			*at0 = t0;
			return true;
		}
	}

	// handle edge-on case where ring looks like a line from the 
	// ray's point of view.
	if (seg_v_sphere(a0, a1, float4(center, radius), &t0, &t1))
	{
		float3 pt = lerp(a0, a1, t0);
		float dist = dot(normal, pt);
		if (abs(dist) < eps)
		{
			*at0 = t0;
			return true;
		}
	}

	return false;
}

}