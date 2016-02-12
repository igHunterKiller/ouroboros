// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oMath/equal.h>

namespace ouro {

bool segslab(float slab_min, float slab_max, float start, float dir, float* inout_enter, float* inout_exit)
{
	if (equal(dir, 0.0f))
		return start >= slab_min && start <= slab_max;

	float rcp_dir = 1.0f / dir;
	float enter = (slab_min - start) * rcp_dir;
	float exit = (slab_max - start) * rcp_dir;

	if (enter > exit)
	{
		float tmp = enter;
		enter = exit;
		exit = tmp;
	}

	if (enter > *inout_exit || exit < *inout_enter)
		return false;

	if (enter > *inout_enter)
		*inout_enter = enter;
	if (exit < *inout_exit)
		*inout_exit = exit;

	return true;
}

}
