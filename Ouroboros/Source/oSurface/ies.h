// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Standard structs and definitions for the IESNA LM-63 Photometric Data 
// File (.ies) file format.

// http://lumen.iee.put.poznan.pl/kw/iesna.txt

#pragma once
#include <cstdint>

static const uint32_t dds_signature = 0x20534444; // "DDS "

enum class ies_format
{
	unknown,
	iesna_86, // LM-63-1986
	iesna_91, // LM-63-1991
	iesna_95, // LM-63-1995
	iesna_02, // LM-63-2002
	
	count,
};

enum class ies_tilt : uint8_t
{
	unknown,
	none,
	include,
	filename,
	count,
};

enum class ies_photometric_type : uint8_t
{
	unknown,
	c,
	b,
	a,
	count,
};

enum class ies_units : uint8_t
{
	unknown,
	feet,
	meters,
	count,
};

enum class ies_geometry : uint8_t
{
	unknown, 
	point,
	rectangle,
	circle,
	sphere,
	verticle_cylinder,
	horizontal_cylinder_along_length,
	horizontal_cylinder_along_width,
	ellipse_along_length,
	ellipse_along_width,
	ellipsoid_along_length,
	ellipsoid_along_width,
};

struct ies_header
{
	ies_header()
		: format(ies_format::unknown)
		, tilt(ies_tilt::unknown)
		, num_lamps(0)
		, lumens_per_lamp(0.0f)
		, candela_multiplier(0.0f)
		, num_vangles(0)
		, num_hangles(0)
		, photometric_type(ies_photometric_type::unknown)
		, units(ies_units::unknown)
		, geometry(ies_geometry::unknown)
		, width(0.0f)
		, length(0.0f)
		, height(0.0f)
	{}

	ies_format format;
	ies_tilt tilt;
	int32_t num_lamps;
	float lumens_per_lamp;
	float candela_multiplier;
	int32_t num_vangles;
	int32_t num_hangles;
	ies_photometric_type photometric_type;
	ies_units units;
	ies_geometry geometry;
	float width;
	float length;
	float height;
	float ballast_factor;
	float ballast_lamp_photometric_factor;
	float input_watts;
};

ies_geometry to_geometry(float w, float l, float h);

// assumes ies_string is null-terminated. out_data receives a pointer to 
// the start of vangles
ies_header to_header(const char* ies_string, const char** out_data = nullptr);


// @tony todo expose more of the components

// the image returned can be made into a texture and the shader to sample it should be:
/*
float photometric_light_intensity(oIN(float3, WSpos), oIN(float3, WSlight_pos), oIN(float3, WSlight_dir))
{
	// identity/no profile: return 1.0f;

	const float3 dir_to_light = normalize(WSpos - WSlight_pos);
	const float angle = asin(dir_to_light); // you'll want to optimize this
	const float normalized_angle = angle * oINVPIf + 0.5f;
	return IEStexture.Sample(BilinSampler, float2(normalized_angle, 0.0f), 0.0f).x;
}
*/