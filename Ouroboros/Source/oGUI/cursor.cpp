// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oCore/countof.h>
#include <oCore/stringf.h>
#include <oGUI/cursor.h>
#include <oSystem/windows/win_error.h>
#include <oString/stringize.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define oWINV() if (!::IsWindow((HWND)window)) throw std::invalid_argument(stringf("Invalid HWND %p specified", window));

namespace ouro { namespace cursor {

void set(void* window, const mouse_cursor& c, void* user_cursor)
{
	void* cur = system_cursor(c == mouse_cursor::none ? mouse_cursor::arrow : c, user_cursor);
	if (window)
	{
		oWINV();
		set_default(window, cur);
	}

	else
		SetCursor((HCURSOR)cur);
	show(c != mouse_cursor::none);
}

mouse_cursor get(void* window)
{
	if (!shown())
		return mouse_cursor::none;
	
	void* cur = nullptr;
	if (window)
	{
		oWINV();
		cur = (void*)(HCURSOR)GetClassLongPtr((HWND)window, GCLP_HCURSOR);
	}
	else
		cur = GetCursor();

	for (int i = (int)mouse_cursor::arrow; i < (int)mouse_cursor::user; i++)
	{
		void* state_cur = system_cursor((mouse_cursor)i);
		if (cur == state_cur)
			return (mouse_cursor)i;
	}

	return mouse_cursor::user;
}

void* get_default(void* window)
{
	oWINV();
	return (void*)(HCURSOR)GetClassLongPtr((HWND)window, GCLP_HCURSOR);
}

void set_default(void* window, void* cursor)
{
	oWINV();
	oVB(SetClassLongPtr((HWND)window, GCLP_HCURSOR, (LONG_PTR)cursor));
}

void* system_cursor(const mouse_cursor& c, void* user_cursor)
{
	LPSTR cursors[] =
	{
		nullptr,
		IDC_ARROW,
		IDC_HAND,
		IDC_HELP,
		IDC_NO,
		IDC_WAIT,
		IDC_APPSTARTING,
		nullptr,
	};
	match_array_e(cursors, mouse_cursor);

	return c == mouse_cursor::user ? user_cursor : LoadCursor(nullptr, cursors[(size_t)c]);
}

void position(int16_t x, int16_t y, void* window)
{
	POINT pt = { x, y };
	if (window)
	{
		oWINV();
		oVB(ClientToScreen((HWND)window, &pt));
	}

	oVB(SetCursorPos(pt.x, pt.y));
}

void position(int16_t* out_x, int16_t* out_y, void* window)
{
	POINT pt;
	oVB(GetCursorPos(&pt));
	if (window)
	{
		oWINV();
		oVB(ScreenToClient((HWND)window, &pt));
	}

	*out_x = static_cast<int16_t>(pt.x);
	*out_y = static_cast<int16_t>(pt.y);
}

void show(bool shown)
{
	if (shown)
		while (ShowCursor(true) < 0) {}
	else
		while (ShowCursor(false) > -1) {}
}

bool shown()
{
	CURSORINFO ci;
	ci.cbSize = sizeof(CURSORINFO);
	GetCursorInfo(&ci);
	return ci.flags == CURSOR_SHOWING;
}

static RECT honest_client_screen_rect(HWND hwnd)
{
	// oWinWindowing spoofs the client size to make the status bar considered
	// part of the window itself, so get the real windowsy client size.
	RECT r;
	oVB(GetClientRect(hwnd, &r));
	POINT pt = { r.left, r.top };
	oVB(ClientToScreen(hwnd, &pt));
	OffsetRect(&r, pt.x, pt.y);
	return r;
}

bool clipped(void* window)
{
	oWINV();
	RECT rClip, rClient = honest_client_screen_rect((HWND)window);
	oVB(GetClipCursor(&rClip));
	return !memcmp(&rClip, &rClient, sizeof(RECT));
}

void clip(void* window, bool clipped)
{
	oWINV();
	
	RECT backing;
	RECT* r = &backing;

	if (clipped)
		*r = honest_client_screen_rect((HWND)window); // allow cursor over status bar
	else
		r = nullptr;

	oVB(ClipCursor(r));
}

void* captured()
{
	return (void*)GetCapture();
}

void capture(void* window, int16_t* out_x, int16_t* out_y)
{
	if (window)
	{
		oWINV();
		SetCapture((HWND)window);

		POINT pt;
		oVB(GetCursorPos(&pt));
		oVB(ScreenToClient((HWND)window, &pt));
		if (out_x) *out_x = static_cast<int16_t>(pt.x);
		if (out_y) *out_y = static_cast<int16_t>(pt.y);
	}

	else
	{
		ReleaseCapture();
		if (out_x) *out_x = 0;
		if (out_y) *out_y = 0;
	}
}

void move_offscreen()
{
	int2 p, sz;
	display::virtual_rect(&p.x, &p.y, &sz.x, &sz.y);
	SetCursorPos(p.x + sz.x, p.y-1);
}

}}
