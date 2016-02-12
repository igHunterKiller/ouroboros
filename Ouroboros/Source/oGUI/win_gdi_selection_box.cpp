// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oGUI/windows/win_gdi_selection_box.h>
#include <oGUI/windows/win_gdi_draw.h>
#include <oMath/quantize.h>

namespace ouro { namespace windows { namespace gdi {

selection_box::selection_box()
	: MouseDownAt(0,0)
	, MouseAt(0,0)
	, Opacity(0.0f)
	, Selecting(false)
	, hNullBrush(make_brush(color::null))
{
}

const selection_box& selection_box::operator=(selection_box&& _That)
{
	if (this != &_That)
	{
		MouseDownAt = std::move(_That.MouseDownAt);
		MouseAt = std::move(_That.MouseAt);
		Opacity = std::move(_That.Opacity);
		Selecting = std::move(_That.Selecting);
		hPen = std::move(_That.hPen);
		hBrush = std::move(_That.hBrush);
		hNullBrush = std::move(_That.hNullBrush);
		hOffscreenBMP = std::move(_That.hOffscreenBMP);
		Info = std::move(_That.Info);
	}
	return *this;
}

void selection_box::set_info(const info& _Info)
{
	if (!_Info.hParent)
		throw std::invalid_argument("A valid HWND for a parent window must be specified");

	if (Info.hParent != _Info.hParent)
	{
		Selecting = false;
		hOffscreenBMP = nullptr;
	}

	hPen = make_pen(_Info.Border);
	hBrush = make_brush(_Info.Fill);

	argb_channels ch;
	ch.argb = _Info.Fill;
	Opacity = n8tof32(ch.a);

	Info = _Info;
}

selection_box::info selection_box::get_info() const 
{
	return Info;
}

bool selection_box::selecting() const
{
	return Selecting;	
}

void selection_box::draw(HDC _hDC)
{
	if (!Selecting)
		return;

	const RECT SelRect = oWinRect(MouseDownAt, MouseAt);
	const int2 SelSize = oWinRectSize(SelRect);

	// by default, don't use an offscreen buffer. Rounded edge rect drawing can
	// crash Windows if rendering to an HDC from a DXGI back-buffer, so that's why
	// we go off-screen here.
	HDC hResolveDC = _hDC, hTargetDC = _hDC;
	
	if (Info.EdgeRoundness && Info.UseOffscreenRender && !hOffscreenBMP)
	{
		RECT rParent;
		GetClientRect(Info.hParent, &rParent);
		hTargetDC = CreateCompatibleDC(_hDC);
		hOffscreenBMP = CreateCompatibleBitmap(hTargetDC, oWinRectW(rParent), oWinRectH(rParent));
		SelectObject(hTargetDC, hOffscreenBMP);
	}

	if (Info.EdgeRoundness && Info.UseOffscreenRender)
		BitBlt(hTargetDC, SelRect.left, SelRect.top, SelSize.x, SelSize.y, hResolveDC, SelRect.left, SelRect.top, SRCCOPY);

	{
		scoped_select B(hTargetDC, hBrush);
		draw_box(hTargetDC, SelRect, Info.EdgeRoundness, Opacity);
	}
	
	{
		scoped_select P(hTargetDC, hPen);
		scoped_select B(hTargetDC, hNullBrush);
		draw_box(hTargetDC, SelRect, Info.EdgeRoundness, 1.0f);
	}

	if (Info.EdgeRoundness && Info.UseOffscreenRender)
		BitBlt(hResolveDC, SelRect.left, SelRect.top, SelSize.x, SelSize.y, hTargetDC, SelRect.left, SelRect.top, SRCCOPY);
}

void selection_box::on_resize(const int2& _NewParentSize)
{
	// pick up realloc in Draw()
	hOffscreenBMP = nullptr;
}

void selection_box::on_mouse_down(const int2& _MousePosition)
{
	MouseDownAt = MouseAt = _MousePosition;
	Selecting = true;
	SetCapture(Info.hParent);
}

void selection_box::on_mouse_move(const int2& _MousePosition)
{
	MouseAt = _MousePosition;
}

void selection_box::on_mouse_up()
{
	Selecting = false;
	ReleaseCapture();
}

}}}
