// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Utility code for images, textures, pictures, bitmaps, etc.
//
//
// === DEFINITIONS ===
//
// SURFACE: The single buffer bound to a modern hardware texture sampler or a 
// SW emulator of such hardware. A surface can also include other 1D series of 
// regular elements such as vertex or index arrays.
//
// SUBSURFACE: A subsurface is a secondary buffer often representing the down-
// sampled components of yuv. The information about such subsurfaces always 
// relates to the main surface, so the api reflect that. Often this will not 
// be used.
//
// SUBRESOURCE: an index into the repeating pattern of slices that contain mip
// chains. This encapsulates the two slice/mip indices for convenience.
//
// BYTE DIMENSIONS: A uint2 containing row_pitch in x and row count in y.
//
// ELEMENT: The 'pixel' or a 4x4 block for block-compressed formats.
//
// CHANNEL: One scalar in an element, i.e. the R or G channel of an RGB element.
//
// MIP LEVEL: A 2D subregion of a surface that most closely resembles an image
// i.e. it is the 2D subregion that contains color data for a particular usage.
//
// MIP CHAIN: All mip levels from the full-sized one at mip0 to a 1x1x1 mip 
// level. Each level in between is 1/2 the size of its predecessor.
//
// ROW PITCH: Bytes to the next scanline of a given mip level.
//
// ROW SIZE: Bytes to copy when copying a scanline. Size != pitch, so be careful 
// to separate the concepts.
//
// SLICE: Array of images adjacent in memory at the same dimensions, like an 
// array of images without a mip chain. This is primarily for 3D surfaces.
//
// ARRAY: A series of mip chains (See SLICE for contrast).
//
// DEPTH PITCH: Bytes to the next depth "slice" within a given
// mip level.
//
// POSITION: Pixel coordinates from the upper left corner into a mip level where 
// the upper left of a tile begins.
//
// TILE: A 2D subregion of a mip level. A tile is the atom on which surface 
// streaming might operate and the size of a tile is often choosen to balance the 
// workload of decompressing or synthesizing a tile vs. how fine that processing 
// gets divided amongst threads.

#pragma once
#include <oCore/byte.h>
#include <oCore/fourcc.h>
#include <oMath/hlsl.h>
#include <oSurface/format.h>

namespace ouro { namespace surface {

enum class mip_layout : uint8_t
{
	// http://www.x.org/docs/intel/VOL_1_graphics_core.pdf
	// none : no mip chain, so row size == row pitch
	// tight: mips are right after each other: row size == row pitch and 
	//        the end of one mip is the start of the next.
	// below: Step down when moving from mip0 to mip1
	//        Step right when moving from mip1 to mip2
	//        Step down for all of the other mips
	//        row pitch is generally the next-scanline stride of mip0, 
	//        except for alignment issues e.g. with mip0 width of 1,
	//        or mip0 width <= 4 for block compressed formats.
	// right: Step right when moving from mip0 to mip1
	//        Step down for all of the other mips.
	//        row pitch is generally the next-scanline stride of mip0 
	//        plus the stride of mip1.
	// Note that for below and right the lowest mip can be below the bottom
	// line of mip1 and mip0 respectively for alignment reasons and/or block 
	// compressed formats.
	// None always has mip0, others have all mips down to 1x1.
	// +---------+ +---------+ +---------+ +---------+----+
	// |         | |         | |         | |         |    |
	// |  none   | |  Tight  | |  Below  | |  Right  |    |
	// |         | |         | |         | |         +--+-+
	// |         | |         | |         | |         |  |  
	// |         | |         | |         | |         +-++  
	// +---------+ +----+----+ +----+--+-+ +---------+++  
	//             |    |      |    |  |             ++
	//             |    |      |    +-++                  
	//             +--+-+      +----+++                    
	//             |  |             ++                    
	//             +-++                                 
	//             +++                                  
	//             ++

	none,
	tight,
	below,
	right,
};

enum class semantic : uint8_t
{
	unknown,

	// texture semantics
	color_srgb,
	color_hdr,
	custom,

	specular,
	diffuse,
	height,
	noise,
	intensity,
	tangent_normal,
	world_normal,
	photometric_profile,

	custom1d,
	color_correction1d,

	custom3d,
	color_correction3d,

	customcube,
	cube_hdr, // IBL
	cube_srgb,

	count,

