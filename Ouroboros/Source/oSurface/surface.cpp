// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oSurface/surface.h>
#include <oMath/hlsl.h>
#include <oString/stringize.h>
#include <atomic>

namespace ouro { namespace surface {

#define oSURF_CHECK(expr, format, ...) oCheck(expr, std::errc::invalid_argument, format, ## __VA_ARGS__ )
#define oSURF_CHECK_DIM(f, dim)        oCheck(dim        >= min_dimensions(f).x, std::errc::invalid_argument, "invalid dimension: %u", dim);
#define oSURF_CHECK_DIM2(f, dim)       oCheck(any(dim    >= min_dimensions(f)),  std::errc::invalid_argument, "invalid dimensions: [%u,%u]", dim.x, dim.y);
#define oSURF_CHECK_DIM3(f, dim)       oCheck(any(dim.xy >= min_dimensions(f)),  std::errc::invalid_argument, "invalid dimensions: [%u,%u,%u]", dim.x, dim.y, dim.z);

	} // namespace surface

template<> const char* as_string(const surface::cube_face& face)
{
	static const char* s_names[] =
	{
		"posx",
		"negx",
		"posy",
		"negy",
		"posz",
		"negz",
	};
	return detail::enum_as(face, s_names);
}

oDEFINE_ENUM_TO_STRING(surface::cube_face)
oDEFINE_ENUM_FROM_STRING(surface::cube_face)

template<> const char* as_string(const surface::semantic& s)
{
	static const char* s_names[] = 
	{
		"unknown",
		"color_srgb",
		"color_hdr",
		"custom",
		"specular",
		"diffuse",
		"height",
		"noise",
		"intensity",
		"tangent_normal",
		"world_normal",
		"photometric_profile",
		"custom1d",
		"color_correction1d",
		"custom3d",
		"color_correction3d",
		"customcube",
		"cube_hdr",
		"cube_srgb",
	};
	return detail::enum_as(s, s_names);
}

oDEFINE_TO_FROM_STRING(surface::semantic)

namespace surface {

uint32_t num_mips(bool mips, const uint3& mip0dimensions)
{
	// Rules of mips are to go to 1x1... so a 1024x8 texture has many more than 4
	// mips.

	if (!mips)
		return 0;

	uint32_t nmips = 1;
	uint3 mip = ::max(uint3(1), mip0dimensions);

	while (any(mip != 1))
	{
		nmips++;
		mip = ::max(uint3(1), mip / uint3(2));
	}

	return nmips;
}

uint32_t dimension(const format& f, uint32_t mip0dimension, uint32_t mip, uint32_t subsurface)
{
	oSURF_CHECK_DIM(f, mip0dimension);
	oSURF_CHECK(f != format::unknown, "Unknown surface format passed to surface::dimension");
	const uint32_t subsampleBias = subsample_bias(f, subsurface);
	uint32_t d = ::max(1u, mip0dimension >> (mip + subsampleBias));
	return is_block_compressed(f) ? static_cast<uint32_t>(align(d, 4)) : d;
}

uint2 dimensions(const format& f, const uint2& mip0dimensions, uint32_t mip, uint32_t subsurface)
{
	return uint2(dimension(f, mip0dimensions.x, mip, subsurface)
						, dimension(f, mip0dimensions.y, mip, subsurface));
}

uint3 dimensions(const format& f, const uint3& mip0dimensions, uint32_t mip, uint32_t subsurface)
{
	return uint3(dimension(f, mip0dimensions.x, mip, subsurface)
						, dimension(f, mip0dimensions.y, mip, subsurface)
						, dimension(format::r32_uint, mip0dimensions.z, mip, subsurface)); // no block-compression alignment for depth
}

uint32_t dimension_npot(const format& f, uint32_t mip0dimension, uint32_t mip, uint32_t subsurface)
{
	oSURF_CHECK_DIM(f, mip0dimension);

	format nth_surface_format = subformat(f, subsurface);
	const auto mip_bias = subsample_bias(f, subsurface);
	const auto min_dim = min_dimensions(f);

	auto d = ::max(1u, mip0dimension >> (mip + mip_bias));
	auto NPOTDim = is_block_compressed(nth_surface_format) ? static_cast<uint32_t>(align(d, 4)) : d;

	if (subsurface == 0 && subsample_bias(f, 1) != 0)
		NPOTDim = ::max(min_dim.x, NPOTDim & ~(min_dim.x-1)); // always even down to 2x2

	return NPOTDim;
}

uint2 dimensions_npot(const format& f, const uint2& mip0dimensions, uint32_t mip, uint32_t subsurface)
{
	oSURF_CHECK_DIM2(f, mip0dimensions);

	format nth_surface_format = subformat(f, subsurface);
	const auto mip_bias = subsample_bias(f, subsurface);
	const auto min_dims = min_dimensions(f);

	uint2 d;
	d.x = ::max(1u, mip0dimensions.x >> (mip + mip_bias));
	d.y = ::max(1u, mip0dimensions.y >> (mip + mip_bias));

	if (is_block_compressed(nth_surface_format))
	{
		d.x = align(d.x, 4);
		d.y = align(d.y, 4);
	}

	if (subsurface == 0 && subsample_bias(f, 1) != 0)
	{
		d.x = ::max(min_dims.x, d.x & ~(min_dims.x-1)); // always even down to 2x2
		d.y = ::max(min_dims.y, d.y & ~(min_dims.y-1)); // always even down to 2x2
	}
	return d;
}

uint3 dimensions_npot(const format& f, const uint3& mip0dimensions, uint32_t mip, uint32_t subsurface)
{
	auto dimxy = dimensions_npot(f, mip0dimensions.xy(), mip, subsurface);
	return uint3(dimxy.x, dimxy.y
		, dimension_npot(format::r32_uint, mip0dimensions.z, mip, subsurface)); // no block-compression alignment for depth
}

uint32_t row_size(const format& f, uint32_t mipwidth, uint32_t subsurface)
{
	oSURF_CHECK_DIM(f, mipwidth);
	uint32_t w = dimension(f, mipwidth, 0, subsurface);
	if (is_block_compressed(f)) // because the atom is a 4x4 block
		w /= 4;
	const auto s = element_size(f, subsurface);
	return align(w * s, s);
}

uint32_t row_pitch(const info_t& info, uint32_t mip, uint32_t subsurface)
{
	const auto nmips = num_mips(info.mip_layout, info.dimensions);
	if (nmips && mip >= nmips)
		oThrow(std::errc::invalid_argument, "invalid mip");

	switch (info.mip_layout)
	{
		case mip_layout::none: 
			return row_size(info.format, info.dimensions, subsurface);
		case mip_layout::tight: 
			return row_size(info.format, dimension_npot(info.format, info.dimensions.x, mip, subsurface), subsurface);
		case mip_layout::below: 
		{
			const auto mip0RowSize = row_size(info.format, info.dimensions.x, subsurface);
			if (nmips > 2)
			{
					return ::max(mip0RowSize, 
					row_size(info.format, dimension_npot(info.format, info.dimensions.x, 1, subsurface)) + 
					row_size(info.format, dimension_npot(info.format, info.dimensions.x, 2, subsurface)) );
			}
			else
				return mip0RowSize;
		}

		case mip_layout::right: 
		{
			const auto mip0RowSize = row_size(info.format, info.dimensions.x, subsurface);
			if (nmips > 1)
				return mip0RowSize + row_size(info.format, dimension_npot(info.format, info.dimensions.x, 1, subsurface), subsurface);
			else
				return mip0RowSize;
		}

		default: oThrow(std::errc::invalid_argument, "unexpected mip_layout %d", info.mip_layout);
	}
}

uint32_t depth_pitch(const info_t& info, uint32_t mip, uint32_t subsurface)
{
	auto mip_dimensions = dimensions_npot(info.format, info.dimensions, mip, 0);
	return row_pitch(info, mip, subsurface) * num_rows(info.format, mip_dimensions.xy(), subsurface);
}

uint32_t num_columns(const format& f, uint32_t mipwidth, uint32_t subsurface)
{
	oSURF_CHECK_DIM(f, mipwidth);
	auto widthInPixels = dimension(f, mipwidth, subsurface);
	return is_block_compressed(f) ? ::max(1u, widthInPixels/4) : widthInPixels;
}

uint32_t num_rows(const format& f, uint32_t mipheight, uint32_t subsurface)
{
	oSURF_CHECK_DIM(f, mipheight);
	auto heightInPixels = dimension_npot(f, mipheight, 0, subsurface);
	return is_block_compressed(f) ? ::max(1u, heightInPixels/4) : heightInPixels;
}

uint32_t mip_size(const format& f, const uint2& mip_dimensions, uint32_t subsurface)
{
	oSURF_CHECK_DIM2(f, mip_dimensions);
	return row_size(f, mip_dimensions, subsurface) * num_rows(f, mip_dimensions, subsurface);
}

static int offset_none(const info_t& info, uint32_t mip, uint32_t subsurface)
{
	oSURF_CHECK(mip == 0, "mip_layout::none doesn't have mip levels");
	uint32_t offset = 0;
	for (uint32_t i = 0; i < subsurface; i++)
		offset += total_size(info, i);
	return offset;
}

static uint32_t offset_tight(const info_t& info, uint32_t mip, uint32_t subsurface)
{
	auto mip0dimensions = info.dimensions;
	uint32_t offset = 0;
	uint32_t cur_mip = 0;
	while (cur_mip != mip)
	{
		auto previousmip_dimensions = dimensions_npot(info.format, mip0dimensions, cur_mip, subsurface);
		offset += mip_size(info.format, previousmip_dimensions, subsurface);
		cur_mip++;
	}
	return offset;
}

static int offset_below(const info_t& info, uint32_t mip, uint32_t subsurface)
{
	if (0 == mip)
		return 0;

	auto mip0dimensions = info.dimensions;
	auto mip1dimensions = dimensions_npot(info.format, info.dimensions, 1, subsurface);
	auto surfaceRowPitch = row_pitch(info, 0, subsurface);

	// Step down when moving from Mip0 to Mip1
	auto offset = surfaceRowPitch * num_rows(info.format, mip0dimensions, subsurface);
	if (1 == mip)
		return offset;

	// Step right when moving from Mip1 to Mip2
	offset += row_size(info.format, mip1dimensions, subsurface);

	// Step down for all of the other MIPs
	uint32_t cur_mip = 2;
	while (cur_mip != mip)
	{
		auto previousmip_dimensions = dimensions_npot(info.format, mip0dimensions, cur_mip, subsurface);
		offset += surfaceRowPitch * num_rows(info.format, previousmip_dimensions, subsurface);
		cur_mip++;
	}		

	return offset;
}

static uint32_t offset_right(const info_t& info, uint32_t mip, uint32_t subsurface)
{
	if (0 == mip)
		return 0;

	auto mip0dimensions = info.dimensions;
	auto surfaceRowPitch = row_pitch(info, 0, subsurface);

	// Step right when moving from Mip0 to Mip1
	auto offset = row_size(info.format, mip0dimensions, subsurface);

	// Step down for all of the other MIPs
	uint32_t cur_mip = 1;
	while (cur_mip != mip)
	{
		auto previousmip_dimensions = dimensions_npot(info.format, mip0dimensions, cur_mip, subsurface);
		offset += surfaceRowPitch * num_rows(info.format, previousmip_dimensions, subsurface);
		cur_mip++;
	}		

	return offset;
}

uint32_t offset(const info_t& info, uint32_t mip, uint32_t subsurface)
{
	const auto nmips = num_mips(info.mip_layout, info.dimensions);
	if (nmips && mip >= nmips) 
		oThrow(std::errc::invalid_argument, "invalid mip");

	switch (info.mip_layout)
	{
		case mip_layout::none: return offset_none(info, mip, subsurface);
		case mip_layout::tight: return offset_tight(info, mip, subsurface);
		case mip_layout::below: return offset_below(info, mip, subsurface);
		case mip_layout::right: return offset_right(info, mip, subsurface);
		default: oThrow(std::errc::invalid_argument, "unexpected mip_layout %d", info.mip_layout);
	}
}

uint32_t slice_pitch(const info_t& info, uint32_t subsurface)
{
	uint32_t pitch = 0;

	switch (info.mip_layout)
	{
		case mip_layout::none: case mip_layout::right: case mip_layout::below:
			return mip_size(info.format, slice_dimensions(info, 0), subsurface);

		case mip_layout::tight:
		{
			// Sum the size of all mip levels
			int3 dimensions = info.dimensions;
			int nmips = num_mips(info.mip_layout, dimensions);
			while (nmips > 0)
			{
				pitch += mip_size(info.format, dimensions.xy(), subsurface) * dimensions.z;
				dimensions = ::max(int3(1,1,1), dimensions / int3(2,2,2));
				nmips--;
			}

			// Align slicePitch to mip0RowPitch
			const uint32_t mip0RowPitch = row_pitch(info, 0, subsurface);
			pitch = (((pitch + (mip0RowPitch - 1)) / mip0RowPitch) * mip0RowPitch);
			break;
		}
		default: oThrow(std::errc::invalid_argument, "unexpected mip_layout %d", info.mip_layout);
	}

	return pitch;
}

uint32_t total_size(const info_t& info)
{
	uint32_t size = 0;
	const int nSurfaces = num_subformats(info.format);
	for (int i = 0; i < nSurfaces; i++)
	{
		// align is needed here to avoid a memory corruption crash. I'm not 
		// sure why it is needed, but I think that size is a memory structure 
		// containing all surface sizes, so they are all expected to be aligned.
		size += total_size(info, i);
	}
	return size;
}

uint32_t total_size(const info_t& info, uint32_t subsurface)
{
	return slice_pitch(info, subsurface) * info.safe_array_size();
}

uint2 dimensions(const info_t& info, uint32_t subsurface)
{
	auto slice_dim = slice_dimensions(info, subsurface);
	return uint2(slice_dim.x, slice_dim.y * info.safe_array_size());
}

uint2 slice_dimensions(const info_t& info, uint32_t subsurface)
{
	auto mip0dimensions = dimensions_npot(info.format, info.dimensions, 0, subsurface);
	switch (info.mip_layout)
	{
		case mip_layout::none:
			return uint2(mip0dimensions.x, (mip0dimensions.y * mip0dimensions.z));
		
		case mip_layout::tight:
		{
			const auto surfaceSlicePitch = slice_pitch(info, subsurface);
			const auto mip0RowPitch = row_pitch(info, 0, subsurface);
			return uint2(mip0dimensions.x, (surfaceSlicePitch / mip0RowPitch));
		}
		case mip_layout::below: 
		{
			auto nmips = num_mips(info.mip_layout, mip0dimensions);
			auto mip1dimensions = nmips > 1 ? dimensions_npot(info.format, mip0dimensions, 1, subsurface) : uint3(0);
			auto mip2dimensions = nmips > 2 ? dimensions_npot(info.format, mip0dimensions, 2, subsurface) : uint3(0);

			auto mip0height = mip0dimensions.y * mip0dimensions.z;
			auto mip1height = mip1dimensions.y * mip1dimensions.z;
			auto mip2andUpHeight = mip2dimensions.y * mip2dimensions.z;
			for (uint32_t mip = 3; mip < nmips; mip++)
			{
				auto mipn_dim = dimensions_npot(info.format, mip0dimensions, mip, subsurface);
				mip2andUpHeight += mipn_dim.y * mipn_dim.z;
			}
			return uint2(::max(mip0dimensions.x, mip1dimensions.x + mip2dimensions.x), (mip0height + ::max(mip1height, mip2andUpHeight)));
		}
		case mip_layout::right: 
		{
			auto nmips = num_mips(info.mip_layout, mip0dimensions);
			auto mip1dimensions = nmips > 1 ? dimensions_npot(info.format, mip0dimensions, 1, subsurface) : int3(0);

			auto mip0height = mip0dimensions.y * mip0dimensions.z;
			auto mip1andUpHeight = mip1dimensions.y * mip1dimensions.z;
			for (uint32_t mip = 2; mip < nmips; mip++)
			{
				auto mipn_dim = dimensions_npot(info.format, mip0dimensions, mip, subsurface);
				mip1andUpHeight += mipn_dim.y * mipn_dim.z;
			}
			return uint2(mip0dimensions.x + mip1dimensions.x, ::max(mip0height, mip1andUpHeight));
		}
		default: oThrow(std::errc::invalid_argument, "unexpected mip_layout %d", info.mip_layout);
	}
}

info_t subsurface(const info_t& info, uint32_t subsurface, uint32_t mip, uint2* out_byte_dimensions)
{
	info_t subresource_info;
	subresource_info.dimensions = dimensions_npot(info.format, info.dimensions, mip, subsurface);
	subresource_info.array_size = info.array_size;
	subresource_info.format = subformat(info.format, subsurface);
	subresource_info.mip_layout = info.mip_layout;
	if (out_byte_dimensions)
	{
		out_byte_dimensions->x = row_size(info.format, info.dimensions);
		out_byte_dimensions->y = num_rows(info.format, info.dimensions);
	}
	return info;
}

}}
