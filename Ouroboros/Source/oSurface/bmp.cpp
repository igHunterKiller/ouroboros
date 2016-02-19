// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oSurface/codec.h>
#include <oSurface/convert.h>
#include "bmp.h"

namespace ouro { namespace surface {

bool is_bmp(const void* buffer, size_t size)
{
	auto h = (const bmp_header*)buffer;
	return size >= sizeof(bmp_header) && h->bfType == bmp_signature;
}

info_t get_info_bmp(const void* buffer, size_t size)
{
	if (!is_bmp(buffer, size))
		return info_t();

	auto h = (const bmp_header*)buffer;
	auto bmi = (const bmp_info*)&h[1];

	if (bmi->bmiHeader.biBitCount != 32 && bmi->bmiHeader.biBitCount != 24)
		return info_t();

	info_t info;
	info.format = bmi->bmiHeader.biBitCount == 32 ? format::b8g8r8a8_unorm : format::b8g8r8_unorm;
	info.dimensions = int3(bmi->bmiHeader.biWidth, bmi->bmiHeader.biHeight, 1);
	return info;
}

format required_input_bmp(const format& stored)
{
	if (num_channels(stored) == 4)
		return format::b8g8r8a8_unorm;
	else if (num_channels(stored) == 3)
		return format::b8g8r8_unorm;
	return format::unknown;
}

blob encode_bmp(const image& img, const allocator& file_alloc, const allocator& temp_alloc, const compression& compression)
{
	auto info = img.info();
	if (info.format != surface::format::b8g8r8a8_unorm && info.format != surface::format::b8g8r8_unorm)
		oThrow(std::errc::invalid_argument, "source must be b8g8r8a8_unorm or b8g8r8_unorm");

	const uint32_t src_byte_count = element_size(info.format);

	const uint32_t element_size = surface::element_size(info.format);
	const uint32_t unaligned_row_pitch = element_size * info.dimensions.x;
	const uint32_t aligned_row_pitch = align(unaligned_row_pitch, 4);
	const uint32_t buffer_size = aligned_row_pitch * info.dimensions.y;

	const uint32_t bfOffBits = sizeof(bmp_header) + sizeof(bmp_infoheader);
	const uint32_t bfSize = bfOffBits + buffer_size;
	const bool is_32bit = info.format == surface::format::b8g8r8a8_unorm;

	blob p = file_alloc.scoped_allocate(bfSize, "encoded bmp");

	auto bfh = (bmp_header*)p;
	auto bmi = (bmp_info*)&bfh[1];
	void* bits_ = (uint8_t*)p + bfOffBits;

	bfh->bfType = bmp_signature;
	bfh->bfSize = bfSize;
	bfh->bfReserved1 = 0;
	bfh->bfReserved2 = 0;
	bfh->bfOffBits = bfOffBits;
	
	bmi->bmiHeader.biSize = sizeof(bmp_infoheader);
	bmi->bmiHeader.biWidth = info.dimensions.x;
	bmi->bmiHeader.biHeight = info.dimensions.y;
	bmi->bmiHeader.biPlanes = 1;
	bmi->bmiHeader.biBitCount = is_32bit ? 32 : 24;
	bmi->bmiHeader.biCompression = bmp_compression::rgb;
	bmi->bmiHeader.biSizeImage = buffer_size;
	bmi->bmiHeader.biXPelsPerMeter = 0x0ec4;
	bmi->bmiHeader.biYPelsPerMeter = 0x0ec4;
	bmi->bmiHeader.biClrUsed = 0;
	bmi->bmiHeader.biClrImportant = 0;

	shared_lock lock(img);
	
	uint32_t padding = aligned_row_pitch - unaligned_row_pitch;
	for (uint32_t y = 0, y1 = info.dimensions.y-1; y < info.dimensions.y; y++, y1--)
	{
		auto scanline = y * aligned_row_pitch + (uint8_t*)bits_;
		auto src = y1 * lock.mapped.row_pitch + (const uint8_t*)lock.mapped.data;
		for (uint32_t x = 0; x < unaligned_row_pitch; x += element_size)
		{
			*scanline++ = src[0];
			*scanline++ = src[1];
			*scanline++ = src[2];
			if (is_32bit)
				*scanline++ = src[3];
			src += src_byte_count;
		}

		for (uint32_t x = unaligned_row_pitch; x < aligned_row_pitch; x++)
			*scanline++ = 0;
	}

	return p;
}

image decode_bmp(const void* buffer, size_t size, const allocator& texel_alloc, const allocator& temp_alloc, const mip_layout& layout)
{
	const bmp_header* bfh = (const bmp_header*)buffer;
	const bmp_info* bmi = (const bmp_info*)&bfh[1];
	const void* bits_ = (const uint8_t*)buffer + bfh->bfOffBits;

	info_t bmp_info = get_info_bmp(buffer, size);
	
	if (bmp_info.format == format::unknown)
		oThrow(std::errc::invalid_argument, "invalid bmp buffer");

	info_t info = bmp_info;
	info.mip_layout = layout;
	const_mapped_subresource src;
	src.data = bits_;
	src.depth_pitch = bmi->bmiHeader.biSizeImage;
	src.row_pitch = src.depth_pitch / bmi->bmiHeader.biHeight;

	image img(info, texel_alloc);
	img.copy_from(0, src, copy_option::flip_vertically);
	return img;
}

}}
