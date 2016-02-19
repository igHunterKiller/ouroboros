// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oCore/color.h>
#include <oSurface/convert.h>
#include <oMath/hlslx.h>
#include <oMath/quantize.h>
#include <oMemory/memory.h>
#include <oString/stringize.h>
#include <ispc_texcomp.h>

#define oSURF_CHECK(expr, format, ...) oCheck(expr, std::errc::invalid_argument, format, ## __VA_ARGS__)

namespace ouro { namespace surface {

struct half2 { uint16_t x,y; const half2& operator=(const float2& that) { x = f32tof16(that.x); y = f32tof16(that.y); return *this; }};
struct half3 { uint16_t x,y,z; const half3& operator=(const float3& that) { x = f32tof16(that.x); y = f32tof16(that.y); z = f32tof16(that.z); return *this; }};
struct half4 { uint16_t x,y,z,w; const half4& operator=(const float4& that) { x = f32tof16(that.x); y = f32tof16(that.y); z = f32tof16(that.z); w = f32tof16(that.w); return *this; }};
struct udec3 { uint32_t xyzw; };

// Here pitch refers to element pitch, or the size of a texel
#define ELCPY_PARAMS_VOID void* oRESTRICT dst, size_t dst_elem_pitch, const void* oRESTRICT src, size_t src_elem_pitch, size_t num_elements
#define ELCPY_PARAMS(srcT, dstT) dstT* oRESTRICT dst, size_t dst_elem_pitch, const srcT* oRESTRICT src, size_t src_elem_pitch, size_t num_elements
#define ELCPY_PARAMS_UINT8 ELCPY_PARAMS(uint8_t, uint8_t)

typedef void (*elcpy_fn)(ELCPY_PARAMS_VOID);

static void copy_3bytes(ELCPY_PARAMS_UINT8)
{
	while (num_elements--)
	{
		dst[0] = src[0];
		dst[1] = src[1];
		dst[2] = src[2];
		dst += dst_elem_pitch;
		src += src_elem_pitch;
	}
}

static void copy_3bytes_alpha(ELCPY_PARAMS_UINT8)
{
	while (num_elements--)
	{
		dst[0] = src[0];
		dst[1] = src[1];
		dst[2] = src[2];
		dst[3] = 0xff;
		dst += dst_elem_pitch;
		src += src_elem_pitch;
	}
}

static void copy_3bytes_alpha_swap_red_blue(ELCPY_PARAMS_UINT8)
{
	while (num_elements--)
	{
		dst[0] = src[2];
		dst[1] = src[1];
		dst[2] = src[0];
		dst[3] = 0xff;
		dst += dst_elem_pitch;
		src += src_elem_pitch;
	}
}

static void copy_alpha_3bytes(ELCPY_PARAMS_UINT8)
{
	while (num_elements--)
	{
		dst[0] = 0xff;
		dst[1] = src[0];
		dst[2] = src[1];
		dst[3] = src[2];
		dst += dst_elem_pitch;
		src += src_elem_pitch;
	}
}

static void copy_skip_alpha_3bytes(ELCPY_PARAMS_UINT8)
{
	while (num_elements--)
	{
		dst[0] = src[1];
		dst[1] = src[2];
		dst[2] = src[3];
		dst += dst_elem_pitch;
		src += src_elem_pitch;
	}
}

static void copy_4bytes_swap_rotate_alpha(ELCPY_PARAMS_UINT8)
{
	while (num_elements--)
	{
		dst[0] = src[1];
		dst[1] = src[2];
		dst[2] = src[3];
		dst[3] = src[0];
		dst += dst_elem_pitch;
		src += src_elem_pitch;
	}
}

static void copy_3bytes_swap_red_blue(ELCPY_PARAMS_UINT8)
{
	while (num_elements--)
	{
		dst[0] = src[2];
		dst[1] = src[1];
		dst[2] = src[0];
		dst += dst_elem_pitch;
		src += src_elem_pitch;
	}
}

static void copy_4bytes_swap_red_blue(ELCPY_PARAMS_UINT8)
{
	while (num_elements--)
	{
		dst[0] = src[2];
		dst[1] = src[1];
		dst[2] = src[0];
		dst[3] = src[3];
		dst += dst_elem_pitch;
		src += src_elem_pitch;
	}
}

static void copy_rgb_to_lum(ELCPY_PARAMS_UINT8)
{
	while (num_elements--)
	{
		argb_channels ch;
		ch.r = src[0];
		ch.g = src[1];
		ch.b = src[2];
		ch.a = 0xff;

		float4 srgb = truetofloat4(ch.argb);
		float lum = srgbtolum(srgb.xyz());
		dst[0] = (uint8_t)f32ton8(saturate(lum));
		dst += dst_elem_pitch;
		src += src_elem_pitch;
	}
}

static void copy_bgr_to_lum(ELCPY_PARAMS_UINT8)
{
	while (num_elements--)
	{
		argb_channels ch;
		ch.r = src[2];
		ch.g = src[1];
		ch.b = src[0];
		ch.a = 0xff;

		float4 srgb = truetofloat4(ch.argb);
		float lum = srgbtolum(srgb.xyz());
		dst[0] = (uint8_t)f32ton8(saturate(lum));
		dst += dst_elem_pitch;
		src += src_elem_pitch;
	}
}

static void copy_float_to_r8(ELCPY_PARAMS(float, uint8_t))
{
	const uint8_t* oRESTRICT end = byte_add(dst, dst_elem_pitch * num_elements);
	while (dst < end)
	{
		*dst = (uint8_t)f32ton8(*src);
		dst = byte_add(dst, dst_elem_pitch);
		src = byte_add(src, src_elem_pitch);
	}
}

static void copy_float2_to_half2(ELCPY_PARAMS(float2, half2))
{
	const half2* oRESTRICT end = byte_add(dst, dst_elem_pitch * num_elements);
	while (dst < end)
	{
		dst->x = f32tof16(src->x);
		dst->y = f32tof16(src->y);
		dst = byte_add(dst, dst_elem_pitch);
		src = byte_add(src, src_elem_pitch);
	}
}

static void copy_half2_to_float2(ELCPY_PARAMS(half2, float2))
{
	const float2* oRESTRICT end = byte_add(dst, dst_elem_pitch * num_elements);
	while (dst < end)
	{
		dst->x = f16tof32(src->x);
		dst->y = f16tof32(src->y);
		dst = byte_add(dst, dst_elem_pitch);
		src = byte_add(src, src_elem_pitch);
	}
}

static void copy_half2_to_float3(ELCPY_PARAMS(half2, float3))
{
	const float3* oRESTRICT end = byte_add(dst, dst_elem_pitch * num_elements);
	while (dst < end)
	{
		dst->x = f16tof32(src->x);
		dst->y = f16tof32(src->y);
		dst->y = 0.0f;
		dst = byte_add(dst, dst_elem_pitch);
		src = byte_add(src, src_elem_pitch);
	}
}

static void copy_float4_to_3(ELCPY_PARAMS(float4, float3))
{
	const float3* oRESTRICT end = byte_add(dst, dst_elem_pitch * num_elements);
	while (dst < end)
	{
		dst->x = src->x;
		dst->y = src->y; 
		dst->z = src->z;
		dst = byte_add(dst, dst_elem_pitch);
		src = byte_add(src, src_elem_pitch);
	}
}

static void copy_float3_to_2(ELCPY_PARAMS(float3, float2))
{
	const float2* oRESTRICT end = byte_add(dst, dst_elem_pitch * num_elements);
	while (dst < end)
	{
		dst->x = src->x;
		dst->y = src->y; 
		dst = byte_add(dst, dst_elem_pitch);
		src = byte_add(src, src_elem_pitch);
	}
}

static void copy_float2_to_3(ELCPY_PARAMS(float2, float3))
{
	const float3* oRESTRICT end = byte_add(dst, dst_elem_pitch * num_elements);
	while (dst < end)
	{
		dst->x = src->x;
		dst->y = src->y;
		dst->z = 0.0f;
		dst = byte_add(dst, dst_elem_pitch);
		src = byte_add(src, src_elem_pitch);
	}
}

static void copy_ushort2_to_float2(ELCPY_PARAMS(ushort2, float2))
{
	const float2* oRESTRICT end = byte_add(dst, dst_elem_pitch * num_elements);
	while (dst < end)
	{
		dst->x = n16tof32(src->x);
		dst->y = n16tof32(src->y);
		dst = byte_add(dst, dst_elem_pitch);
		src = byte_add(src, src_elem_pitch);
	}
}

static void copy_float3_to_ushort4(ELCPY_PARAMS(float3, ushort4))
{
	const ushort4* oRESTRICT end = byte_add(dst, dst_elem_pitch * num_elements);
	while (dst < end)
	{
		dst->x = (uint16_t)f32ton16(src->x);
		dst->y = (uint16_t)f32ton16(src->y); 
		dst->z = (uint16_t)f32ton16(src->z);
		dst->w = 65535;//f32ton16(1.0f);
		dst = byte_add(dst, dst_elem_pitch);
		src = byte_add(src, src_elem_pitch);
	}
}

static void copy_ushort4_to_float3(ELCPY_PARAMS(ushort4, float3))
{
	const float3* oRESTRICT end = byte_add(dst, dst_elem_pitch * num_elements);
	while (dst < end)
	{
		dst->x = n16tof32(src->x);
		dst->y = n16tof32(src->y); 
		dst->z = n16tof32(src->z);
		dst = byte_add(dst, dst_elem_pitch);
		src = byte_add(src, src_elem_pitch);
	}
}

static void copy_float3_to_half4(ELCPY_PARAMS(float3, half4))
{
	const half4* oRESTRICT end = byte_add(dst, dst_elem_pitch * num_elements);
	while (dst < end)
	{
		dst->x = f32tof16(src->x);
		dst->y = f32tof16(src->y);
		dst->z = f32tof16(src->z);
		dst->w = f32tof16(0.0f);
		dst = byte_add(dst, dst_elem_pitch);
		src = byte_add(src, src_elem_pitch);
	}
}

static void copy_half4_to_float3(ELCPY_PARAMS(half4, float3))
{
	const float3* oRESTRICT end = byte_add(dst, dst_elem_pitch * num_elements);
	while (dst < end)
	{
		dst->x = f16tof32(src->x);
		dst->y = f16tof32(src->y);
		dst->z = f16tof32(src->z);
		dst = byte_add(dst, dst_elem_pitch);
		src = byte_add(src, src_elem_pitch);
	}
}

static void copy_float4_to_half4(ELCPY_PARAMS(float4, half4))
{
	const half4* oRESTRICT end = byte_add(dst, dst_elem_pitch * num_elements);
	while (dst < end)
	{
		dst->x = f32tof16(src->x);
		dst->y = f32tof16(src->y);
		dst->z = f32tof16(src->z);
		dst->w = f32tof16(src->w);
		dst = byte_add(dst, dst_elem_pitch);
		src = byte_add(src, src_elem_pitch);
	}
}

static void copy_half4_to_float4(ELCPY_PARAMS(half4, float4))
{
	const float4* oRESTRICT end = byte_add(dst, dst_elem_pitch * num_elements);
	while (dst < end)
	{
		dst->x = f16tof32(src->x);
		dst->y = f16tof32(src->y);
		dst->z = f16tof32(src->z);
		dst->w = f16tof32(src->w);
		dst = byte_add(dst, dst_elem_pitch);
		src = byte_add(src, src_elem_pitch);
	}
}

static void copy_float3_to_udec3(ELCPY_PARAMS(float3, udec3))
{
	const udec3* oRESTRICT end = byte_add(dst, dst_elem_pitch * num_elements);
	while (dst < end)
	{
		dst->xyzw = float4toudec3(float4(*src, 0.0f));
		dst = byte_add(dst, dst_elem_pitch);
		src = byte_add(src, src_elem_pitch);
	}
}

static void copy_udec3_to_float3(ELCPY_PARAMS(udec3, float3))
{
	const float3* oRESTRICT end = byte_add(dst, dst_elem_pitch * num_elements);
	while (dst < end)
	{
		*dst = udec3tofloat4(src->xyzw).xyz();
		dst = byte_add(dst, dst_elem_pitch);
		src = byte_add(src, src_elem_pitch);
	}
}

static void copy_float4_to_udec3(ELCPY_PARAMS(float4, udec3))
{
	const udec3* oRESTRICT end = byte_add(dst, dst_elem_pitch * num_elements);
	while (dst < end)
	{
		dst->xyzw = float4toudec3(*src);
		dst = byte_add(dst, dst_elem_pitch);
		src = byte_add(src, src_elem_pitch);
	}
}

static void copy_udec3_to_float4(ELCPY_PARAMS(udec3, float4))
{
	const float4* oRESTRICT end = byte_add(dst, dst_elem_pitch * num_elements);
	while (dst < end)
	{
		*dst = udec3tofloat4(src->xyzw);
		dst = byte_add(dst, dst_elem_pitch);
		src = byte_add(src, src_elem_pitch);
	}
}

static elcpy_fn select(format src_format, format dst_format)
{
	#define CASE_(s,d) ((uint32_t(s)<<16) | uint32_t(d))
	#define CASE(s,d) case CASE_(format::s, format::d)
	uint32_t sel = CASE_(src_format, dst_format);
	switch (sel)
	{
    CASE(r8g8b8a8_unorm,			r8g8b8_unorm):
    CASE(b8g8r8a8_unorm,			b8g8r8_unorm):		     return (elcpy_fn)copy_3bytes;
    CASE(b8g8r8_unorm,				b8g8r8a8_unorm):
    CASE(r8g8b8_unorm,				r8g8b8a8_unorm):
    CASE(b8g8r8_unorm,				b8g8r8x8_unorm):       return (elcpy_fn)copy_3bytes_alpha;
		CASE(r8g8b8_unorm,				b8g8r8a8_unorm):
    CASE(b8g8r8_unorm,				r8g8b8a8_unorm):
    CASE(b8g8r8_unorm,				r8g8b8x8_unorm):	     return (elcpy_fn)copy_3bytes_alpha_swap_red_blue;
    CASE(b8g8r8_unorm,				a8b8g8r8_unorm):
    CASE(b8g8r8_unorm,				x8b8g8r8_unorm):       return (elcpy_fn)copy_alpha_3bytes;
    CASE(a8b8g8r8_unorm,			b8g8r8_unorm):
    CASE(x8b8g8r8_unorm,			b8g8r8_unorm):		     return (elcpy_fn)copy_skip_alpha_3bytes;
		CASE(a8b8g8r8_unorm,			b8g8r8a8_unorm):
    CASE(x8b8g8r8_unorm,			b8g8r8a8_unorm):	     return (elcpy_fn)copy_4bytes_swap_rotate_alpha;
		CASE(b8g8r8a8_unorm,			r8g8b8_unorm): 
    CASE(b8g8r8_unorm,				r8g8b8_unorm): 
    CASE(r8g8b8_unorm,				b8g8r8_unorm):		     return (elcpy_fn)copy_3bytes_swap_red_blue;
    CASE(b8g8r8a8_unorm,			r8g8b8a8_unorm):
    CASE(r8g8b8a8_unorm,			b8g8r8a8_unorm):	     return (elcpy_fn)copy_4bytes_swap_red_blue;

		CASE(r8g8b8_unorm,        r8_unorm):             return (elcpy_fn)copy_rgb_to_lum;
		CASE(b8g8r8_unorm,        r8_unorm):             return (elcpy_fn)copy_bgr_to_lum;

		CASE(r32_float,           r8_unorm):             return (elcpy_fn)copy_float_to_r8;

		CASE(r32g32_float,        r16g16_float):
		CASE(r32g32b32_float,     r16g16_float):
		CASE(r32g32b32a32_float,  r16g16_float):         return (elcpy_fn)copy_float2_to_half2;
		CASE(r16g16_float,        r32g32_float):         return (elcpy_fn)copy_half2_to_float2;
		CASE(r16g16_float,        r32g32b32_float):      return (elcpy_fn)copy_half2_to_float3;
		CASE(r32g32b32a32_float,  r32g32b32_float):      return (elcpy_fn)copy_float4_to_3;
		CASE(r32g32b32_float,     r32g32_float):         return (elcpy_fn)copy_float3_to_2;
		CASE(r32g32_float,        r32g32b32_float):      return (elcpy_fn)copy_float2_to_3;
		CASE(r16g16_uint,         r32g32_float):
		CASE(r16g16b16a16_uint,   r32g32_float):         return (elcpy_fn)copy_ushort2_to_float2;
		CASE(r32g32b32_float,     r16g16b16a16_uint):    return (elcpy_fn)copy_float3_to_ushort4;
		CASE(r16g16b16a16_uint,   r32g32b32_float):      return (elcpy_fn)copy_ushort4_to_float3;
		CASE(r32g32b32_float,     r16g16b16a16_float):   return (elcpy_fn)copy_float3_to_half4;
		CASE(r16g16b16a16_float,  r32g32b32_float):      return (elcpy_fn)copy_half4_to_float3;
		CASE(r32g32b32a32_float,  r16g16b16a16_float):   return (elcpy_fn)copy_float4_to_half4;
		CASE(r16g16b16a16_float,  r32g32b32a32_float):   return (elcpy_fn)copy_half4_to_float4;
		CASE(r32g32b32_float,     r10g10b10a2_unorm):    return (elcpy_fn)copy_float3_to_udec3;
		CASE(r10g10b10a2_unorm,   r32g32b32_float):      return (elcpy_fn)copy_udec3_to_float3;
		CASE(r32g32b32a32_float,  r10g10b10a2_unorm):    return (elcpy_fn)copy_float4_to_udec3;
		CASE(r10g10b10a2_unorm,   r32g32b32a32_float):   return (elcpy_fn)copy_udec3_to_float4;

		default: break;
	}

	oThrow(std::errc::not_supported, "%s -> %s not supported", as_string(src_format), as_string(dst_format));
	#undef CASE
	#undef CASE_
}

static format expected_bc_source(const format& f, bool bc7alpha)
{
	// expected inputs of ispc_texcomp
	switch (f)
	{
		case format::bc1_unorm: return format::r8g8b8x8_unorm;
		case format::bc3_unorm: return format::r8g8b8a8_unorm;
		case format::bc6h_uf16: return format::r16g16b16a16_float;
		case format::bc7_unorm: return bc7alpha ? format::r8g8b8a8_unorm : format::r8g8b8x8_unorm;
		default: break;
	}
	return format::unknown;
}

// dst and src must be 2d buffers (subresources) and dst must be properly allocated to receive the intended format
void convert_formatted_bc(const mapped_subresource& dst, const format& dst_format
	, const const_mapped_subresource& src, const format& src_format
	, const uint3& dimensions, const copy_option& option)
{
	const bool src_has_alpha = has_alpha(src_format);
	const auto expected_source_format = expected_bc_source(dst_format, src_has_alpha);

	oSURF_CHECK(src_format == expected_source_format, "input to bc compression must be %s", as_string(expected_source_format));
	oSURF_CHECK(is_block_compressed(dst_format), "non-bc format specified to BC compression");
	oSURF_CHECK(has_alpha(src_format) == has_alpha(dst_format) || dst_format == format::bc7_typeless || dst_format == format::bc7_unorm || dst_format == format::bc7_unorm_srgb, "BC compression alpha support mismatch");
	oSURF_CHECK(is_unorm(src_format) == is_unorm(dst_format), "BC compression unorm mismatch");
	oSURF_CHECK(is_srgb(src_format) == is_srgb(dst_format), "BC compression srgb mismatch");
	oSURF_CHECK((dimensions.x & 0x3) == 0, "width must be a multiple of 4 for BC compression");
	oSURF_CHECK((dimensions.y & 0x3) == 0, "height must be a multiple of 4 for BC compression");
	oSURF_CHECK(option == copy_option::none, "cannot flip vertically during BC compression");
	
	const uint32_t calculated_src_row_pitch = row_size(src_format, dimensions.x);
	const uint32_t calculated_dst_row_pitch = row_size(dst_format, dimensions.x);
	oSURF_CHECK(calculated_src_row_pitch == src.row_pitch, "mip_layout must be 'none' for a BC compression destination buffer");
	oSURF_CHECK(calculated_dst_row_pitch == dst.row_pitch, "mip_layout must be 'none' for a BC compression destination buffer");

	const uint32_t num_depth_slices = max(1u, dimensions.z);
	uint8_t* oRESTRICT data = (uint8_t*)dst.data;
	for (uint32_t d = 0; d < num_depth_slices; d++, data += dst.depth_pitch)
	{
		rgba_surface s;
		s.ptr = (uint8_t*)src.data + d * src.depth_pitch;
		s.width = dimensions.x;
		s.height = dimensions.y;
		s.stride = src.row_pitch;

		switch (dst_format)
		{
			case format::bc1_unorm: CompressBlocksBC1(&s, data); break;
			case format::bc3_unorm: CompressBlocksBC3(&s, data); break;
			case format::bc6h_uf16:
			{
				bc6h_enc_settings settings;
				GetProfile_bc6h_fast(&settings);
				CompressBlocksBC6H(&s, data, &settings);
				break;
			}
			
			case format::bc7_unorm:
			{
				bc7_enc_settings settings;
				src_has_alpha ? GetProfile_alpha_fast(&settings) : GetProfile_fast(&settings);
				CompressBlocksBC7(&s, data, &settings);
				break;
			}

			default:
				oThrow(std::errc::not_supported, "unsupported block compression format %s", as_string(dst_format));
		}
	}
}

void convert_formatted(const mapped_subresource& dst, const format& dst_format
	, const const_mapped_subresource& src, const format& src_format
	, const uint3& dimensions, const copy_option& option)
{
	const uint32_t src_elem_pitch = element_size(src_format);
	const uint32_t dst_elem_pitch = element_size(dst_format);
	const uint32_t num_depth_slices = max(1u, dimensions.z);

	if (dst_format == src_format)
	{
		const uint32_t row_bytes = dimensions.x * src_elem_pitch;
		const uint32_t nrows = num_rows(src_format, dimensions.y);
		const bool flip = option == copy_option::flip_vertically;

		for (uint32_t d = 0; d < num_depth_slices; d++)
		{
			uint8_t* oRESTRICT dst_data = (uint8_t*)dst.data + d * dst.depth_pitch;
			const uint8_t* oRESTRICT src_data = (const uint8_t*)src.data + d * src.depth_pitch;
			memcpy2d(dst_data, dst.row_pitch, src_data, src.row_pitch, row_bytes, nrows, flip);
		}
		return;
	}

	if (is_block_compressed(dst_format))
	{
		convert_formatted_bc(dst, dst_format, src, src_format, dimensions, option);
		return;
	}

	elcpy_fn elcpy = select(src_format, dst_format);

	if (option == copy_option::flip_vertically)
	{
		for (uint32_t d = 0; d < num_depth_slices; d++)
		{
			uint8_t* oRESTRICT dst_data = (uint8_t*)dst.data + d * dst.depth_pitch;
			const uint8_t* oRESTRICT src_data = (const uint8_t*)src.data + (d+1) * src.depth_pitch;

			for (uint32_t y = dimensions.y-1; y >= 0; y--, dst_data += dst.row_pitch)
			{
				src_data -= src.row_pitch;
				elcpy(dst_data, dst_elem_pitch, src_data, src_elem_pitch, dimensions.x);
			}
		}
	}

	else
	{
		for (uint32_t d = 0; d < num_depth_slices; d++)
		{
			uint8_t* oRESTRICT dst_data = (uint8_t*)dst.data + d * dst.depth_pitch;
			const uint8_t* oRESTRICT src_data = (const uint8_t*)src.data + d * src.depth_pitch;
			for (uint32_t y = 0; y < dimensions.y; y++, dst_data += dst.row_pitch, src_data += src.row_pitch)
				elcpy(dst_data, dst_elem_pitch, src_data, src_elem_pitch, dimensions.x);
		}
	}
}

void convert_structured(void* oRESTRICT dst, uint32_t dst_elem_pitch, const format& dst_format, const void* oRESTRICT src, uint32_t src_elem_pitch, const format& src_format, uint32_t num_elements)
{
	if (is_block_compressed(dst_format) || is_block_compressed(src_format))
		oThrow(std::errc::invalid_argument, "block compressed formats not supported when copying elements");

	if (dst_format == src_format)
	{
		if (dst_elem_pitch == src_elem_pitch)
			memcpy(dst, src, num_elements * dst_elem_pitch);
		else
		{
			const size_t size = element_size(src_format);
			const void* oRESTRICT end = byte_add(src, src_elem_pitch * num_elements);
			while (src < end)
			{
				memcpy(dst, src, size);
				dst = byte_add(dst, dst_elem_pitch);
				src = byte_add(src, src_elem_pitch);
			}
		}
	}
	else
	{
		elcpy_fn elcpy = select(src_format, dst_format);
		elcpy(dst, dst_elem_pitch, src, src_elem_pitch, num_elements);
	}
}

typedef void (*swizzle_fn)(void* elements, uint32_t element_stride, uint32_t num_elements);

static void sw_red_blue(uint8_t* elements, uint32_t element_stride, uint32_t num_elements)
{
	while (num_elements--)
	{
		uint8_t& red = *elements;
		uint8_t& blue = elements[2];
		uint8_t temp = red;
		red = blue;
		blue = temp;
		elements += element_stride;
	}
}

swizzle_fn select_swizzle(format src_format, format dst_format)
{
	#define CASE_(s,d) ((uint32_t(s)<<16) | uint32_t(d))
	#define CASE(s,d) case CASE_(format::s, format::d)
	uint32_t sel = CASE_(src_format, dst_format);
	switch (sel)
	{
		CASE(r8g8b8_unorm,		b8g8r8_unorm):
		CASE(b8g8r8_unorm,		r8g8b8_unorm):
		CASE(r8g8b8a8_unorm, b8g8r8a8_unorm):
		CASE(b8g8r8a8_unorm, r8g8b8a8_unorm): return (swizzle_fn)sw_red_blue;
		default: break;
	}

	oThrow(std::errc::not_supported, "%s -> %s swizzle not supported", as_string(src_format), as_string(dst_format));
	#undef CASE
	#undef CASE_
}

void swizzle_formatted(const info_t& info, const surface::format& new_format, const mapped_subresource& mapped)
{
	auto sw = select_swizzle(info.format, new_format);
	
	auto row = (uint8_t*)mapped.data;
	auto end = info.safe_depth() * mapped.depth_pitch + row;
	const uint32_t element_stride = element_size(new_format);
	for (; row < end; row += mapped.row_pitch)
		sw(row, element_stride, info.dimensions.x);
}

}}
