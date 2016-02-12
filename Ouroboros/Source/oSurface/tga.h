// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Standard structs and definitions for the targa (.tga) image file format.

#pragma once
#include <cstdint>

enum tga_data_type_field
{
  no_image_data                = 0,
  paletted                     = 1,
  rgb                          = 2,
  black_and_white              = 3,
  rle_paletted                 = 9,
  rle_rgb                      = 10,
  compressed_black_and_white   = 11,
  compressed_paletted          = 32,
  compressed_paletted_quadtree = 33,
};

#pragma pack(push,1)
struct tga_header
{
  uint8_t id_length;
  uint8_t paletted_type;
  uint8_t data_type_field;
  uint16_t paletted_origin;
  uint16_t paletted_length;
  uint8_t paletted_depth;
  uint16_t x_origin;
  uint16_t y_origin;
  uint16_t width;
  uint16_t height;
  uint8_t bpp;
  uint8_t image_descriptor;
};
#pragma pack(pop)

inline bool tga_is_valid_dtf(uint8_t dtf)
{
	switch (dtf)
	{
		case no_image_data:
		case paletted:
		case rgb:
		case black_and_white:
		case rle_paletted:
		case rle_rgb:
		case compressed_black_and_white:
		case compressed_paletted:
		case compressed_paletted_quadtree:
			return true;
		default: break;
	}
	return false;
}
