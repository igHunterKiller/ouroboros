// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oMath/clip_convex.h>
#include <oMath/hlslx.h>

namespace ouro {

uint32_t clip_convex(const float3* oRESTRICT polygon, uint32_t num_vertices
	, const float4& plane, float3* oRESTRICT out_clipped_vertices)
{
	float3 v0;
	float d0;
	bool i0;
	
	float3 v1 = polygon[num_vertices-1];
	float d1 = sdistance(plane, v1);
	bool i1 = d1 > 0.0f;

	uint32_t num_clipped_vertices = 0;
	for (uint32_t i = 0; i < num_vertices; i++)
	{
		v0 = v1;
		d0 = d1;
		i0 = i1;

		v1 = polygon[i];
		d1 = sdistance(plane, v1);
		i1 = d1 > 0.0f;

		if (i0)       out_clipped_vertices[num_clipped_vertices++] = v0;
		if (i0 != i1) out_clipped_vertices[num_clipped_vertices++] = lerp(v0, v1, d0 / (d0-d1));
	}

	return num_clipped_vertices;
}

}
