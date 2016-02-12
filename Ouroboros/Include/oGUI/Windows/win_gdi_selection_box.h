// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.
// A 2D screen space selection box implemented using Windows GDI.
#pragma once
#ifndef oGUI_win_gdi_selection_box_h
#define oGUI_win_gdi_selection_box_h

#include <oGUI/Windows/win_gdi.h>

namespace ouro { namespace windows { namespace gdi {

class selection_box
{
	// Like when you drag to select several files in Windows Explorer. This uses
	// SetCapture() and ReleaseCapture() on hParent to retain mouse control while
	// drawing the selection box.

	selection_box(const selection_box&);
	const selection_box& operator=(const selection_box&);

public:
	struct info
	{
		info()
			: hParent(nullptr)
			, Fill(color::dodger_blue)
			, Border(color::dodger_blue)
			, EdgeRoundness(0)
			, UseOffscreenRender(false)
		{
			argb_channels ch; ch.argb = Fill;
			ch.a = 85;
			Fill = ch.argb;
		}

		HWND hParent;
		uint32_t Fill; // argb can have alpha value
		uint32_t Border; // argb
		int EdgeRoundness;

		// DXGI back-buffers don't like rounded edge rendering, so copy contents
		// to an offscreen HBITMAP, draw the rect and resolve it back to the main
		// target. Even if this is true, offscreen render only occurs if there is
		// edge roundness.
		bool UseOffscreenRender;
	};

	selection_box();
	selection_box(selection_box&& _That) { operator=(_That); }
	const selection_box& operator=(selection_box&& _That);

	void set_info(const info& _Info);
	info get_info() const;

	// returns true if drawing is required by the state (between mouse down and 
	// mouse up events). This is useful if there's non-trivial effort in 
	// extracting the HDC that draw will use.
	bool selecting() const;

	void draw(HDC _hDC);
	void on_resize(const int2& _NewParentSize);
	void on_mouse_down(const int2& _MousePosition);
	void on_mouse_move(const int2& _MousePosition);
	void on_mouse_up();

private:
	scoped_pen hPen;
	scoped_brush hBrush;
	scoped_brush hNullBrush;
	scoped_bitmap hOffscreenBMP;

	int2 MouseDownAt;
	int2 MouseAt;
	float Opacity;
	bool Selecting;

	info Info;
};

}}}

#endif
