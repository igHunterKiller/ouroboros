// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oMath/seg_closest_points.h>
#include <oMath/floats.h>

namespace ouro {

void seg_closest_points(const float3& a0, const float3& a1
	, const float3& b0, const float3& b1
	, float* at, float* bt)
{ 
	// Paul Bourke, assumed public domain
	// http://paulbourke.net/geometry/pointlineplane/lineline.c
	// (modified to use hlsl, push through degenerate cases, and defer calcuations of the points to user code)
	// note: a0 = p1, a1 = p2, b0 = p3, b1 = p4
	const float3 p13 = a0 - b0;
	const float3 p43 = b1 - b0;
	const float3 p21 = a1 - a0;
	const float d1343 = dot(p13, p43);
	const float d4321 = dot(p43, p21);
	const float d1321 = dot(p13, p21);
	const float d4343 = dot(p43, p43);
	const float d2121 = dot(p21, p21);
	const float denom = d2121 * d4343 - d4321 * d4321;
	const float numer = d1343 * d4321 - d1321 * d4343;
	*at = numer / max(denom, oVERY_SMALLf);
	*bt = (d1343 + d4321 * (*at)) / d4343;
} 

}
