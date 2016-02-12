// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oSurface/codec.h>
#include <oSurface/convert.h>
#include "tga.h"

namespace ouro { namespace surface {

bool is_tga(const void* buffer, size_t size)
{
	auto h = (const tga_header*)buffer;
	return !h->id_length && !h->paletted_type && tga_is_valid_dtf(h->data_type_field)
		&& !h->paletted_origin && !h->paletted_length && !h->paletted_depth
		&& h->width >= 1 && h->height >= 1 && !h->image_descriptor
		&& (h->bpp == 32 || h->bpp == 24);
}

format required_input_tga(const format& stored)
{
	if (num_channels(stored) == 4)
		return format::b8g8r8a8_unorm;
	else if (num_channels(stored) == 3)
		return format::b8g8r8_unorm;
	return format::unknown;
}

info_t get_info_tga(const void* buffer, size_t size)
{
	if (!is_tga(buffer, size))
		return info_t();

	auto h = (const tga_header*)buffer;
	info_t si;
	si.format = h->bpp == 32 ? format::b8g8r8a8_unorm : format::b8g8r8_unorm;
	si.dimensions = int3(h->width, h->height, 1);
	return si;
}

blob encode_tga(const image& img, const allocator& file_alloc, const allocator& temp_alloc, const compression& compression)
{
	if (compression != compression::none)
		throw std::system_error(std::errc::not_supported, std::system_category(), "compression not supported");

	auto info = img.info();

	if (info.format != format::b8g8r8a8_unorm && info.format != format::b8g8r8_unorm)
		throw std::invalid_argument("source must be b8g8r8a8_unorm or b8g8r8_unorm");

	if (info.dimensions.x > 0xffff || info.dimensions.y > 0xffff)
		throw std::invalid_argument("dimensions must be <= 65535");

	tga_header h = {0};
	h.data_type_field = tga_data_type_field::rgb;
	h.bpp = (uint8_t)bits(info.format);
	h.width = (uint16_t)info.dimensions.x;
	h.height = (uint16_t)info.dimensions.y;

	const size_t size = sizeof(tga_header) + h.width * h.height * (h.bpp/8);

	blob p = file_alloc.scoped_allocate(size, "encoded tga");
	memcpy(p, &h, sizeof(tga_header));
	mapped_subresource dst;
	dst.data = ((uint8_t*)p + sizeof(tga_header));
	dst.row_pitch = element_size(info.format) * h.width;
	dst.depth_pitch = dst.row_pitch * h.height;

	img.copy_to(0, dst, copy_option::flip_vertically);
	return p;
}

image decode_tga(const void* buffer, size_t size, const allocator& texel_alloc, const allocator& temp_alloc, const mip_layout& layout)
{
	info_t tga_info = get_info_tga(buffer, size);

	if (tga_info.format == format::unknown)
		throw std::invalid_argument("invalid tga buffer");

	auto h = (const tga_header*)buffer;
	info_t info = tga_info;
	info.mip_layout = layout;
	auto src = map_const_subresource(tga_info, 0, &h[1]);

	image img(info, texel_alloc);
	img.copy_from(0, src, copy_option::flip_vertically);

	if (info.mips())
		img.generate_mips();

	return img;
}

}}
