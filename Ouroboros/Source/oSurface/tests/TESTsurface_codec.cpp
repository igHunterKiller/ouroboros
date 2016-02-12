// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include <oCore/color.h>
#include <oBase/scoped_timer.h>
#include <oSystem/filesystem.h>
#include <oSurface/codec.h>
#include <oSurface/fill.h>
#include <oString/fixed_string.h>

using namespace ouro;

static bool kSaveToDesktop = false;

void save_bmp_to_desktop(const surface::image& img, const char* _path)
{
	auto encoded = surface::encode(img
		, surface::file_format::bmp
		, default_allocator
		, default_allocator
		, surface::as_3chan(img.info().format)
		, surface::compression::none);
	filesystem::save(filesystem::desktop_path() / path_t(_path), encoded, encoded.size());
}

static void compare_checkboards(unit_test::services& srv, const int2& dimensions, const surface::format& format, const surface::file_format& file_format, float max_rms)
{
	srv.trace("testing codec %s -> %s", as_string(format), as_string(file_format));

	// Create a buffer with a known format
	surface::info_t si;
	si.format = format;
	si.mip_layout = surface::mip_layout::none;
	si.dimensions = int3(dimensions, 1);
	surface::image known(si);
	size_t knownSize = known.size();
	{
		surface::lock_guard lock(known);
		surface::fill_image_t img;
		img.argb_surface = (uint32_t*)lock.mapped.data;
		img.row_width    = si.dimensions.x;
		img.num_rows     = si.dimensions.y;
		img.row_pitch    = lock.mapped.row_pitch;
		surface::fill_checker(&img, si.dimensions.x / 2, si.dimensions.y / 2, (uint32_t)color::blue, (uint32_t)color::red);
	}

	size_t EncodedSize = 0;
	blob encoded;

	{
		scoped_timer("encode");
		encoded = surface::encode(known
			, file_format
			, surface::as_3chan(known.info().format)
			, surface::compression::none);
	}

	sstring buf;
	format_bytes(buf, EncodedSize, 2);
	srv.trace("encoded %s", buf.c_str());

	if (kSaveToDesktop)
	{
		mstring fname;
		snprintf(fname, "encoded_from_known_%s.%s", as_string(file_format), as_string(file_format));
		filesystem::save(path_t(fname), encoded, encoded.size());
	}

	surface::image decoded;
	{
		scoped_timer("decode");
		decoded = surface::decode(encoded, encoded.size(), format);
	}

	{
		size_t decodedSize = decoded.size();

		oCHECK(known.size() == decoded.size(), "encoded %u but got %u on decode", knownSize, decodedSize);

		if (kSaveToDesktop)
		{
			mstring fname;
			snprintf(fname, "encoded_from_decoded_%s.%s", as_string(file_format), as_string(surface::file_format::bmp));
			save_bmp_to_desktop(decoded, fname);
		}

		float rms = surface::calc_rms(known, decoded);
		oCHECK(rms <= max_rms, "encoded/decoded bytes mismatch for %s", as_string(file_format));
	}
}

void compare_load(unit_test::services& srv, const char* path, const char* desktop_filename_prefix)
{
	auto encoded = srv.load_buffer(path);
	surface::image decoded;
	{
		scoped_timer("decode");
		decoded = surface::decode(encoded, encoded.size(), surface::format::b8g8r8a8_unorm);
	}

	auto ff = surface::get_file_format(path);

	mstring fname;
	snprintf(fname, "%s_%s.%s", desktop_filename_prefix, as_string(ff), as_string(surface::file_format::bmp));
	save_bmp_to_desktop(decoded, fname);
}

oTEST(oSurface_codec)
{
	// still a WIP
	//compare_checkboards(uint2(11,21), surface::format::b8g8r8a8_unorm, surface::file_format::psd, 1.0f);
	//compare_load(_Services, "Test/Textures/lena_1.psd", "lena_1");

	compare_checkboards(srv, uint2(11,21), surface::format::b8g8r8a8_unorm, surface::file_format::bmp, 1.0f);
	compare_checkboards(srv, uint2(11,21), surface::format::b8g8r8a8_unorm, surface::file_format::dds, 1.0f);
	compare_checkboards(srv, uint2(11,21), surface::format::b8g8r8a8_unorm, surface::file_format::jpg, 4.0f);
	compare_checkboards(srv, uint2(11,21), surface::format::b8g8r8a8_unorm, surface::file_format::png, 1.0f);
	compare_checkboards(srv, uint2(11,21), surface::format::b8g8r8a8_unorm, surface::file_format::tga, 1.0f);
}
