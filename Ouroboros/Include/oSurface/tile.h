// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Utility code for surface tiles: subregions of a single surface subresource
// mainly for sparse virtual texture (megatexture) paging units.

#pragma once
#include <oSurface/surface.h>

namespace ouro { namespace surface {

struct tile_info
{
	uint2 position;
	uint2 dimensions;
	uint32_t mip_level;
	uint32_t array_slice;
};

uint2 dimensions_in_tiles(const uint2& mip_dimensions, const uint2& tile_dimensions);    // converts the specified texel dimensions into tile dimensions
uint3 dimensions_in_tiles(const uint3& mip_dimensions, const uint2& tile_dimensions);    //
uint32_t num_tiles(const uint2& mip_dimensions, const uint2& tile_dimensions);           // total tile count for the specified mip dimensions
uint32_t num_tiles(const uint3& mip_dimensions, const uint2& tile_dimensions);           //
uint32_t num_slice_tiles(const info_t& info, const uint2& tile_dimensions);              // total tile count for all in an array_slice's mip chain
uint32_t calc_tile(const info_t& info, const tile_info& tile_info, uint2* out_position); // like calc_subresource, but for tiles
tile_info tileinfo(const info_t& info, const uint2& tile_dimensions, uint32_t tile);     //

}}
