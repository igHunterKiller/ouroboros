// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.
// API to make working with Window's GDI easier.
#pragma once
#ifndef oGUI_win_gdi_h
#define oGUI_win_gdi_h

#include <oGUI/Windows/oWinWindowing.h> // for scoped_offscreen
#include <oGUI/Windows/oWinRect.h> // for scoped_offscreen

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// extra casting is here in case type is std::atomic<>
#define oDEFINE_GDI_BOOL_CAST_OPERATORS(_Type, _Member) \
	operator bool() { return !!(_Type)_Member; } \
	operator bool() const { return !!(_Type)_Member; } \
	operator bool() volatile { return !!(_Type)_Member; } \
	operator bool() const volatile { return !!(_Type)_Member; } \
	operator _Type() { return (_Type)_Member; } \
	operator _Type() const { return (_Type)_Member; } \
	operator _Type() volatile { return (_Type)_Member; } \
	operator _Type() const volatile { return (_Type)_Member; }

#define oDEFINE_GDI_MOVE_PTR(_Name) do { _Name = _That._Name; _That._Name = nullptr; } while (false)

namespace ouro {
	namespace windows {
		namespace gdi {

// For functions that take logical height. Point size is like what appears in 
// most Windows API.
int point_to_logical_height(HDC _hDC, int _Point);
int point_to_logical_heightf(HDC _hDC, float _Point);

// Goes the opposite way of oGDIPointToLogicalHeight() and returns point size
int logical_height_to_point(HDC _hDC, int _Height);
float logical_height_to_pointf(HDC _hDC, int _Height);

inline float point_to_dip(float _Point) { return 96.0f * _Point / 72.0f; }
inline float dip_to_point(float _DIP) { return 72.0f * _DIP / 96.0f; }

float2 dpi_scale(HDC _hDC);
int estimate_point_size(int _PixelHeight);

inline HBITMAP current_bitmap(HDC _hDC) { return (HBITMAP)GetCurrentObject(_hDC, OBJ_BITMAP); }
inline HBRUSH current_brush(HDC _hDC) { return (HBRUSH)GetCurrentObject(_hDC, OBJ_BRUSH); }
inline HFONT current_font(HDC _hDC) { return (HFONT)GetCurrentObject(_hDC, OBJ_FONT); }
inline HPEN current_pen(HDC _hDC) { return (HPEN)GetCurrentObject(_hDC, OBJ_PEN); }

template<typename HGDIOBJType> class scoped_object
{
	HGDIOBJType hObject;

	scoped_object(const scoped_object& _That);
	const scoped_object& operator=(const scoped_object& _That);

public:
	scoped_object() : hObject(nullptr) {}
	scoped_object(scoped_object&& _That) { operator=(std::move(_That)); }
	scoped_object(HGDIOBJType _hObject) : hObject(_hObject) {}
	~scoped_object() { if (hObject) DeleteObject(hObject); }
	
	const scoped_object& operator=(HGDIOBJType _hObject)
	{
		if (hObject) DeleteObject(hObject);
		hObject = _hObject;
		return *this;
	}

	scoped_object& operator=(scoped_object&& _That)
	{
		if (this != &_That)
		{
			oDEFINE_GDI_MOVE_PTR(hObject);
		}
		return *this;
	}

	oDEFINE_GDI_BOOL_CAST_OPERATORS(HGDIOBJType, hObject);
};

typedef scoped_object<HBITMAP> scoped_bitmap;
typedef scoped_object<HBRUSH> scoped_brush;
typedef scoped_object<HFONT> scoped_font;
typedef scoped_object<HICON> scoped_icon;
typedef scoped_object<HPEN> scoped_pen;

class scoped_blt_mode
{
	// SetStretchBltMode to the specified mode, then restores the prior value
	// on scope exit.

	HDC hDC;
	int PrevMode;

	scoped_blt_mode(const scoped_blt_mode& _That);
	const scoped_blt_mode& operator=(const scoped_blt_mode& _That);

public:
	scoped_blt_mode() : hDC(nullptr) {}
	~scoped_blt_mode() { if (hDC) SetStretchBltMode(hDC, PrevMode); }
	scoped_blt_mode(HDC _hDC, int _Mode) : hDC(_hDC), PrevMode(SetStretchBltMode(hDC, _Mode)) {}
	scoped_blt_mode(scoped_blt_mode&& _That) { operator=(_That); }

	scoped_blt_mode& operator=(scoped_blt_mode&& _That)
	{
		if (this != &_That)
		{
			oDEFINE_GDI_MOVE_PTR(hDC);
			PrevMode = _That.PrevMode;
		}
		return *this;
	}

