// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oMath/seg_v_aabb.h>
#include "slab.h"

namespace ouro {

bool seg_vs_aabb(const float3& a0, const float3& a1
	, const float3& aabb_min, const float3& aabb_max, float* at, float* bt)
{
	float enter = 0.0f, exit = 1.0f;
	float3 vec = a1 - a0;
	if ( segslab(aabb_min.x, aabb_max.x, a0.x, a1.x, &enter, &exit)
		&& segslab(aabb_min.y, aabb_max.y, a0.y, a1.y, &enter, &exit)
		&& segslab(aabb_min.z, aabb_max.z, a0.z, a1.z, &enter, &exit))
	{
		*at = enter;
		*bt = exit;
		return true;
	}
	return false;
}

}
