// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Facade for encoding and decoding several image formats

#pragma once
#include <oSurface/image.h>
#include <oSurface/surface.h>

namespace ouro { namespace surface {

enum class file_format : uint8_t
{
	unknown,
	bmp,
	dds,
	jpg,
	png,
	psd,
	tga,
	oimg,
};

enum class compression : uint8_t
{
	none,
  low,
  medium,
  high,
};

// checks the file extension
file_format get_file_format(const char* path);

// checks the first few bytes/header info
file_format get_file_format(const void* buffer, size_t size);
inline file_format get_file_format(const blob& buffer) { return get_file_format(buffer, buffer.size()); }

// converts the first few bytes of a supported format into a surface::info
// if not recognized the returned format will be surface::unknown
info_t get_info(const void* buffer, size_t size);
inline info_t get_info(const blob& buffer) { return get_info(buffer, buffer.size()); }

// returns a buffer ready to be written to disk in the specified format.
// this may use the specified allocator to convert the texel buffer to 
// the input format for the codec.
blob encode(const image& img, const file_format& fmt
	, const allocator& file_alloc = default_allocator
	, const allocator& temp_alloc = default_allocator
	, const format& desired_format = format::unknown
	, const compression& compression = compression::low);

inline blob encode(const image& img, const file_format& fmt, const format& desired_format
	, const compression& compression = compression::low) { return encode(img, fmt, default_allocator, default_allocator, desired_format, compression); }

// Parses the in-memory formatted buffer into a surface. temp_alloc will be used
// for any conversion or temporary storage, texel_alloc will be used for image.
image decode(const void* buffer, size_t size
	, const allocator& texel_alloc = default_allocator
	, const allocator& temp_alloc = default_allocator
	, const format& desired_format = format::unknown
	, const mip_layout& layout = mip_layout::none);

inline image decode(const void* buffer, size_t size
	, const format& desired_format, const mip_layout& layout = mip_layout::none) { return decode(buffer, size, default_allocator, default_allocator, desired_format, layout); }

inline image decode(const blob& buffer
	, const allocator& texel_alloc = default_allocator
	, const allocator& temp_alloc = default_allocator
	, const format& desired_format = format::unknown
	, const mip_layout& layout = mip_layout::none) { return decode(buffer, buffer.size(), texel_alloc, temp_alloc, desired_format, layout); }

inline image decode(const blob& buffer
	, const format& desired_format
	, const mip_layout& layout = mip_layout::none) { return decode(buffer, buffer.size(), default_allocator, default_allocator, desired_format, layout); }

}}
