// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#pragma once

#include <oMath/hlsl.h>

namespace ouro {

// Determines a location in 3D space based on 4 reference locations and their 
// distances from the location.
float trilaterate(const float3 in_observers[4], const float in_distances[4], float3* out_position);

inline float trilaterate(const float3 observers[4], float distance_, float3* out_position)
{
	float distances[4];
	for(int i = 0; i < 4; i++)
		distances[i] = distance_;
	return trilaterate(observers, distances, out_position);
}

}
