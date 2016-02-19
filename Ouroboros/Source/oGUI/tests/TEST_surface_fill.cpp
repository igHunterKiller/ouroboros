// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include <oSurface/fill.h>
#include <oGUI/Windows/win_gdi.h>
#include <oGUI/Windows/win_gdi_bitmap.h>
#include <oGUI/Windows/win_gdi_draw.h>

using namespace ouro;
using namespace ouro::windows::gdi;

namespace ouro { namespace tests {

struct ctx_t
{
	HDC hDC;
	uint32_t color;
};

static void draw_text(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const char* text, void* user)
{
	const ctx_t& ctx = *(ctx_t*)user;

	text_info td;
	td.alignment = alignment::middle_center;
	td.argb_shadow = color::null;
	td.single_line = true;
	td.position = int2(x,y);
	td.size = int2(w,h);
	td.argb_foreground = ctx.color;
	::draw_text(ctx.hDC, td, text);
};

void core_fill_grid_numbers(surface::image* _pBuffer, const int2& _GridDimensions, uint32_t _NumberColor)
{
	const auto& si = _pBuffer->info();

	if (si.format != surface::format::b8g8r8a8_unorm)
		oThrow(std::errc::invalid_argument, "only b8g8r8a8_unorm currently supported");

	scoped_bitmap hBmp = make_bitmap(_pBuffer);
	scoped_getdc hScreenDC(nullptr);
	scoped_compat hDC(hScreenDC);

	font_info fd;
	fd.name = "Tahoma";
	fd.point_size = (logical_height_to_pointf(hDC, _GridDimensions.y) * 0.33f);
	fd.bold = fd.point_size < 15;
	scoped_font hFont(make_font(fd));

	scoped_select ScopedSelectBmp(hDC, hBmp);
	scoped_select ScopedSelectFont(hDC, hFont);

	text_info td;
	td.alignment = alignment::middle_center;
	td.argb_shadow = color::null;
	td.single_line = true;

	ctx_t ctx;
	ctx.hDC = hDC;
	ctx.color = _NumberColor;
	surface::fill_grid_numbers(si.dimensions.x, si.dimensions.y, _GridDimensions.x, _GridDimensions.y, draw_text, &ctx);

	surface::lock_guard lock(_pBuffer);
	windows::gdi::memcpy2d(lock.mapped.data, lock.mapped.row_pitch, hBmp, si.dimensions.y, true);
}

}}