// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#pragma once
#include <oMath/hlsl.h>
#include <cstdint>

namespace ouro {

float calcuate_area_and_centroid(float2* out_centroid, const float2* vertices, uint32_t vertex_stride, uint32_t num_vertices);

}
