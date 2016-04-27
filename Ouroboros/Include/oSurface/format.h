// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Definition of modern relevant texture/image/3D data formats and related introspection.

// === BC SUMMARY ===
//
// Format  Precision    Bytes/Pixel  Recommended Use
// BC1     R5G6B5 + A1  0.5          Color w/ 1bit alpha, low-precision normal maps
// BC2     R5G6B5 + A4  1            ?
// BC3     R5G6B5 + A8  1            Color w/ full alpha or packed grayscale
// BC4     R8           0.5          Height/gloss maps, font atlases, any grayscale
// BC5     R8G8         1            Tangent-space normal maps or packed grayscales
// BC6     R16G16B16    1            HDR images 
// BC7     R8G8B8(A8)   1            High-quality color maps (w/ alpha)

#pragma once
#include <oCore/fourcc.h>
#include <oMath/hlsl.h>
#include <cstdint>

namespace ouro { namespace surface {

enum class format : uint8_t
{
	unknown,
	r32g32b32a32_typeless,
	r32g32b32a32_float,
	r32g32b32a32_uint,
	r32g32b32a32_sint,
	r32g32b32_typeless,
	r32g32b32_float,
	r32g32b32_uint,
	r32g32b32_sint,
	r16g16b16a16_typeless,
	r16g16b16a16_float,
	r16g16b16a16_unorm,
	r16g16b16a16_uint,
	r16g16b16a16_snorm,
	r16g16b16a16_sint,
	r32g32_typeless,
	r32g32_float,
	r32g32_uint,
	r32g32_sint,
	r32g8x24_typeless,
	d32_float_s8x24_uint,
	r32_float_x8x24_typeless,
	x32_typeless_g8x24_uint,
	r10g10b10a2_typeless,
	r10g10b10a2_unorm,
	r10g10b10a2_uint,
	r11g11b10_float,
	r8g8b8a8_typeless,
	r8g8b8a8_unorm,
	r8g8b8a8_unorm_srgb,
	r8g8b8a8_uint,
	r8g8b8a8_snorm,
	r8g8b8a8_sint,
	r16g16_typeless,
	r16g16_float,
	r16g16_unorm,
	r16g16_uint,
	r16g16_snorm,
	r16g16_sint,
	r32_typeless,
	d32_float,
	r32_float,
	r32_uint,
	r32_sint,
	r24g8_typeless,
	d24_unorm_s8_uint,
	r24_unorm_x8_typeless,
	x24_typeless_g8_uint,
	r8g8_typeless,
	r8g8_unorm,
	r8g8_uint,
	r8g8_snorm,
	r8g8_sint,
	r16_typeless,
	r16_float,
	d16_unorm,
	r16_unorm,
	r16_uint,
	r16_snorm,
	r16_sint,
	r8_typeless,
	r8_unorm,
	r8_uint,
	r8_snorm,
	r8_sint,
	a8_unorm,
	r1_unorm,
	r9g9b9e5_sharedexp,
	r8g8_b8g8_unorm,
	g8r8_g8b8_unorm,
	bc1_typeless,
	bc1_unorm,
	bc1_unorm_srgb,
	bc2_typeless,
	bc2_unorm,
	bc2_unorm_srgb,
	bc3_typeless,
	bc3_unorm,
	bc3_unorm_srgb,
	bc4_typeless,
	bc4_unorm,
	bc4_snorm,
	bc5_typeless,
	bc5_unorm,
	bc5_snorm,
	b5g6r5_unorm,
	b5g5r5a1_unorm,
	b8g8r8a8_unorm,
	b8g8r8x8_unorm,
	r10g10b10_xr_bias_a2_unorm,
	b8g8r8a8_typeless,
	b8g8r8a8_unorm_srgb,
	b8g8r8x8_typeless,
	b8g8r8x8_unorm_srgb,
	bc6h_typeless,
	bc6h_uf16,
	bc6h_sf16,
	bc7_typeless,
	bc7_unorm,
	bc7_unorm_srgb,
	ayuv,
	y410,
	y416,
	nv12,
	p010,
	p016,
	opaque_420,
	yuy2,
	y210,
	y216,
	nv11,
	ai44,
	ia44,
	p8,
	a8p8,
	b4g4r4a4_unorm,

