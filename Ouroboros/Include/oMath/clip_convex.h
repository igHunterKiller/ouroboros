// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#pragma once
#include <oArch/compiler.h>
#include <oMath/hlsl.h>
#include <cstdint>

namespace ouro {

// This will rotate the order of vertices by 1: meaning if 012 were 
// not clipped, then the output would be 201.
// returns the number of valid vertices in out_clipped_vertices
// polygon: a list of vertices in a convex polygon
// num_vertices: number of vertices in polygon
// plane: clip plane, verts on positive side are kept
// out_clipped_vertices: results of clipping, must be able to hold num_vertices+1
uint32_t clip_convex(const float3* oRESTRICT polygon, uint32_t num_vertices
	, const float4& plane, float3* oRESTRICT out_clipped_vertices);

}
