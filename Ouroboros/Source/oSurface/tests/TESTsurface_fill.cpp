// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include <oSurface/fill.h>
#include <oSurface/codec.h>
#include <oCore/color.h>
#include <oCore/timer.h>
#include <vector>

namespace ouro { namespace tests {

void core_fill_grid_numbers(surface::image* _pBuffer, const int2& _GridDimensions, uint32_t _NumberColor);

}}

using namespace ouro;

#define SETUP_AND_MAKE() \
	surface::info_t si; \
	si.format = surface::format::b8g8r8a8_unorm; \
	si.mip_layout = surface::mip_layout::none; \
	si.dimensions = uint3(_Dimensions, 1); \
	surface::image s(si);

surface::image make_numbered_grid(
	const int2& _Dimensions
	, const int2& _GridDimensions
	, uint32_t _GridColor
	, uint32_t _NumberColor
	, const uint32_t _CornerColors[4])
{
	SETUP_AND_MAKE();

	{
		surface::lock_guard lock(s);
		surface::fill_image_t img;
		img.argb_surface = (uint32_t*)lock.mapped.data;
		img.row_width    = si.dimensions.x;
		img.num_rows     = si.dimensions.y;
		img.row_pitch    = lock.mapped.row_pitch;

		surface::fill_gradient(&img, _CornerColors);
		surface::fill_grid    (&img, _GridDimensions.x, _GridDimensions.y, _GridColor);
	}
	
	tests::core_fill_grid_numbers(&s, _GridDimensions, _NumberColor);
	return s;
}

surface::image make_checkerboard(
	const int2& _Dimensions
	, const int2& _GridDimensions
	, uint32_t _Color0
	, uint32_t _Color1)
{
	SETUP_AND_MAKE();
	surface::lock_guard lock(s);
	surface::fill_image_t img;
	img.argb_surface = (uint32_t*)lock.mapped.data;
	img.row_width    = si.dimensions.x;
	img.num_rows     = si.dimensions.y;
	img.row_pitch    = lock.mapped.row_pitch;

	surface::fill_checker(&img, 64, 64, (uint32_t)_Color0, (uint32_t)_Color1);
	return s;
}

surface::image make_solid(const int2& _Dimensions, uint32_t _argb)
{
	SETUP_AND_MAKE();
	surface::lock_guard lock(s);
	surface::fill_image_t img;
	img.argb_surface = (uint32_t*)lock.mapped.data;
	img.row_width    = si.dimensions.x;
	img.num_rows     = si.dimensions.y;
	img.row_pitch    = lock.mapped.row_pitch;
	surface::fill_solid(&img, _argb);
	return s;
}

oTEST(oSurface_surface_fill)
{
	static const uint32_t gradiantColors0[4] = { color::blue, color::purple, color::lime, color::orange };
	static const uint32_t gradiantColors1[4] = { color::midnight_blue, color::dark_slate_blue, color::green, color::chocolate };
	surface::image s;
	s = make_numbered_grid(int2(256,256), int2(64,64), color::black, color::black, gradiantColors0);
	srv.check(s, 0);
	s = make_numbered_grid(int2(512,512), int2(32,32), color::gray, color::white, gradiantColors1);
	srv.check(s, 1);
	s = make_checkerboard(int2(256,256), int2(32,32), color::cyan, color::pink);
	srv.check(s, 2);
	s = make_solid(int2(256,256), color::tangent_space_normal_blue);
	srv.check(s, 3);
}
