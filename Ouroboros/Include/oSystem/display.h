// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Interface for working with monitors/displays

#pragma once
#include <oCore/macros.h>
#include <functional>

namespace ouro { namespace display {

class id
{
public:
	id() : Handle(-1) {}

	bool operator==(const id& _That) const { return Handle == _That.Handle; }
	bool operator!=(const id& _That) const { return !(*this == _That); }
	operator bool() const { return Handle != -1; }

private:
	int Handle;
};

struct mode_info
{
	mode_info()
		: width(oDEFAULT)
		, height(oDEFAULT)
		, depth(oDEFAULT)
		, refresh_rate(oDEFAULT)
	{}

	// resolution
	int width;
	int height;
		
	// usually 16- or 32-bit. oDEFAULT implies current settings
	int depth;

	// usually 60, 75, 85, 120, 240. oDEFAULT implies current settings
	int refresh_rate;
};

struct info
{
	info()
		: native_handle(nullptr)
		, x(oDEFAULT)
		, y(oDEFAULT)
		, workarea_x(oDEFAULT)
		, workarea_y(oDEFAULT)
		, workarea_width(oDEFAULT)
		, workarea_height(oDEFAULT)
		, is_primary(false)
		, is_power_on(false)
	{}
	
	mode_info mode;
	void* native_handle; // HMONITOR on Windows
	int x;
	int y;
	int workarea_x;
	int workarea_y;
	int workarea_width;
	int workarea_height;
	id display_id;
	bool is_primary;
	bool is_power_on;
};

id primary_id();

id get_id(void* _NativeHandle);

int count();

id find(int _ScreenX, int _ScreenY);

info get_info(id _ID);

void enumerate(const std::function<bool(const info& _Info)>& _Enumerator);

void set_mode(id _ID, const mode_info& _Mode);

// restores state after a call to set_mode
void reset_mode(id _ID);

void virtual_rect(int* _pX, int* _pY, int* _pWidth, int* _pHeight);

void taskbar_rect(int* _pX, int* _pY, int* _pWidth, int* _pHeight);

// turns all monitors on or sets them to a low-power state
void set_power_on(bool _On = true);

}}