	oDEFINE_GDI_BOOL_CAST_OPERATORS(HDC, hDC);
};

class scoped_bk_mode
{
	// SetBkMode to the specified mode, then restores the prior value
	// on scope exit.

	HDC hDC;
	int PrevMode;

	scoped_bk_mode(const scoped_bk_mode& _That);
	const scoped_bk_mode& operator=(const scoped_bk_mode& _That);

public:
	scoped_bk_mode() : hDC(nullptr) {}
	~scoped_bk_mode() { if (hDC) SetBkMode(hDC, PrevMode); }
	scoped_bk_mode(HDC _hDC, int _Mode) : hDC(_hDC), PrevMode(SetBkMode(hDC, _Mode)) {}
	scoped_bk_mode(scoped_bk_mode&& _That) { operator=(_That); }

	scoped_bk_mode& operator=(scoped_bk_mode&& _That)
	{
		if (this != &_That)
		{
			oDEFINE_GDI_MOVE_PTR(hDC);
			PrevMode = _That.PrevMode;
		}
		return *this;
	}

	oDEFINE_GDI_BOOL_CAST_OPERATORS(HDC, hDC);
};

class scoped_clip_region
{
	// Selects a new clip region into the specified HDC and restores the prior
	// one on scope exit.

	HDC hDC;
	HRGN PrevRegion;

	scoped_clip_region(const scoped_bk_mode& _That);
	const scoped_clip_region& operator=(const scoped_clip_region& _That);

public:
	scoped_clip_region() : hDC(nullptr), PrevRegion(nullptr) {}
	~scoped_clip_region() 
	{
		// If there was no clip region set before our scoped one, resetting to 
		// no clip region is done by calling the function with nullptr,
		// which means we should always call this function (If hDC is valid).
		if (hDC)
			SelectClipRgn(hDC, PrevRegion); 
		if (PrevRegion) 
			DeleteObject(PrevRegion); 
	}
	scoped_clip_region(HDC _hDC, const RECT& _Region) : hDC(_hDC), PrevRegion(nullptr) 
	{
		// Unfortunately it seems that in order to get the currently set clip
		// region, we first have to create one for GetClipRgn to copy into.
		// Also see:
		// http://stackoverflow.com/questions/3478180/correct-usage-of-getcliprgn
		PrevRegion = CreateRectRgn(0,0,0,0);
		if (1 != GetClipRgn(hDC, PrevRegion))
		{
			// If there was no clip region set, then we have to set PrevRegion
			// to nullptr so that the destructor will do the correct thing.
			// So we have to clean up the one we just created.
			DeleteObject(PrevRegion);
			PrevRegion = nullptr;
		}
		IntersectClipRect(hDC, _Region.left, _Region.top, _Region.right, _Region.bottom);
	}
	scoped_clip_region(scoped_clip_region&& _That) { operator=(_That); }

	scoped_clip_region& operator=(scoped_clip_region&& _That)
	{
		if (this != &_That)
		{
			oDEFINE_GDI_MOVE_PTR(hDC);
			oDEFINE_GDI_MOVE_PTR(PrevRegion);
		}
		return *this;
	}

	oDEFINE_GDI_BOOL_CAST_OPERATORS(HDC, hDC);
};

class scoped_compat
{
	// Creates a new DC compatible with the specified one and 
	// destroys it on scope exit.

	HDC hDC;

	scoped_compat(const scoped_compat& _That);
	const scoped_compat& operator=(const scoped_compat& _That);

public:
	scoped_compat() : hDC(nullptr) {}
	~scoped_compat() { if (hDC) DeleteDC(hDC); }
	scoped_compat(HDC _hDC) : hDC(CreateCompatibleDC(_hDC)) {}
	scoped_compat(scoped_compat&& _That) { operator=(_That); }

	const scoped_compat& operator=(HDC _hDC)
	{
		if (hDC) DeleteDC(hDC);
		hDC = _hDC;
		return *this;
	}

	scoped_compat& operator=(scoped_compat&& _That)
	{
		if (this != &_That)
		{
			if (hDC) DeleteDC(hDC);
			oDEFINE_GDI_MOVE_PTR(hDC);
		}
		return *this;
	}

	oDEFINE_GDI_BOOL_CAST_OPERATORS(HDC, hDC);
};

class scoped_getdc
{
	// Calls GetDC on the specified HWND and ReleaseDC on scope exit.

	HWND hWnd;
	HDC hDC;

