// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oSurface/tile.h>

#define oSURF_CHECK(expr, format, ...) do { if (!(expr)) oThrow(std::errc::invalid_argument, format, ## __VA_ARGS__); } while(false)

namespace ouro { namespace surface {

uint32_t dimension_in_tiles(int mip_dimension, int tile_dimension)
{
	uint32_t div = mip_dimension / tile_dimension;
	if (0 != (mip_dimension % tile_dimension))
		div++;
	return div;
}

uint2 dimensions_in_tiles(const uint2& mip_dimensions, const uint2& tile_dimensions)
{
	return uint2(dimension_in_tiles(mip_dimensions.x, tile_dimensions.x)
		, dimension_in_tiles(mip_dimensions.y, tile_dimensions.y));
}

uint3 dimensions_in_tiles(const uint3& mip_dimensions, const uint2& tile_dimensions)
{
	return uint3(dimension_in_tiles(mip_dimensions.x, tile_dimensions.x)
		, dimension_in_tiles(mip_dimensions.y, tile_dimensions.y)
		, mip_dimensions.z);
}

uint32_t num_tiles(const uint2& mip_dimensions, const uint2& tile_dimensions)
{
	auto mip_tile_dim = dimensions_in_tiles(mip_dimensions, tile_dimensions);
	return mip_tile_dim.x * mip_tile_dim.y;
}

uint32_t num_tiles(const uint3& mip_dimensions, const uint2& tile_dimensions)
{
	auto mip_tile_dim = dimensions_in_tiles(mip_dimensions, tile_dimensions);
	return mip_tile_dim.x * mip_tile_dim.y;
}

// returns the nth mip where the mip level and tile size are the same
static uint32_t best_fit_mip_level(const info_t& info, const uint2& tile_dimensions)
{
	if (!info.mips())
		return 0;

	uint32_t nth = 0;
	auto mip = info.dimensions;
	while (any(mip != uint3(1)))
	{
		if (all(info.dimensions.xy() <= tile_dimensions))
			break;

		nth++;
		mip = ::max(uint3(1), mip / 2);
	}

	return nth;
}

uint32_t num_slice_tiles(const info_t& info, const uint2& tile_dimensions)
{
	oSURF_CHECK(!is_planar(info.format), "planar formats not tested");

	if (!info.mips())
		return num_tiles(info.dimensions, tile_dimensions);

	uint32_t ntiles = 0;
	uint32_t last_mip = 1 + best_fit_mip_level(info, tile_dimensions);
	for (uint32_t i = 0; i <= last_mip; i++)
	{
		auto mip_dim = dimensions(info.format, info.dimensions, i);
		ntiles += num_tiles(mip_dim, tile_dimensions);
	}

	return ntiles;
}

static uint32_t slice_first_tile_id(const info_t& info, const uint2& tile_dimensions, uint32_t slice)
{
	oSURF_CHECK(!is_planar(info.format), "planar formats not tested");
	auto num_tiles_per_slice = num_slice_tiles(info, tile_dimensions);
	return slice * num_tiles_per_slice;
}

// how many tiles from the start to start the specified mip
static uint32_t tile_offset(const info_t& info, const uint2& tile_dimensions, uint32_t mip)
{
	oSURF_CHECK(!is_planar(info.format), "planar formats not tested");
	if (mip_layout::none == info.mip_layout)
		return 0;
	uint32_t ntiles = 0;
	uint32_t nMips = ::min(mip, 1u + best_fit_mip_level(info, tile_dimensions));
	for (uint32_t i = 0; i < nMips; i++)
	{
		auto mip_dim = dimensions(info.format, info.dimensions, i);
		ntiles += num_tiles(mip_dim, tile_dimensions);
	}

	return ntiles;
}

static uint32_t mip_first_tile_id(const info_t& info, const uint2& tile_dimensions, uint32_t mip, uint32_t slice)
{
	oSURF_CHECK(!is_planar(info.format), "planar formats not tested");
	auto sliceStartID = slice_first_tile_id(info, tile_dimensions, slice);
	auto mipIDOffset = tile_offset(info, tile_dimensions, mip);
	return sliceStartID + mipIDOffset;
}

uint32_t calc_tile_id(const info_t& info, const tile_info& _TileInfo, uint2* out_position)
{
	oSURF_CHECK(!is_planar(info.format), "planar formats not tested");
	int mipStartTileID = mip_first_tile_id(info, _TileInfo.dimensions, _TileInfo.mip_level, _TileInfo.array_slice);
	int2 PositionInTiles = _TileInfo.position / _TileInfo.dimensions;
	int2 mip_dim = dimensions(info.format, info.dimensions.xy(), _TileInfo.mip_level);
	int2 mip_tile_dim = dimensions_in_tiles(mip_dim, _TileInfo.dimensions);
	int tileID = mipStartTileID + (mip_tile_dim.x * PositionInTiles.y) + PositionInTiles.x;
	if (out_position)
		*out_position = PositionInTiles * _TileInfo.dimensions;
	return tileID;
}

tile_info get_tile(const info_t& info, const uint2& tile_dimensions, uint32_t tileid)
{
	tile_info tileinf;

	oSURF_CHECK(!is_planar(info.format), "planar formats not tested");
	auto num_tiles_per_slice = num_slice_tiles(info, tile_dimensions);
	tileinf.dimensions = tile_dimensions;
	tileinf.array_slice = tileid / num_tiles_per_slice;
	oSURF_CHECK(::max(1u, tileinf.array_slice) < info.safe_array_size(), "TileID is out of range for the specified mip dimensions");

	uint32_t firstTileInMip = 0;
	auto mip_dim = info.dimensions;
	tileinf.mip_level = 0;
	uint32_t nthTileIntoSlice = tileid % num_tiles_per_slice; 

	if (nthTileIntoSlice > 0)
	{
		do 
		{
			mip_dim = dimensions(info.format, info.dimensions, ++tileinf.mip_level);
			firstTileInMip += num_tiles(mip_dim, tile_dimensions);

		} while (nthTileIntoSlice < firstTileInMip);
	}
	
	auto tileOffsetFromMipStart = nthTileIntoSlice - firstTileInMip;
	auto mip_tile_dim = dimensions_in_tiles(mip_dim, tile_dimensions);
	auto positionInTiles = uint2(tileOffsetFromMipStart % mip_tile_dim.x, tileOffsetFromMipStart / mip_tile_dim.y);
	tileinf.position = positionInTiles * tile_dimensions;
	return tileinf;
}

}}
