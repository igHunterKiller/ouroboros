// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oMath/seg_v_sphere.h>
#include <oMath/polynomial.h>

namespace ouro {

bool seg_v_sphere(const float3& a0, const float3& a1, const float4& sphere, float* at0, float* at1)
{
	const float3 v01 = a1 - a0;
	const float3 vc0 = a0 - sphere.xyz();

	float roots[2];
	int nroots = quadratic(dot(v01, v01), 2.0f * dot(v01, vc0), dot(vc0, vc0) - (sphere.w * sphere.w), roots);
	if (nroots)
	{
		*at0 = roots[0];
		*at1 = roots[1];
	}
	
	return !!nroots;
}

bool ray_v_sphere(const float3& pos, const float3& dir, const float4 sphere, float* at0, float* at1)
{
	// Cychosz, Joseph M., Intersecting a Ray with An Elliptical Torus, Graphics Gems II, p. 251-256
	// http://tog.acm.org/resources/GraphicsGems/gemsii/intersect/

	const float3 d   = pos - sphere.xyz();
	const float bsq  = dot(d, dir);
	const float u    = dot(d, d) - sphere.w * sphere.w;
	const float disc = bsq * bsq - u;

	if (disc >= 0.0f)
	{
		float root = sqrt(disc);
		*at0 = -bsq - root;
		*at1 = -bsq + root;
		return true;
	}

	return false;
}

}