	scoped_getdc(const scoped_getdc&);
	const scoped_getdc& operator=(const scoped_getdc&);

public:
	scoped_getdc() : hWnd(nullptr), hDC(nullptr) {}
	scoped_getdc(HWND _hWnd) : hWnd(_hWnd), hDC(GetDC(_hWnd)) {}
	scoped_getdc(scoped_getdc&& _That) { operator=(_That); }
	~scoped_getdc() { if (hWnd && hDC) ReleaseDC(hWnd, hDC); }

	scoped_getdc& operator=(scoped_getdc&& _That)
	{
		if (this != &_That)
		{
			if (hWnd && hDC)
				ReleaseDC(hWnd, hDC);
			oDEFINE_GDI_MOVE_PTR(hWnd);
			oDEFINE_GDI_MOVE_PTR(hDC);
		}
		return *this;
	}

	oDEFINE_GDI_BOOL_CAST_OPERATORS(HDC, hDC);
};

class scoped_select
{
	// Selects the GDI object into the HDC and restores the prior value
	// on scope exit.

	HDC hDC;
	HGDIOBJ hOldObj;

	scoped_select(const scoped_getdc&);
	const scoped_select& operator=(const scoped_select&);

public:
	scoped_select() : hDC(nullptr), hOldObj(nullptr) {}
	scoped_select(HDC _hDC, HGDIOBJ _hObj) : hDC(_hDC) { hOldObj = SelectObject(hDC, _hObj); }
	scoped_select(scoped_select&& _That) { operator=(std::move(_That)); }
	~scoped_select() { SelectObject(hDC, hOldObj); }
	scoped_select& operator=(scoped_select&& _That)
	{
		if (this != &_That)
		{
			if (hDC && hOldObj) SelectObject(hDC, hOldObj);
			oDEFINE_GDI_MOVE_PTR(hDC);
			oDEFINE_GDI_MOVE_PTR(hOldObj);
		}
		return *this;
	}
};

class scoped_text_color
{
	// Calls SetTextColor SetBkColor and SetBkMode appropriately and restores
	// the prior state on scope exit.

	HDC hDC;
	COLORREF OldTextColor;
	COLORREF OldBkColor;
	int OldBkMode;
public:
	scoped_text_color(HDC _hDC, COLORREF _Foreground, COLORREF _Background, int _Alpha)
		: hDC(_hDC)
		, OldTextColor(SetTextColor(_hDC, _Foreground))
		, OldBkColor(SetBkColor(_hDC, _Background))
		, OldBkMode(SetBkMode(_hDC, _Alpha != 0 ? OPAQUE : TRANSPARENT))
	{}

	~scoped_text_color()
	{
		SetBkMode(hDC, OldBkMode);
		SetBkColor(hDC, OldBkColor);
		SetTextColor(hDC, OldTextColor);
	}
};

class scoped_offscreen
{
	// Creates an offscreen rendering context for a window's client area.

	scoped_getdc hDCWin;
	scoped_compat hDCOffscreen;
	scoped_bitmap hOffscreen;
	scoped_select SelectOffscreen;
	RECT rClient;

public:
	scoped_offscreen() {}
	scoped_offscreen(HWND _hWnd)
		: hDCWin(_hWnd)
		, hDCOffscreen(CreateCompatibleDC(hDCWin))
	{
		oWinGetClientRect(_hWnd, &rClient);
		hOffscreen = CreateCompatibleBitmap(hDCWin, oWinRectW(rClient), oWinRectH(rClient));
		SelectOffscreen = scoped_select(hDCOffscreen, hOffscreen);
	}
	scoped_offscreen(scoped_offscreen&& _That) { operator=(std::move(_That)); }
	~scoped_offscreen() { BitBlt(hDCWin, 0, 0, oWinRectW(rClient), oWinRectH(rClient), hDCOffscreen, 0, 0, SRCCOPY); }

	const scoped_offscreen& operator=(HWND _hWnd)
	{
		scoped_offscreen off(_hWnd);
		*this = std::move(off);
		return *this;
	}

	scoped_offscreen& operator=(scoped_offscreen&& _That)
	{
		if (this != &_That)
		{
			hDCWin = std::move(_That.hDCWin);
			hDCOffscreen = std::move(_That.hDCOffscreen);
			hOffscreen = std::move(_That.hOffscreen);
			rClient = std::move(_That.rClient);
		}
		return *this;
	}

	oDEFINE_GDI_BOOL_CAST_OPERATORS(HDC, hDCOffscreen);
};

		} // namespace gdi
	}
}

#endif
