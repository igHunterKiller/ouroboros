// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oSystem/peripherals.h>
#include <oSystem/windows/win_error.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>

namespace ouro {

mouse_t::mouse_t()
{
	initialize();
}

mouse_t::~mouse_t()
{
	deinitialize();
}

void mouse_t::initialize()
{
	memset(this, 0, sizeof(*this));
}

void mouse_t::deinitialize()
{
	memset(this, 0, sizeof(*this));
}

void mouse_t::update()
{
	static const uint8_t mask = (uint8_t)~((uint8_t)mouse_button::dbl_left|(uint8_t)mouse_button::dbl_middle|(uint8_t)mouse_button::dbl_right);
	const uint8_t pre_dn = cur_dn;
	cur_dn &= mask; // dblclicks are only down for one frame
	cur_dn |= msg_dn; // take in all new frame downs
	cur_dn &= ~msg_up; // clear all the ups

	const uint16_t changed = (pre_dn ^ cur_dn) & mask; // ignore dblclicks for pressed/released
	cur_pr = changed & cur_dn;
	cur_re = changed & pre_dn;
	msg_up = 0;
	msg_dn = 0;

	status_ = peripheral_status::full_power;

	cur_x = evt_x; //evt_x = 0;
	cur_y = evt_y; //evt_y = 0;
	dlt_x = evt_dlt_x; evt_dlt_x = 0;
	dlt_y = evt_dlt_y; evt_dlt_y = 0;
	dlt_w = evt_dlt_w; evt_dlt_w = 0;
}

void mouse_t::reset()
{
	memset(this, 0, sizeof(*this));
}

void mouse_t::trigger(const mouse_event& e)
{
	switch (e.type)
	{
		case mouse_event::press:
		{
			uint8_t& clr = e.pr.down ? msg_up : msg_dn;
			uint8_t& set = e.pr.down ? msg_dn : msg_up;

			clr &=~ (uint8_t)e.pr.button;
			set |=  (uint8_t)e.pr.button;
			break;
		}

		case mouse_event::move:
		{
			evt_x = e.mv.x;
			evt_y = e.mv.y;
			evt_dlt_x += e.mv.x_delta;
			evt_dlt_y += e.mv.y_delta;
			evt_dlt_w += e.mv.wheel_delta;
			break;
		}
	}
}

bool handle_mouse(void* hwnd, uint32_t msg, uintptr_t wparam, uintptr_t lparam, const mouse_capture& capture, uintptr_t* inout_context, mouse_event* out)
{
	static const uintptr_t kIgnoreMouseMove = uintptr_t(1) << ((sizeof(uintptr_t)*8)-1);

	mouse_event e;
	memset(&e, 0, sizeof(e));
	e.type = mouse_event::press;
	e.pr.down = true;

	bool fill_deltas = false;

	switch (msg)
	{
		case WM_LBUTTONUP: e.pr.down = false; case WM_LBUTTONDOWN: e.pr.button = mouse_button::left; break;
		case WM_MBUTTONUP: e.pr.down = false; case WM_MBUTTONDOWN: e.pr.button = mouse_button::middle; break;
		case WM_RBUTTONUP: e.pr.down = false; case WM_RBUTTONDOWN: e.pr.button = mouse_button::right; break;
		case WM_XBUTTONUP: e.pr.down = false; case WM_XBUTTONDOWN: e.pr.button = GET_XBUTTON_WPARAM(wparam) == XBUTTON1 ? mouse_button::side1 : mouse_button::side2; break;

		case WM_LBUTTONDBLCLK: e.pr.button = mouse_button::dbl_left; break;
		case WM_MBUTTONDBLCLK: e.pr.button = mouse_button::dbl_middle; break;
		case WM_RBUTTONDBLCLK: e.pr.button = mouse_button::dbl_right; break;

		case WM_MOUSEMOVE:
			if (*inout_context & kIgnoreMouseMove)
			{
				*inout_context &=~ kIgnoreMouseMove;
				return false;
			}

			e.type = mouse_event::move;
			e.mv.x = GET_X_LPARAM(lparam);
			e.mv.y = GET_Y_LPARAM(lparam);
			e.mv.wheel_delta = 0;
			fill_deltas = true;
			break;

		case WM_MOUSEWHEEL:
		{
			e.type = mouse_event::move;
			POINT pt;
			oVB(GetCursorPos(&pt));
			oVB(ScreenToClient((HWND)hwnd, &pt));
			e.mv.x = static_cast<int16_t>(pt.x);
			e.mv.y = static_cast<int16_t>(pt.y);
			e.mv.wheel_delta = GET_WHEEL_DELTA_WPARAM(wparam);
			fill_deltas = true;
			break;
		}

		default: return false;
	}

	if (fill_deltas)
	{
		int16_t prev_x = GET_X_LPARAM(*inout_context);
		int16_t prev_y = GET_Y_LPARAM(*inout_context);

		e.mv.x_delta = e.mv.x - prev_x;
		e.mv.y_delta = e.mv.y - prev_y;

		if (capture == mouse_capture::relative)
		{
			// don't update prev point at all, then reposition the cursor to where it
			// was at time of capture. This will send another WM_MOUSEMOVE so ignore it.
			
			*inout_context |= kIgnoreMouseMove;
			POINT pt = { prev_x, prev_y };
			oVB(ClientToScreen((HWND)hwnd, &pt));
			oVB(SetCursorPos(pt.x, pt.y));
		}

		else
			*inout_context = MAKELONG(e.mv.x, e.mv.y);
	}

	if (out)
		*out = e;

	return true;
}

}
