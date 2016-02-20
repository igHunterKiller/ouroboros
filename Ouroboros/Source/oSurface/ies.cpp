// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oCore/finally.h>
#include <oMath/quantize.h>
#include <oString/atof.h>
#include <oCore/stringize.h>
#include <oSurface/codec.h>
#include <oSurface/convert.h>
#include "ies.h"

namespace ouro {

static void move_to_next_line(const char*& cur)
{
	// move to next line
	while (*cur && (*cur != '\r' && *cur != '\n')) cur++;
	while (*cur && (*cur == '\r' || *cur == '\n')) cur++;
}
	
template<> const char* as_string(const ies_format& f)
{
	static const char* s_names[] = 
	{
		"unknown",
		"IESNA86",
		"IESNA91",
		"IESNA:LM-63-1995",
		"IESNA:LM-63-2002",
	};
	return as_string(f, s_names);
}

template<> const char* as_string(const ies_units& u)
{
	static const char* s_names[] = 
	{
		"unknown",
		"feet",
		"meters",
	};
	return as_string(u, s_names);
}

bool from_string(ies_format* out_format, const char* src)
{
	// there's no header for this, so we can only assume
	// it's right if the others don't match.
	*out_format = ies_format::iesna_86;
	if (_stricmp(src, as_string(ies_format::iesna_91)))
		*out_format = ies_format::iesna_91;
	else if (_stricmp(src, as_string(ies_format::iesna_95)))
		*out_format = ies_format::iesna_95;
	return *out_format != ies_format::unknown;
}

bool from_string(ies_tilt* out_tilt, const char* src)
{
	*out_tilt = ies_tilt::filename;
	if (_stricmp(src, "NONE"))
		*out_tilt = ies_tilt::none;
	else if (_stricmp(src, "INCLUDE"))
		*out_tilt = ies_tilt::include;
	return *out_tilt != ies_tilt::unknown;
}
	
ies_geometry to_geometry(float w, float l, float h)
{
	const bool wzero = equal(0.0f, w); const bool wneg = w < 0.0f; const bool wpos = w > 0.0f;
	const bool lzero = equal(0.0f, l); const bool lneg = l < 0.0f; const bool lpos = l > 0.0f;
	const bool hzero = equal(0.0f, h); const bool hneg = h < 0.0f; const bool hpos = h > 0.0f;

	if (wzero  && lzero  && hzero ) return ies_geometry::point;
	if (!wzero && !lzero && !hzero) return ies_geometry::rectangle;
	if (wneg   && lzero  && hzero ) return ies_geometry::circle;
	if (wneg   && lzero  && hneg  ) return ies_geometry::sphere;
	if (wneg   && lzero  && hpos  ) return ies_geometry::verticle_cylinder;
  if (wzero  && lpos   && hneg  ) return ies_geometry::horizontal_cylinder_along_length;
	if (wpos   && lzero  && hneg  ) return ies_geometry::horizontal_cylinder_along_width;
	if (wneg   && lpos	 && hpos	) return ies_geometry::ellipse_along_length;
	if (wpos   && lneg	 && hpos	) return ies_geometry::ellipse_along_width;
	if (wneg   && lpos	 && hneg	) return ies_geometry::ellipsoid_along_length;
	if (wpos   && lneg	 && hneg	) return ies_geometry::ellipsoid_along_width;

	return ies_geometry::unknown;
}

ies_header to_header(const char* ies_string, const char** out_data = nullptr)
{
	const char* cur = ies_string;
	ies_header h;

	if (!from_string(&h.format, cur))
		return ies_header();

	cur = strstr(cur, "TILT=");
	if (!cur)
		oThrow(std::errc::invalid_argument, "invalid ies (no TILT)");

	cur += 5;
	from_string(&h.tilt, cur);
	if (h.tilt != ies_tilt::none)
		oThrow(std::errc::invalid_argument, "invalid ies (TILT=INCLUDE and TILT=<filename> not supported)");

	move_to_next_line(cur);

	if (10 != sscanf_s(cur, "%d %f %f %d %d %hhu %hhu %hhu %f %f %f", &h.num_lamps, &h.lumens_per_lamp, &h.candela_multiplier, &h.num_vangles, &h.num_hangles, &h.photometric_type, &h.units, &h.geometry, &h.width, &h.length, &h.height))
		oThrow(std::errc::invalid_argument, "invalid ies meta data (line 10)");

	move_to_next_line(cur);
	if (3 != sscanf_s(cur, "%f %f %f", &h.ballast_factor, &h.ballast_lamp_photometric_factor, &h.input_watts))
		oThrow(std::errc::invalid_argument, "invalid ies meta data (line 11)");

	h.geometry = to_geometry(h.width, h.length, h.height);

	// validate
	if (1 != h.num_lamps)                                           oThrow(std::errc::invalid_argument, "only 1 lamp supported");
	if (0.0f > h.lumens_per_lamp)                                   oThrow(std::errc::invalid_argument, "lumens_per_lamp must be a non-zero positive number");
	if (0.0f > h.candela_multiplier)                                oThrow(std::errc::invalid_argument, "candela_multiplier must be a non-zero positive number");
	if (0 > h.num_vangles)                                          oThrow(std::errc::invalid_argument, "num_vangles must be a non-zero positive number");
	if (0 > h.num_hangles)                                          oThrow(std::errc::invalid_argument, "num_hangles must be a non-zero positive number");
	if (h.photometric_type != ies_photometric_type::a && 
			h.photometric_type != ies_photometric_type::b && 
			h.photometric_type != ies_photometric_type::c)              oThrow(std::errc::invalid_argument, "invalid photometric type");
	if (h.units != ies_units::feet && h.units != ies_units::meters) oThrow(std::errc::invalid_argument, "invalid units");
	//if (h.geometry == ies_geometry::unknown)                        oThrow(std::errc::invalid_argument, "invalid geometry");
	if (0.0f > h.ballast_factor)                                    oThrow(std::errc::invalid_argument, "ballast_factor must be a non-zero positive number");
	if (0.0f > h.ballast_lamp_photometric_factor)                   oThrow(std::errc::invalid_argument, "ballast_lamp_photometric_factor must be a non-zero positive number");
	if (0.0f > h.input_watts)                                       oThrow(std::errc::invalid_argument, "input_watts must be a non-zero positive number");

	if (out_data)
	{
		move_to_next_line(cur);
		*out_data = cur;
	}

	return h;
}

static ies_header to_header(const void* buffer, size_t size, const char** out_data = nullptr)
{
	// ies files are pretty small so alloc on stack to null-terminate
	char ies_string[32 * 1024];

	if (size >= sizeof(ies_string))
		throw std::bad_alloc();

	memcpy(ies_string, buffer, size);
	ies_string[size] = '\0';
	
	return to_header(ies_string, out_data);
}

float coordinate(const float* sorted_floats, size_t num_floats)
{
	return 0.0f;
}

namespace surface {

bool is_ies(const void* buffer, size_t size)
{
	ies_format f;
	return from_string(&f, (const char*)buffer);
}

format required_input_ies(const format& stored)
{
	return format::unknown;
}

static info_t to_info(const ies_header& h)
{
	info_t si;
	si.dimensions = uint3(256, 1, 1);
	si.array_size = 0;
	si.format = format::r16_float;
	si.semantic = semantic::photometric_profile;
	si.mip_layout = mip_layout::none;
	si.pad = 0;
	return si;
}

info_t get_info_ies(const void* buffer, size_t size)
{
	if (!is_ies(buffer, size))
		return info_t();
	return to_info(to_header(buffer, size));
}

// angles must be sorted ascending
// this is similar to converting to a texel space coordinate given an interpolated value
static float ies_spherical_pos(float value, const float* angles, size_t num_angles)
{
	size_t start = 0;
	size_t end = num_angles - 1;

	if (value < angles[start])
		return (float)start;
	if (value > angles[end])
		return (float)end;

	while (start < end)
	{
		size_t mid = (start + end + 1) / 2;
		float pivot = angles[mid];
		if (value >= pivot)
			start = mid;
		else
			end = mid - 1;
	}

	float floor_ = angles[start];
	float frac_  = 0.0f;

	if (start + 1 < end)
	{
		float ceil_ = angles[start+1];
		float diff = ceil_ - floor_;
		if (!equal(0.0f, diff))
			frac_ = (value - floor_) / diff;
	}

	return (float)start + frac_;
}

static float get_candela(int hangle, int vangle, const float* candela, const size_t num_hangles, const size_t num_vangles)
{
	auto x = hangle % num_hangles;
	auto y = vangle % num_vangles;
	return candela[y + num_vangles * x];
}

static float bilerp_candela(float spherical_x, float spherical_y, const float* candela, const size_t num_hangles, const size_t num_vangles)
{
	const int    x = (int)floor(spherical_x);
	const int    y = (int)floor(spherical_y);
	const float rx = spherical_x - x;
	const float ry = spherical_y - y;

	const float tl = get_candela(x + 0, y + 0, candela, num_hangles, num_vangles);
	const float tr = get_candela(x + 1, y + 0, candela, num_hangles, num_vangles);
	const float bl = get_candela(x + 0, y + 1, candela, num_hangles, num_vangles);
	const float br = get_candela(x + 1, y + 1, candela, num_hangles, num_vangles);
	const float t  = lerp(tl, tr, ry);
	const float b  = lerp(bl, br, ry);

	return lerp(t, b, rx);
}

float sample_candela(float hangle, float vangle, const float* hangles, size_t num_hangles, const float* vangles, size_t num_vangles, const float* candela)
{
	const float spherical_x = ies_spherical_pos(hangle, hangles, num_hangles);
	const float spherical_y = ies_spherical_pos(vangle, vangles, num_vangles);
	return bilerp_candela(spherical_x, spherical_y, candela, num_hangles, num_vangles);
}

static float sample_candela_1d(float vangle, size_t num_hangles, const float* vangles, size_t num_vangles, const float* candela)
{
	const float spherical_y = ies_spherical_pos(vangle, vangles, num_vangles);

	float acc = 0.0f;
	for (size_t x = 0; x < num_hangles; x++)
		acc += bilerp_candela((float)x, spherical_y, candela, num_hangles, num_vangles);

	return acc / (float)num_hangles;
}


blob encode_ies(const image& img, const allocator& file_alloc, const allocator& temp_alloc, const compression& compression)
{
	oThrow(std::errc::not_supported, "encoding ies is not supported");
}

image decode_ies(const void* buffer, size_t size, const allocator& texel_alloc, const allocator& temp_alloc, const mip_layout& layout)
{
	const char* cur;
	auto h = to_header(buffer, size, &cur);

	if (h.photometric_type != ies_photometric_type::c)
		oThrow(std::errc::invalid_argument, "only photometric type c is currently supported");

	// allocate temp memory for angles and candela
	const auto candela_count = h.num_vangles * h.num_hangles;
	const auto angle_bytes = sizeof(float) * (h.num_vangles + h.num_hangles);
	const auto candela_bytes = sizeof(float) * candela_count;
	
	auto backing = temp_alloc.scoped_allocate(angle_bytes + candela_bytes, "ies values");
	auto vangles = (float*)backing;
	auto hangles = vangles + h.num_vangles;
	auto candela = hangles + h.num_hangles;

	// read the vertical angles
	for (int i = 0; i < h.num_vangles; i++)
		if (!ouro::atof(&cur, vangles + i))
			std::invalid_argument("invalid vertical angle value");

	// read the horizontal angles
	move_to_next_line(cur);
	for (int i = 0; i < h.num_hangles; i++)
		if (!ouro::atof(&cur, hangles + i))
			std::invalid_argument("invalid horizontal angle value");
	
	// read the candela
	float* cur_candela = candela;
	float max_candela = -FLT_MAX;
	const float* end_candela = cur_candela + candela_count;
	for (int x = 0; x < h.num_hangles; x++)
	{
		move_to_next_line(cur);
		while (cur_candela < end_candela)
			if (ouro::atof(&cur, cur_candela))
			{
				*cur_candela *= h.candela_multiplier;
				max_candela = max(max_candela, *cur_candela);
				cur_candela++;
			}
			else
				std::invalid_argument("invalid candela value");
	}

	// normalize values
	float inv_max = 1.0f / max_candela;
	cur_candela = candela;
	while (cur_candela < end_candela)
		*cur_candela++ *= inv_max;

	// set up an image for final storage
	info_t info = to_info(h);

	if (info.format == format::unknown)
		oThrow(std::errc::invalid_argument, "invalid ies buffer");

	image img(info, texel_alloc);

	const float rcp_dimx = 1.0f / float(info.dimensions.x - 1);

	mapped_subresource mapped;
	uint2 dim;
	img.map(0, &mapped, &dim);

	uint16_t* dst = (uint16_t*)mapped.data;
	for (uint32_t y = 0; y < info.dimensions.y; y++)
	{
		auto scanline = dst;
		for (uint32_t x = 0; x < info.dimensions.x; x++)
		{
			float percentage = x * rcp_dimx;
			float degree = percentage * 180.0f;
			
			float cand = sample_candela_1d(degree, h.num_hangles, vangles, h.num_vangles, candela);
			
			*scanline++ = f32tof16(cand);
		}

		dst = (uint16_t*)((uint8_t*)dst + mapped.row_pitch);
	}

	// Copy candela data

	img.unmap(0);

	return img;
}

}}
