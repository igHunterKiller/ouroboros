// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oMath/seg_v_cylinder.h>
#include "slab.h"
#include <float.h>

namespace ouro {

bool seg_vs_cylinder(const float3& a0, const float3& a1
	, const float3& cap_center0, const float3& cap_center1, float radius, float* at)
{
	// Real-Time Collision Detection. Christer Ericson. pp 197-198.

	const float3 d = cap_center1 - cap_center0;
	const float3 m = a0 - cap_center0;
	const float3 n = a1 - a0;

	const float md = dot(m, d);
	const float nd = dot(n, d);
	const float dd = dot(d, d);

	if (md < 0.0f && md + nd < 0.0f)
		return false;
	
	if (md > dd && md + nd > dd)
		return false;

	const float nn = dot(n, n);
	const float mn = dot(m, n);
	const float a = dd * nn - nd * nd;
	const float k = dot(m, m) - radius * radius;
	const float c = dd * k - md * md;
	if (abs(a) < FLT_EPSILON)
	{
		if (c > 0.0f)
			return false;

		if (md < 0.0f)
			*at = -mn / nn;
		else if (md > dd)
			*at = (nd - mn) / nn;
		else
			*at = 0.0f;

		return true;
	}

	const float b = dd * mn - nd * md;
	const float discr = b * b - a * c;

	if (discr < 0.0f)
		return false;

	*at = (-b - sqrt(discr)) / a;

	if (*at < 0.0f || *at > 1.0f)
		return false;

	if (md + (*at) * nd < 0.0f)
	{
		if (nd < 0.0f)
			return false;
		
		*at = -md / nd;
		return k + 2.0f * (*at) * (mn + (*at) * nn) <= 0.0f;
	}

	else if (md + *at * nd > dd)
	{
		if (nd >= 0.0f)
			return false;

		*at = (dd - md) / nd;

		return k + dd - 2.0f * md * (*at) * (2.0f * (mn - nd) + (*at) * nn) <= 0.0f;
	}

	return true;
}

}
