// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oSurface/fill.h>
#include <oMath/quantize.h>

namespace ouro { namespace surface {

static inline uint32_t* scanline(fill_image_t* img, uint32_t n)
{
	return (uint32_t*)((uint8_t*)img->argb_surface + img->row_pitch * n);
}

void fill_solid(fill_image_t* out_img, uint32_t argb, uint32_t element_write_mask)
{
	if (element_write_mask == 0xffffffff)
	{
		for (uint32_t y = 0; y < out_img->num_rows; y++)
		{
			uint32_t* row = scanline(out_img, y);
			for (uint32_t x = 0; x < out_img->row_width; x++)
				row[x] = argb;
		}
	}

	else
	{
		uint32_t masked = argb & element_write_mask;
		uint32_t clear  = ~element_write_mask;
		for (uint32_t y = 0; y < out_img->num_rows; y++)
		{
			uint32_t* row = scanline(out_img, y);
			for (uint32_t x = 0; x < out_img->row_width; x++)
				row[x] = masked | (row[x] & clear);
		}
	}
}

void fill_checker(fill_image_t* out_img, uint32_t grid_width, uint32_t grid_height, uint32_t argb0, uint32_t argb1)
{
	uint32_t c[2];
	for (uint32_t y = 0; y < out_img->num_rows; y++)
	{
		int tileY = y / grid_height;

		if (tileY & 0x1)
		{
			c[0] = argb1;
			c[1] = argb0;
		}

		else
		{
			c[0] = argb0;
			c[1] = argb1;
		}

		uint32_t* row = scanline(out_img, y);
		for (uint32_t x = 0; x < out_img->row_width; x++)
		{
			int tileX = x / grid_width;
			row[x] = c[tileX & 0x1];
		}
	}
}

void fill_gradient(fill_image_t* out_img, const uint32_t corner_argbs[4])
{
	float4 corner[4];
	for (int i = 0; i < 4; i++)
		corner[i] = truetofloat4(corner_argbs[i]);

	for (uint32_t y = 0; y < out_img->num_rows; y++)
	{
		uint32_t* row = scanline(out_img, y);
		float ry      = y / static_cast<float>(out_img->num_rows - 1);
		for (uint32_t x = 0; x < out_img->row_width; x++)
		{
			auto rx  = x / static_cast<float>(out_img->row_width - 1);
			auto top = lerp(corner[0], corner[1], rx);
			auto bot = lerp(corner[2], corner[3], rx);
			auto col = lerp(top, bot, ry);
			row[x]   = float4totrue(col);
		}
	}
}

void fill_grid(fill_image_t* out_img, uint32_t grid_width, uint32_t grid_height, uint32_t argb)
{
	for (uint32_t y = 0; y < out_img->num_rows; y++)
	{
		uint32_t* row  = scanline(out_img, y);
		uint32_t  modY = y % grid_height;
		if (modY == 0 || modY == (grid_height - 1))
		{
			for (uint32_t x = 0; x < out_img->row_width; x++)
				row[x] = argb;
		}

		else
		{
			for (uint32_t x = 0; x < out_img->row_width; x++)
			{
				uint32_t modX = x % grid_width;
				if (modX == 0 || modX == (grid_width - 1))
					row[x] = argb;
			}
		}
	}
}

void fill_grid_numbers(uint32_t width, uint32_t height, uint32_t grid_width, uint32_t grid_height, void (*draw_text)(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const char* text, void* user), void* user)
{
	uint32_t posx, posy;

	uint32_t i = 0;
	for (uint32_t y = 0; y < height; y += grid_height)
	{
		posy = y;
		for (uint32_t x = 0; x < width; x += grid_width)
		{
			posx = x;
			char buf[64];
			snprintf(buf, "%d", i++);
			draw_text(posx, posy, grid_width, grid_height, buf, user);
		}
	}
}

void fill_color_cube(fill_image_t* out_img, uint32_t slice_pitch, uint32_t num_slices)
{
	float3 fcolor(0.0f, 0.0f, 0.0f);
	float3 step = 1.0f / (float3((float)out_img->row_width, (float)out_img->num_rows, (float)num_slices) - 1.0f);

	for (uint32_t z = 0; z < num_slices; z++)
	{
		uint32_t* slice = (uint32_t*)((uint8_t*)out_img->argb_surface + z * slice_pitch);

		fcolor.y = 0.0f;
		for (uint32_t y = 0; y < out_img->num_rows; y++)
		{
			uint32_t* row = (uint32_t*)((uint8_t*)slice + y * out_img->row_pitch);
			fcolor.x = 0.0f;
			for (uint32_t x = 0; x < out_img->row_width; x++)
			{
				row[x] = float4totrue(float4(fcolor.x, fcolor.y, fcolor.z, 1.0f));
				fcolor.x += step.x;
			}

			fcolor.y += step.y;
		}

		fcolor.z += step.z;
	}
}

}}