	first1d   = custom1d,
	last1d    = color_correction1d,
	first2d   = custom,
	last2d    = world_normal,
	first3d   = custom3d,
	last3d    = color_correction3d,
	firstcube = customcube,
	lastcube  = cube_srgb,
};

enum class cube_face : uint8_t
{
	posx,
	negx,
	posy,
	negy,
	posz,
	negz,

	count,
};

enum class copy_option : uint8_t
{
	none,
	flip_vertically,
};

struct info_t
{
	info_t()
		: dimensions(0, 0, 0)
		, array_size(0)
		, format(format::unknown)
		, semantic(semantic::custom)
		, mip_layout(mip_layout::none)
		, pad(0)
	{}

	inline bool operator==(const info_t& that) const
	{
		return dimensions.x == that.dimensions.x
				&& dimensions.y == that.dimensions.y
				&& dimensions.z == that.dimensions.z
				&& array_size == that.array_size
				&& format == that.format
				&& semantic == that.semantic
				&& mip_layout == that.mip_layout;
	}

	inline bool is_1d() const { return semantic >= semantic::first1d && semantic <= semantic::last1d; }
	inline bool is_2d() const { return semantic >= semantic::first2d && semantic <= semantic::last2d; }
	inline bool is_3d() const { return semantic >= semantic::first3d && semantic <= semantic::last3d; }
	inline bool is_cube() const { return semantic >= semantic::firstcube && semantic <= semantic::lastcube; }
	inline bool is_array() const { return array_size != 0; }
	inline bool mips() const { return mip_layout != mip_layout::none; }
	inline bool is_normal_map() const { return semantic == semantic::tangent_normal || semantic == semantic::world_normal; }

	// 0 indicates the absence of the dimension, however when iterating in loops it's useful to ensure 
	// at least one cycle through.
	inline uint32_t safe_depth() const { return max(1u, dimensions.z); }
	inline uint32_t safe_array_size() const { return max(1u, array_size); }

