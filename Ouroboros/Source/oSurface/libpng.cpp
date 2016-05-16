// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oCore/byte.h>
#include <oCore/finally.h>
#include <oSurface/codec.h>
#include <oMemory/allocate.h>
#include <libpng/png.h>
#include <zlib/zlib.h>
#include <vector>

// this is set at the start of encode and/or decode to pass through to memory hooks 
static thread_local const ouro::allocator* tl_alloc;

struct read_state
{
	const void* data;
	size_t offset;
};

struct write_state
{
	void* data;
	size_t size;
	size_t capacity;
};

static void user_read_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
	read_state& r = *(read_state*)png_get_io_ptr(png_ptr);
	if (data) memcpy(data, (uint8_t*)r.data + r.offset, length);
	r.offset += length;
}

void user_write_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
	write_state& w = *(write_state*)png_get_io_ptr(png_ptr);
	size_t OldSize = w.size;
	size_t NewSize = OldSize + length;

	if (NewSize > w.capacity)
	{
		w.capacity = NewSize + (NewSize / 2);
		void* old = w.data;
		w.data = tl_alloc->allocate(w.capacity, "libpng");
		memcpy(w.data, old, OldSize);
		tl_alloc->deallocate(old);
	}

	memcpy((uint8_t*)w.data + w.size, data, length);
	w.size += length;
}

void user_flush_data(png_structp png_ptr)
{
}

