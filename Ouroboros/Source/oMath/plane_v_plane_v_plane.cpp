// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oMath/plane_v_plane_v_plane.h>
#include <oMath/equal.h>

namespace ouro {

bool plane_v_plane_v_plane(const float4& a, const float4& b, const float4& c, float3* out_intersection)
{
	// Goldman, Ronald. Intersection of Three Planes. In A. Glassner,
	// ed., Graphics Gems pg 305. Academic Press, Boston, 1991.
	// http://paulbourke.net/geometry/3planes/

	// check that there is a valid cross product
	float3 bXc = cross(b.xyz(), c.xyz());
	if (equal(dot(bXc, bXc), 0.0f)) 
		return false;

	float3 cXa = cross(c.xyz(), a.xyz());
	if (equal(dot(cXa, cXa), 0.0f)) 
		return false;

	float3 aXb = cross(a.xyz(), b.xyz());
	if (equal(dot(aXb, aXb), 0.0f)) 
		return false;

	*out_intersection = (-a.w * bXc - b.w * cXa - c.w * aXb) / determinant(float3x3(a.xyz(), b.xyz(), c.xyz()));
	return true;
}

}
