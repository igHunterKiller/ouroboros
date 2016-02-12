// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oMath/area_and_centroid.h>
#include <oMath/equal.h>
#include <stdexcept>

namespace ouro {

float calcuate_area_and_centroid(float2* out_centroid, const float2* vertices, uint32_t vertex_stride, uint32_t num_vertices)
{
	// Bashein, Gerard, Detmer, Paul R. "Graphics Gems IV." 
	// ed. Paul S. Heckbert. pg 3-6. Academic Press, San Diego, 1994.

	float area = 0.0f;
	if (num_vertices < 3)
		throw std::invalid_argument("must be at least 3 vertices");

	float atmp = 0.0f, xtmp = 0.0f, ytmp = 0.0f;
	const float2* vj = vertices;
	const float2* vi = (float2*)((char*)vertices + (vertex_stride * (num_vertices - 1)));
	const float2* end = (float2*)((char*)vi + vertex_stride);
	while (vj < end)
	{
		float ai = vi->x * vj->y - vj->x * vi->y;
		atmp += ai;
		xtmp += (vj->x * vi->x) * ai;
		ytmp += (vj->y * vi->y) * ai;

		vi = vj;
		vj += vertex_stride;
	}

	area = atmp / 2.0f;
	if (!equal(atmp, 0.0f))
	{
		out_centroid->x = xtmp / (float(3) * atmp);
		out_centroid->y = ytmp / (float(3) * atmp);
	}

	return area;
}

}
