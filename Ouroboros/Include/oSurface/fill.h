// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// fill surfaces with color in various patterns, mostly for debug images/textures

#pragma once
#include <cstdint>

namespace ouro { namespace surface {

struct fill_image_t
{
	uint32_t* argb_surface;
	uint32_t  row_width; // argb count in a row
	uint32_t  num_rows;  // row count
	uint32_t  row_pitch; // byte width of a row
};

// 2D
void fill_solid   (fill_image_t* out_img, uint32_t argb, uint32_t element_write_mask = 0xffffffff);
void fill_checker (fill_image_t* out_img, uint32_t grid_width, uint32_t grid_height, uint32_t argb0, uint32_t argb1);
void fill_gradient(fill_image_t* out_img, const uint32_t corner_argbs[4]);
void fill_grid    (fill_image_t* out_img, uint32_t grid_width, uint32_t grid_height, uint32_t argb);

// writes numbers in the center of a grid as defined by fill_grid
void fill_grid_numbers(uint32_t width, uint32_t height, uint32_t grid_width, uint32_t grid_height, void (*draw_text)(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const char* text, void* user), void* user);

// fills a 3D texture with gradients: red along U, green along V, blue along W
void fill_color_cube(fill_image_t* out_img, uint32_t slice_pitch, uint32_t num_slices);

}}