namespace ouro { namespace surface {

static surface::format to_format(int type, int bitdepth)
{
	if (bitdepth <= 8)
	{
		switch (type)
		{
			case PNG_COLOR_TYPE_GRAY: return surface::format::r8_unorm;
			case PNG_COLOR_TYPE_RGB: return surface::format::b8g8r8_unorm;
			case PNG_COLOR_TYPE_RGB_ALPHA: return surface::format::b8g8r8a8_unorm;
			default: break;
		}
	}
	return surface::format::unknown;
}

bool is_png(const void* buffer, size_t size)
{
	static const uint8_t png_sig[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
	return size >= 8 && !memcmp(png_sig, buffer, sizeof(png_sig));
}

format required_input_png(const format& stored)
{
	const uint32_t n = num_channels(stored);
	switch (n)
	{
		case 4: return format::r8g8b8a8_unorm;
		case 3: return format::r8g8b8_unorm;
		case 1: return format::r8_unorm;
		default: break;
	}
	return format::unknown;
}

info_t get_info_png(const void* buffer, size_t size)
{
	if (!is_png(buffer, size))
		return info_t();
	
	// initialze libpng with user functions pointing to buffer
	png_infop info_ptr = nullptr;
	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	if (!png_ptr)
		throw std::exception("get_info_png failed");
	oFinally { png_destroy_read_struct(&png_ptr, &info_ptr, nullptr); };
	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
		throw std::exception("get_info_png failed");

	#pragma warning(disable:4611) // interaction between '_setjmp' and C++ object destruction is non-portable
	if (setjmp(png_jmpbuf(png_ptr)))
		throw std::exception("png read failed");
	#pragma warning(default:4611) // interaction between '_setjmp' and C++ object destruction is non-portable

	read_state rs;
	rs.data = buffer;
	rs.offset = 0;
	png_set_read_fn(png_ptr, &rs, user_read_data);
	png_read_info(png_ptr, info_ptr);

	// Read initial information and configure the decoder accordingly
	unsigned int w = 0, h = 0;
	int depth = 0, color_type = 0;
	png_get_IHDR(png_ptr, info_ptr, &w, &h, &depth, &color_type, nullptr, nullptr, nullptr);
	
	surface::info_t info;
	info.format = to_format(color_type, depth);
	info.mip_layout = mip_layout::none;
	info.dimensions = int3(w, h, 1);
	return info;
}

blob encode_png(const image& img, const allocator& file_alloc, const allocator& temp_alloc, const compression& compression)
{
	tl_alloc = &file_alloc;
	oFinally { tl_alloc = nullptr; };

	info_t info = img.info();

	// initialize libpng with user functions pointing to _pBuffer
	png_infop info_ptr = nullptr;
	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	if (!png_ptr)
		throw std::exception("png read failed");
	oFinally { png_destroy_write_struct(&png_ptr, &info_ptr); };
	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
		throw std::exception("png read failed");

	#pragma warning(disable:4611) // interaction between '_setjmp' and C++ object destruction is non-portable
	if (setjmp(png_jmpbuf(png_ptr)))
		throw std::exception("png read failed");
	#pragma warning(default:4611) // interaction between '_setjmp' and C++ object destruction is non-portable

	write_state ws;
	ws.capacity = info.dimensions.y * info.dimensions.x * element_size(info.format);
	ws.capacity += (ws.capacity / 2);
	ws.data = tl_alloc->allocate(ws.capacity, "libpng"); // use uncompressed size as an estimate to reduce reallocs
	ws.size = 0;
	png_set_write_fn(png_ptr, &ws, user_write_data, user_flush_data);

	int zcomp = Z_NO_COMPRESSION;
	switch (compression)
	{
		case compression::none: zcomp = Z_NO_COMPRESSION; break;
		case compression::low: zcomp = Z_BEST_SPEED; break;
		case compression::medium: zcomp = Z_DEFAULT_COMPRESSION; break;
		case compression::high: zcomp = Z_BEST_COMPRESSION; break;
		default: throw std::exception("invalid compression");
	}
	png_set_compression_level(png_ptr, zcomp);

	int color_type = 0;
	switch (info.format)
	{
		case format::r8_unorm: color_type = PNG_COLOR_TYPE_GRAY; break;
		case format::b8g8r8_unorm: 
		case format::r8g8b8_unorm: color_type = PNG_COLOR_TYPE_RGB; break;
		case format::b8g8r8a8_unorm: 
		case format::r8g8b8a8_unorm: color_type = PNG_COLOR_TYPE_RGB_ALPHA; break;
		default: throw std::exception("invalid format");
	}

	// how big a buffer?
	png_set_IHDR(png_ptr, info_ptr, info.dimensions.x, info.dimensions.y, 8
		, color_type, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	png_write_info(png_ptr, info_ptr);
	
	if (info.format == format::b8g8r8_unorm || info.format == format::b8g8r8a8_unorm)
		png_set_bgr(png_ptr);

	{
		std::vector<uint8_t*> rows;
		rows.resize(info.dimensions.y);
		const_mapped_subresource msr;
		shared_lock lock(img);
		rows[0] = (uint8_t*)lock.mapped.data;
		for (uint32_t y = 1; y < info.dimensions.y; y++)
			rows[y] = (uint8_t*)rows[y-1] + lock.mapped.row_pitch;
		png_write_image(png_ptr, rows.data());
		png_write_end(png_ptr, info_ptr);
	}

	return blob(ws.data, ws.size, tl_alloc->deallocator());
}

image decode_png(const void* buffer, size_t size, const allocator& texel_alloc, const allocator& temp_alloc, const mip_layout& layout)
{
	tl_alloc = &temp_alloc;
	oFinally { tl_alloc = nullptr; };

	// initialze libpng with user functions pointing to _pBuffer
	png_infop info_ptr = nullptr;
	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	if (!png_ptr)
		throw std::exception("png read failed");
	oFinally { png_destroy_read_struct(&png_ptr, &info_ptr, nullptr); };
	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
		throw std::exception("png read failed");

	#pragma warning(disable:4611) // interaction between '_setjmp' and C++ object destruction is non-portable
	if (setjmp(png_jmpbuf(png_ptr)))
		throw std::exception("png read failed");
	#pragma warning(default:4611) // interaction between '_setjmp' and C++ object destruction is non-portable

	read_state rs;
	rs.data = buffer;
	rs.offset = 0;
	png_set_read_fn(png_ptr, &rs, user_read_data);
	png_read_info(png_ptr, info_ptr);

	// Read initial information and configure the decoder accordingly
	unsigned int w = 0, h = 0;
	int depth = 0, color_type = 0;
	png_get_IHDR(png_ptr, info_ptr, &w, &h, &depth, &color_type, nullptr, nullptr, nullptr);
	
	if (depth == 16)
		png_set_strip_16(png_ptr);

	png_set_bgr(png_ptr);

	info_t info;
	info.mip_layout = layout;
	info.dimensions = int3(w, h, 1);
	switch (color_type)
	{
		case PNG_COLOR_TYPE_GRAY:
			info.format = format::r8_unorm;
			if (depth < 8)
				png_set_expand_gray_1_2_4_to_8(png_ptr);
			break;
		case PNG_COLOR_TYPE_PALETTE:
			info.format = format::b8g8r8_unorm;
			png_set_palette_to_rgb(png_ptr);
			break;
		case PNG_COLOR_TYPE_RGB:
			info.format = format::b8g8r8_unorm;
			break;
		case PNG_COLOR_TYPE_RGB_ALPHA:
			info.format = format::b8g8r8a8_unorm;
			break;
		default:
		case PNG_COLOR_TYPE_GRAY_ALPHA:
			throw std::exception("unsupported gray/alpha");
	}

	// Set up the surface buffer
	png_read_update_info(png_ptr, info_ptr);
	image img(info, texel_alloc);
	{
		std::vector<uint8_t*> rows;
		rows.resize(info.dimensions.y);
		lock_guard lock(img);

		rows[0] = (uint8_t*)lock.mapped.data;
		for (uint32_t y = 1; y < info.dimensions.y; y++)
			rows[y] = (uint8_t*)rows[y-1] + lock.mapped.row_pitch;

		png_read_image(png_ptr, rows.data());
	}

	if (layout != mip_layout::none)
		img.generate_mips();

	return img;
}

}}
