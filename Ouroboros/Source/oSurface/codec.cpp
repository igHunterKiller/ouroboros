// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oSurface/codec.h>
#include <oSurface/convert.h>
#include <oMemory/memory.h>
#include <oString/stringize.h>

namespace ouro { namespace surface {

// === ADDING A FILE FORMAT ===
//
// add format extension to this list and it will propagate to all the apis below.
// Implement all functions declared by DECLARE_CODEC.
// note: tga doesn't have a signature so keep it last since it's the weakest to authenticate
#define FOREACH_EXT(macro) macro(oimg) macro(bmp) macro(dds) macro(jpg) macro(png) macro(psd) macro(tga)

// _____________________________________________________________________________
// Boilerplate (don't use directly, they're registered with the functions below)

#define DECLARE_CODEC(ext) \
	bool is_##ext(const void* buffer, size_t size); \
	format required_input_##ext(const format& stored); \
	info_t get_info_##ext(const void* buffer, size_t size); \
	blob encode_##ext(const image& img, const allocator& file_alloc, const allocator& temp_alloc, const compression& compression); \
	image decode_##ext(const void* buffer, size_t size, const allocator& texel_alloc, const allocator& temp_alloc, const mip_layout& layout);

#define GET_FILE_FORMAT_EXT(ext) if (!_stricmp(extension, "." #ext)) return file_format::##ext;
#define GET_FILE_FORMAT_HEADER(ext) if (is_##ext(buffer, size)) return file_format::##ext;
#define GET_REQ_INPUT(ext) case file_format::##ext: return required_input_##ext(stored_format);
#define GET_INFO(ext) case file_format::##ext: return get_info_##ext(buffer, size);
#define ENCODE(ext) case file_format::##ext: return encode_##ext(*input, file_alloc, temp_alloc, compression);
#define DECODE(ext) case file_format::##ext: decoded = decode_##ext(buffer, size, texel_alloc, temp_alloc, layout); break;
#define AS_STRING(ext) case surface::file_format::##ext: return #ext;

FOREACH_EXT(DECLARE_CODEC)

file_format get_file_format(const char* path)
{
	const char* extension = rstrstr(path, ".");
	FOREACH_EXT(GET_FILE_FORMAT_EXT)
	return file_format::unknown;
}

file_format get_file_format(const void* buffer, size_t size)
{
	FOREACH_EXT(GET_FILE_FORMAT_HEADER)
	return file_format::unknown;
}

format required_input(const file_format& file_format, const format& stored_format)
{
	switch (file_format)
	{
		FOREACH_EXT(GET_REQ_INPUT)
		default: break;
	}

	return format::unknown;
}

info_t get_info(const void* buffer, size_t size)
{
	switch (get_file_format(buffer, size))
	{	FOREACH_EXT(GET_INFO)
		default: break;
	}
	throw std::exception("unknown image encoding");
}
	
blob encode(const image& img
	, const file_format& fmt
	, const allocator& file_alloc
	, const allocator& temp_alloc
	, const format& desired_format
	, const compression& compression)
{
	auto buffer_format = img.info().format;
	auto dst_format = desired_format;
	if (dst_format == format::unknown)
		dst_format = buffer_format;

	dst_format = required_input(fmt, dst_format);
	oCheck(dst_format != format::unknown, std::errc::not_supported, "%s encoding does not support desired format %s", as_string(fmt), as_string(desired_format));

	image converted;
	image converted_for_bc_input;
	const image* input = &img;

	if (is_block_compressed(dst_format))
	{
		// this is a rule particular to the intel compressor, though other compressors have similar rules
		// so figure out where this should actually be: really close to the codec, in convert, here. Is 
		// there a way to allow better inspection of the requirements?
		buffer_format = has_alpha(dst_format) ? format::r8g8b8a8_unorm : format::r8g8b8x8_unorm;

		converted_for_bc_input = input->convert(buffer_format, temp_alloc);
		input = &converted_for_bc_input;
	}
	
	if (buffer_format != dst_format)
	{
		converted = input->convert(dst_format, temp_alloc);
		converted_for_bc_input = image();
		input = &converted;
	}

	switch (fmt)
	{ FOREACH_EXT(ENCODE)
		default: break;
	}
	throw std::exception("unknown image encoding");
}

image decode(const void* buffer
	, size_t size
	, const allocator& texel_alloc
	, const allocator& temp_alloc
	, const format& desired_format
	, const mip_layout& layout)
{
	image decoded;
	switch (get_file_format(buffer, size))
	{ FOREACH_EXT(DECODE)
		default: throw std::exception("unknown image encoding");
	}

	if (desired_format != format::unknown && desired_format != decoded.info().format)
		return decoded.convert(desired_format);
	return decoded;
}

	}

const char* as_string(const surface::file_format& ff)
{
	switch (ff)
	{
		FOREACH_EXT(AS_STRING)
		default: break;
	}
	return "?";
}

}
