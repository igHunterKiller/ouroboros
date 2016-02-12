// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oMath/seg_v_seg.h>
#include <oMath/seg_closest_points.h>
#include <oMath/floats.h>

namespace ouro {

bool seg_vs_seg(const float3& a0, const float3& a1
	, const float3& b0, const float3& b1
	, float epsilon, float* at, float* bt, float3* p0)
{
	seg_closest_points(a0, a1, b0, b1, at, bt);
	if (*at < 0.0f || *at > 1.0f || *bt < 0.0f || *bt > 1.0f)
		return false;
	*p0 = lerp(a0, a1, *at);
	const float3 p1 = lerp(b0, b1, *bt);
	return distance(*p0, p1) < epsilon;
}

}
