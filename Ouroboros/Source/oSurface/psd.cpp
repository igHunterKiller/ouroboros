// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oCore/stringf.h>
#include <oCore/finally.h>
#include <oSurface/codec.h>
#include <oSurface/convert.h>
#include <oMemory/memory.h>
#include "psd.h"

namespace ouro { namespace surface {

static format get_format(const psd_header* h)
{
#define SELFMT(ch, bpp) (((ch)<<16)|(bpp))
	switch (SELFMT(h->num_channels, h->bits_per_channel))
	{
		case SELFMT(3, psd_bits_per_channel::k8): return format::b8g8r8_unorm;
		case SELFMT(4, psd_bits_per_channel::k8): return format::b8g8r8a8_unorm;
		case SELFMT(4, psd_bits_per_channel::k16): return format::r16g16b16a16_unorm;
		case SELFMT(3, psd_bits_per_channel::k32): return format::r32g32b32_uint;
		case SELFMT(4, psd_bits_per_channel::k32): return format::r32g32b32a32_uint;
		default: break;
	}
#undef SELFMT
	return format::unknown;
}

bool is_psd(const void* buffer, size_t size)
{
	auto h = (const psd_header*)buffer;
  return size >= sizeof(psd_header)
		&& psd_signature == psd_swap(h->signature)
		&& psd_version == psd_swap(h->version);
}

info_t get_info_psd(const void* buffer, size_t size, psd_header* out_header)
{
	if (!psd_validate(buffer, size, out_header))
		return info_t();

	// only supports rgb or rgba at present
	if (out_header->num_channels < 3 || out_header->num_channels > 4)
		return info_t();

	if (out_header->bits_per_channel != psd_bits_per_channel::k8)
		return info_t();

	info_t info;
	info.dimensions = uint3(out_header->width, out_header->height, 1);
	info.format = get_format(out_header);
	return info;
}

format required_input_psd(const format& f)
{
	return format::unknown; // no encode yet supported
}

info_t get_info_psd(const void* buffer, size_t size)
{
	psd_header h;
	return get_info_psd(buffer, size, &h);
}

blob encode_psd(const image& img, const allocator& file_alloc, const allocator& temp_alloc, const compression& compression)
{
	throw std::system_error(std::errc::operation_not_supported, std::system_category(), "psd encoding not supported");
}

image decode_psd(const void* buffer, size_t size, const allocator& texel_alloc, const allocator& temp_alloc, const mip_layout& layout)
{
	psd_header h;
	info_t psd_info = get_info_psd(buffer, size, &h);

	if (psd_info.format == format::unknown)
		throw std::invalid_argument("invalid psd buffer");

	info_t info = psd_info;
	info.mip_layout = layout;

	image img(info, texel_alloc);

	auto bits_header = (const uint16_t*)get_image_data_section(buffer, size);
	auto compression = (const psd_compression)psd_swap(*bits_header);
	const void* bits = &bits_header[1];
	const size_t channel_bytes = h.bits_per_channel / 8;
	const size_t pixel_bytes = channel_bytes * h.num_channels;
		
	mapped_subresource mapped;
	uint2 byte_dimensions;
	img.map(0, &mapped, &byte_dimensions);
	oFinally { img.unmap(0); };

	switch (compression)
	{
		//case psd_compression::raw:
		//{
		//	const auto sizes = channel_bits(si.format);
		//	const size_t plane_elemment_pitch = sizes.r / 8;
		//	const size_t plane_row_pitch = plane_elemment_pitch * si.dimensions.x;
		//	const size_t plane_depth_pitch = plane_row_pitch * si.dimensions.y;

		//	const bool dst_has_alpha = has_alpha(si.format);

		//	const void* red   = (const uint8_t*)bits  + plane_depth_pitch;
		//	const void* green = (const uint8_t*)red   + plane_depth_pitch;
		//	const void* blue  = (const uint8_t*)green + plane_depth_pitch;
		//	const void* alpha = (sizes.a ? ((const uint8_t*)blue + plane_depth_pitch) : nullptr);

		//	switch (h.bits_per_channel)
		//	{
		//		// todo: swap around the formats as appropriate
		//		case psd_bits_per_channel::k8: interleave_channels<uint8_t>((uint8_t*)mapped.data, byte_dimensions.x, byte_dimensions.y, dst_has_alpha, red, green, blue, alpha); break;
		//		case psd_bits_per_channel::k16: interleave_channels<uint16_t>((uint16_t*)mapped.data, byte_dimensions.x, byte_dimensions.y, dst_has_alpha, red, green, blue, alpha); break;
		//		case psd_bits_per_channel::k32: interleave_channels<uint32_t>((uint32_t*)mapped.data, byte_dimensions.x, byte_dimensions.y, dst_has_alpha, red, green, blue, alpha); break;
		//		default: throw std::system_error(std::errc::operation_not_supported, "unsupported bitdepth in psd decode");
		//	}

		//	break;
		//}

		case psd_compression::rle:
		{
			size_t size = byte_dimensions.x * byte_dimensions.y;
			void* d = mapped.data;
			
			for (uint16_t i = 0; i < h.num_channels; i++)
			{
				bits = rle_decoden(d, size, pixel_bytes, channel_bytes, bits);
				d = (uint8_t*)d + channel_bytes;
			}

			break;
		}

		default: throw std::system_error(std::errc::operation_not_supported, std::system_category(), stringf("unsupported compression type %d in psd decode", (int)compression));
	}

	if (info.mips())
		img.generate_mips();

	return img;
}

}}
