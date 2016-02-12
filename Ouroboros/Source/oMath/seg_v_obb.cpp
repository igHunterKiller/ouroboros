// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oMath/matrix.h>
#include <oMath/seg_v_aabb.h>
#include <oMath/seg_v_obb.h>
#include "slab.h"

namespace ouro {

bool seg_vs_obb(const float3& a0, const float3& a1
	, const float3x3& rotation, const float3& center, const float3& extents, float* at, float* bt)
{
	// rotate segment into a space where the obb is an aabb at the origin
	const float3x3 r = transpose(rotation);
	const float3 r0 = mul(r, a0 - center);
	const float3 r1 = mul(r, a1 - center);

	// evaluate as an aabb
	return seg_vs_aabb(r0, r1, -extents, extents, at, bt);
}

}