	uint3 dimensions;
	uint32_t array_size;
	format format;
	semantic semantic;
	mip_layout mip_layout;
	uint8_t pad;
};
static_assert(sizeof(info_t) == 20, "size mismatch. This object is serialized in surface::image; a version change will be necessary.");

// returns 0 if mips is false, else the total number of mips
uint32_t num_mips(bool mips, const uint3& mip0dimensions);
inline uint32_t num_mips(bool mips, const uint2& mip0dimensions) { return num_mips(mips, uint3(mip0dimensions, 1)); }
inline uint32_t num_mips(const mip_layout& layout, const uint3& mip0dimensions) { return num_mips(layout != mip_layout::none, mip0dimensions); }
inline uint32_t num_mips(const mip_layout& layout, const uint2& mip0dimensions) { return num_mips(layout != mip_layout::none, uint3(mip0dimensions, 1)); }
inline uint32_t num_mips(const info_t& info) { return num_mips(info.mip_layout, info.dimensions); }

// returns dimensions for a specific mip level given mip0's dimension, appropriately padding
// BC formats. This assumes power of 2 sizes.
uint32_t dimension(const format& f, uint32_t mip0dimensions, uint32_t mip = 0, uint32_t subsurface = 0);
uint2 dimensions(const format& f, const uint2& mip0dimensions, uint32_t mip = 0, uint32_t subsurface = 0);
uint3 dimensions(const format& f, const uint3& mip0dimensions, uint32_t mip = 0, uint32_t subsurface = 0);

// calculate mip dimensions for non-power-of-2 textures using the D3D/OGL2.0
// floor convention: http://www.opengl.org/registry/specs/ARB/texture_non_power_of_two.txt
uint32_t dimension_npot(const format& f, uint32_t mip0dimensions, uint32_t mip = 0, uint32_t subsurface = 0);
uint2 dimensions_npot(const format& f, const uint2& mip0dimensions, uint32_t mip = 0, uint32_t subsurface = 0);
uint3 dimensions_npot(const format& f, const uint3& mip0dimensions, uint32_t mip = 0, uint32_t subsurface = 0);

// returns the number of bytes for one row of valid data excluding padding but validly handling BC formats
uint32_t row_size(const format& f, uint32_t mipwidth, uint32_t subsurface = 0);
inline uint32_t row_size(const format& f, const uint2& mip_dimensions, uint32_t subsurface = 0) { return row_size(f, mip_dimensions.x, subsurface); }
inline uint32_t row_size(const format& f, const uint3& mip_dimensions, uint32_t subsurface = 0) { return row_size(f, mip_dimensions.x, subsurface); }

// returns the number of bytes to increment to get to the next row of a 2d 
// surface supporting padding and BC formats
uint32_t row_pitch(const info_t& info, uint32_t mip = 0, uint32_t subsurface = 0);

// returns the number of bytes to increment to get to the next slice of a 3d surface
uint32_t depth_pitch(const info_t& info, uint32_t mip = 0, uint32_t subsurface = 0);

// returns number of columns (number of row elements) in a mip level with the specified 
// width in pixels. Block compressed formats will return 1/4 the columns since their 
// atomic element - the block - is 4x4 pixels.
uint32_t num_columns(const format& f, uint32_t mipwidth, uint32_t subsurface = 0);
inline uint32_t num_columns(const format& f, const uint2& mip_dimensions, uint32_t subsurface = 0) { return num_columns(f, mip_dimensions.x, subsurface); }

// returns the number of rows in a mip level with the specified height in pixels. BC formats
// have 1/4 the rows since their pitch includes 4 rows at a time.
uint32_t num_rows(const format& f, uint32_t mipheight, uint32_t subsurface = 0);
inline uint32_t num_rows(const format& f, const uint2& mip_dimensions, uint32_t subsurface = 0) { return num_rows(f, mip_dimensions.y, subsurface); }
inline uint32_t num_rows(const format& f, const uint3& mip_dimensions, uint32_t subsurface = 0) { return num_rows(f, mip_dimensions.y, subsurface) * max(1u, mip_dimensions.z); }

// returns the number of columns (x) and rows (y) in one call. For BC formats this is
// in 4x4 blocks, not pixels.
inline uint2 num_columns_and_rows(const format& f, const uint2& mip_dimensions, uint32_t subsurface = 0) { return uint2(num_columns(f, mip_dimensions, subsurface), num_rows(f, mip_dimensions, subsurface)); }

// returns row_pitch() and num_rows() in x and y
inline uint2 byte_dimensions(const format& f, const uint2& mip_dimensions, uint32_t subsurface = 0) { return uint2(row_size(f, mip_dimensions, subsurface), num_rows(f, mip_dimensions, subsurface)); }
inline uint2 byte_dimensions(const format& f, const uint3& mip_dimensions, uint32_t subsurface = 0) { return uint2(row_size(f, mip_dimensions, subsurface), num_rows(f, mip_dimensions, subsurface)); }

// returns the size in bytes for the specified mip level, not considering padding
uint32_t mip_size(const format& f, const uint2& mip_dimensions, uint32_t subsurface = 0);
inline uint32_t mip_size(const format& f, const uint3& mip_dimensions, uint32_t subsurface = 0) { return mip_size(f, mip_dimensions.xy(), subsurface) * mip_dimensions.z; }

// returns the bytes from the start of mip0 where the specified mip's data begins
uint32_t offset(const info_t& info, uint32_t mip = 0, uint32_t subsurface = 0);

// Returns the size in bytes for one slice for the described mip chain.
// That is, in an array of textures each texture is a full mip chain and this
// pitch is the number of bytes for the total mip chain of one of those textures. 
// This is the same as calculating the size of a mip page as described by layout. 
// For 3d textures, make sure that array_size is set to 1 and use depth to supply 
// the size in the 3rd dimension.
uint32_t slice_pitch(const info_t& info, uint32_t subsurface = 0);

// Returns the size for a total buffer of 1d/2d/3d/cube textures by summing 
// the various mip chains then multiplying it by the number of slices. 
// Optionally you can supply a subsurface index to limit the size calculation to 
// that subsurface only. For more clarity on the difference between array_size 
// and depth see: http://www.cs.umbc.edu/~olano/s2006c03/ch02.pdf where it is 
// clear that when a first mip level for a 3d texture is (4,3,5) that the next 
// mip level is (2,1,2) and the next (1,1,1) whereas array_size set to 5 would 
// mean: 5*(4,3), 5*(2,1), 5*(1,1).
uint32_t total_size(const info_t& info);
uint32_t total_size(const info_t& info, uint32_t subsurface);

// returns the dimensions you would need for an image to fit this surface
// natively and in its entirety.
uint2 dimensions(const info_t& info, uint32_t subsurface = 0);

// returns the dimensions you would need for an image to fit a slice of this 
// surface natively and in its entirety.
uint2 slice_dimensions(const info_t& info, uint32_t subsurface = 0);

// returns an info_t for the nth subsurfaces mip
info_t subsurface(const info_t& info, uint32_t subsurface, uint32_t mip, uint2* out_byte_dimensions = nullptr);

}}