	// formats below here are not currently directly loadable to directx.
	r8g8b8_unorm,
	r8g8b8_unorm_srgb,
	r8g8b8x8_unorm,
	r8g8b8x8_unorm_srgb,
	b8g8r8_unorm,
	b8g8r8_unorm_srgb,
	a8b8g8r8_unorm,
	a8b8g8r8_unorm_srgb,
	x8b8g8r8_unorm,
	x8b8g8r8_unorm_srgb,

	// multi-surface yuv formats (for emulation ahead of hw and for more robust 
	// compression); also not currently directly loadable to directx.
	y8_u8_v8_unorm, // one r8_unorm per channel
	y8_a8_u8_v8_unorm, // one r8_unorm per channel
	ybc4_ubc4_vbc4_unorm, // one bc4_unorm per channel
	ybc4_abc4_ubc4_vbc4_unorm, // one bc4_unorm per channel
	y8_u8v8_unorm, // y: r8_unorm uv: r8g8_unorm (half-res)
	y8a8_u8v8_unorm, // ay: r8g8_unorm uv: r8g8_unorm (half-res)
	ybc4_uvbc5_unorm, // y: bc4_unorm uv: bc5_unorm (half-res)
	yabc5_uvbc5_unorm, // ay: bc5_unorm uv: bc5_unorm (half-res)
	
	count,
};

struct bit_size
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
};

static const uint32_t max_num_subsurfaces = 4;

inline bool is_texture         (const format& f) { return (int)f <= (int)format::b4g4r4a4_unorm; }
bool        is_block_compressed(const format& f);                          // true if a block-compressed format
bool        is_typeless        (const format& f);                          // true if a typeless fomrat
bool        is_depth           (const format& f);                          // true if typically used to write Z-buffer/depth information
bool        has_alpha          (const format& f);                          // true if has alpha - rgbx types do not have alpha
bool        is_unorm           (const format& f);                          // true if normalized between 0.0f and 1.0f
bool        is_srgb            (const format& f);                          // true if srgb format
bool        is_planar          (const format& f);                          // true if channels of a pixel are not interleaved
bool        is_yuv             (const format& f);                          // true if in YUV space
format      as_srgb            (const format& f);                          // returns the base type with the specified extension
format      as_depth           (const format& f);                          // returns the base type with the specified extension
format      as_typeless        (const format& f);                          // returns the base type with the specified extension
format      as_unorm           (const format& f);                          // returns the base type with the specified extension
format      as_4chanx          (const format& f);                          // adds a 4th component or replaces an alpha type with an x type
format      as_4chana          (const format& f);                          // adds a 4th component or replaces an x type with an alpha type
format      as_3chan           (const format& f);                          // removes a 4th component
format      as_texture         (const format& f);                          // return the closest format that would return true from is_texture()
format      as_nv12            (const format& f);                          // given a surface format determine the NV12 format that comes closest to it
bit_size    channel_bits       (const format& f);                          // returns number of bits per channel: either r,g,b,a or y,u,v,a
uint32_t    num_channels       (const format& f);                          // returns the number of separate channels used for a pixel
uint32_t    num_subformats     (const format& f);                          // returns the number of separate plane's for planar formats
uint32_t    bits               (const format& f);                          // returns number of bits for an element including X bits
uint2       min_dimensions     (const format& f);                          // typically 1,1 but block formats and yuv may differ
uint32_t    subsample_bias     (const format& f, uint32_t subsurface);     // returns the offset to be applied to the main surface's mip level when inferring the size of a subsurface (think of the UV size in many YUV formats).
uint32_t    element_size       (const format& f, uint32_t subsurface = 0); // size in bytes of either the pixel or bc block. For 1-bit textures this returns 1 (byte)
format      subformat          (const format& f, uint32_t subsurface);     // returns the surface format of the nth subsurface (plane) or unknown for non-planar formats
fourcc_t    to_fourcc          (const format& f);                          // get the typical fourcc code associated with the format
format      from_fourcc        (const fourcc_t& fcc);                      // converts a fourcc returned from to_fourcc to a format

}}
