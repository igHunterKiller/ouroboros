// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.
// API to make working with Window's GDI bitmaps and icons easier.
#pragma once
#ifndef oGUI_win_gdi_bitmap_h
#define oGUI_win_gdi_bitmap_h

#include <oGUI/Windows/win_gdi.h>
#include <oSurface/image.h>

namespace ouro {
	namespace windows {
		namespace gdi {

// Supported formats: b8g8r8_unorm b8g8r8a8_unorm r8_unorm. Must be 2D non-array
BITMAPINFOHEADER make_header(const surface::info_t& info, bool top_down = true);
BITMAPV4HEADER make_headerv4(const surface::info_t& info, bool top_down = true);
void fill_monochrone_palette(RGBQUAD* out_colors, uint32_t argb0 = color::black, uint32_t argb1 = color::white);

surface::info_t get_info(const BITMAPINFOHEADER& header);
surface::info_t get_info(const BITMAPV4HEADER& header);
inline surface::info_t get_info(const BITMAPINFO& bitmap_info) { return get_info(bitmap_info.bmiHeader); }

// Creates a bitmap with the contents of the specified buffer.
scoped_bitmap make_bitmap(const surface::image* img);

// Copy the contents of a bitmap to the specified buffer.
void memcpy2d(void* _pDestination, size_t _DestinationPitch, HBITMAP _hBmp, size_t _NumRows, bool _FlipVertically = false);

// Draws the specified bitmap to the device context using the ROP operation
// as described in BitBlt documentation.
void draw_bitmap(HDC _hDC, int _X, int _Y, HBITMAP _hBitmap, DWORD _dwROP);
void stretch_bitmap(HDC _hDC, int _X, int _Y, int _Width, int _Height, HBITMAP _hBitmap, DWORD _dwROP);
void stretch_blend_bitmap(HDC _hDC, int _X, int _Y, int _Width, int _Height, HBITMAP _hBitmap);

// This stretches the source bits directly to fill the specified rectangle in
// the specified device context.
void stretch_bits(HDC _hDC, const RECT& _DestRect, const int2& _SourceSize, surface::format _SourceFormat, const void* _pSourceBits, bool _FlipVertically = true);

// This stretches the source bits directly to fill the specified HWND's client
// area. This is a nice shortcut when working with cameras that will fill some
// UI element, or a top-level window.
void stretch_bits(HWND _hWnd, const int2& _SourceSize, surface::format _SourceFormat, const void* _pSourceBits, bool _FlipVertically = true);
void stretch_bits(HWND _hWnd, const RECT& _DestRect, const int2& _SourceSize, surface::format _SourceFormat, const void* _pSourceBits, bool _FlipVertically = true);

// Returns the width and height of the bitmap in the _hDC
int2 bitmap_dimensions(HDC _hDC);

int2 icon_dimensions(HICON _hIcon);

// Use DeleteObject when finished with the returned handle
HBITMAP bitmap_from_icon(HICON _hIcon);

// Use DestroyIcon when finished with the return handle
HICON bitmap_to_icon(HBITMAP _hBmp);

// Use DestroyIcon when finished with the return handle
inline HICON load_icon(int _ResourceID) { return (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(_ResourceID), IMAGE_ICON, 0, 0, 0); }

		} // namespace gdi
	}
}

#endif
