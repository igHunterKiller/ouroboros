// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oMath/seg_v_torus.h>
#include <oMath/seg_v_sphere.h>
#include <oMath/matrix.h>
#include <oMath/polynomial.h>
#include <algorithm>

namespace ouro {

template<typename T> int seg_v_xy_torus(const oHLSL3<T>& p, const oHLSL3<T>& d, T major_radius, T minor_radius, T out_roots[4])
{
	// http://www.emeyex.com/site/projects/raytorus.pdf

	const T r2   = minor_radius * minor_radius;
	const T R2   = major_radius * major_radius;
	const T a    = dot(d, d);
	const T b    = dot(p, d) * T(2);
	const T c    = dot(p, p) - r2 - R2;
	const T _4R2 = T(4) * R2;

	const T a4   = a * a;
	const T a3   = a * b * T(2);
	const T a2   = b * b + T(2) * a * c + _4R2 * d.z * d.z;
	const T a1   = b * c * T(2) + T(8) * R2 * p.z * d.z;
	const T a0   = c * c + _4R2 * p.z * p.z - _4R2 * r2;

	return quartic(a0, a1, a2, a3, a4, out_roots);
}

int seg_v_torus(const float3& a0, const float3& a1, const float3& center, const float3& normalized_axis, float major_radius, float minor_radius, float* at0, float* at1, float* at2, float* at3)
{
	const float3 dir = normalize(a1 - a0);

	// check bounding sphere before moving onto more intensive math
	float rmin, rmax;
	if (!ray_v_sphere(a0, dir, float4(center, major_radius + minor_radius), &rmin, &rmax))
		return 0;

	// transform ray into torus local space: xy plane, z-up
	float4x4 tx = translate(-center) * rotate(normalized_axis, float3(0.0f, 0.0f, 1.0f));

	const float3 local_pos = mul(tx, float4(a0, 1.0f)).xyz();
	const float3 local_dir = mul((float3x3)tx, dir);

	// early-out if ray doesn't intersect the two XY planes that bound the torus
	const float zmin = local_pos.z + rmin * local_dir.z;
	const float zmax = local_pos.z + rmax * local_dir.z;
	if ((zmin > minor_radius && zmax > minor_radius) || (zmin < -minor_radius && zmax < -minor_radius))
		return 0;

	double roots[4];
	int n = seg_v_xy_torus<double>(local_pos, local_dir, major_radius, minor_radius, roots);
	if (n)
	{
		std::sort(roots, roots + n);

		if (n > 0) *at0 = (float)roots[0];
		if (n > 1) *at1 = (float)roots[1];
		if (n > 2) *at2 = (float)roots[2];
		if (n > 3) *at3 = (float)roots[3];
	}
	return n;
}
}
