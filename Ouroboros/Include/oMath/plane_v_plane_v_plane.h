// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#pragma once
#include <oMath/hlsl.h>

namespace ouro {

// returns true if the 3 planes intersect. out_intersection contains the point of intersection
bool plane_v_plane_v_plane(const float4& a, const float4& b, const float4& c, float3* out_intersection);

}
