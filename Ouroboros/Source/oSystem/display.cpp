// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oSystem/display.h>
#include <oSystem/windows/win_error.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <Shellapi.h>

namespace ouro {
	namespace display {

id primary_id()
{
	id ID;
	enumerate([&](const info& _Info)->bool
	{
		if (_Info.is_primary)
		{
			ID = _Info.display_id;
			return false;
		}
		return true;
	});

	oCheck(ID != id(), std::errc::no_such_device, "");
	return ID;
}

id get_id(void* _NativeHandle)
{
	id ID;
	enumerate([&](const info& _Info)->bool
	{
		if (_Info.native_handle == _NativeHandle)
		{
			ID = _Info.display_id;
			return false;
		}
		return true;
	});

	oCheck(ID != id(), std::errc::no_such_device, "");
	return ID;
}

int count()
{
	return GetSystemMetrics(SM_CMONITORS);
}

id find(int _ScreenX, int _ScreenY)
{
	POINT p = { _ScreenX, _ScreenY };
	HMONITOR hMonitor = MonitorFromPoint(p, MONITOR_DEFAULTTONULL);
	oCheck(hMonitor, std::errc::no_such_device, "");

	id ID;
	enumerate([&](const info& _Info)->bool
	{
		if (hMonitor == (HMONITOR)_Info.native_handle)
		{
			ID = _Info.display_id;
			return false;
		}
		return true;
	});

	oCheck(ID != id(), std::errc::no_such_device, "");
	return ID;
}

static BOOL CALLBACK find_work_area(HMONITOR _hMonitor, HDC _hdcMonitor, LPRECT _lprcMonitor, LPARAM _dwData)
{
	MONITORINFOEX mi;
	mi.cbSize = sizeof(mi);
	oVB(GetMonitorInfo(_hMonitor, &mi));
	std::pair<const TCHAR*, info*>& ctx = *(std::pair<const TCHAR*, info*>*)_dwData;
	if (!_stricmp(mi.szDevice, ctx.first))
	{
		ctx.second->workarea_x = mi.rcWork.left;
		ctx.second->workarea_y = mi.rcWork.top;
		ctx.second->workarea_width = mi.rcWork.right - mi.rcWork.left;
		ctx.second->workarea_height = mi.rcWork.bottom - mi.rcWork.top;
		ctx.second->native_handle = (void*)_hMonitor;
		return FALSE;
	}
	return TRUE;
}

static info get_info(int _Index, const DISPLAY_DEVICE& _DisplayDevice)
{
	info di;
	*(int*)&di.display_id = _Index;
	di.is_primary = !!(_DisplayDevice.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE);
		
	DEVMODE dm;
	oVB(EnumDisplaySettings(_DisplayDevice.DeviceName, ENUM_CURRENT_SETTINGS, &dm));

	di.mode.width = dm.dmPelsWidth;
	di.mode.height = dm.dmPelsHeight;
	di.mode.depth = dm.dmBitsPerPel;
	di.mode.refresh_rate = dm.dmDisplayFrequency;

	di.x = dm.dmPosition.x;
	di.y = dm.dmPosition.y;

	di.native_handle = INVALID_HANDLE_VALUE;
	std::pair<const TCHAR*, info*> ctx;
	ctx.first = _DisplayDevice.DeviceName;
	ctx.second = &di;
	EnumDisplayMonitors(0, 0, find_work_area, (LPARAM)&ctx);
		
	// Why is a handle coming right out of EnumDisplayMonitors considered invalid?
	BOOL isOn = FALSE;
	GetDevicePowerState((HMONITOR)di.native_handle, &isOn);
	di.is_power_on = !!isOn;
	return di;
}

info get_info(id _ID)
{
	DISPLAY_DEVICE dev;
	dev.cb = sizeof(dev);
	oCheck(EnumDisplayDevices(0, *(int*)&_ID, &dev, 0), std::errc::no_such_device, "");
	return get_info(*(int*)&_ID, dev);
}

void enumerate(const std::function<bool(const info& _Info)>& _Enumerator)
{
	DISPLAY_DEVICE dev;
	dev.cb = sizeof(dev);
	int index = 0;

	while (EnumDisplayDevices(0, index, &dev, 0))
	{
		info di = get_info(index, dev);
		if (!_Enumerator(di))
			break;
		index++;
	}
}

static void set_mode(const char* _DeviceName, const mode_info& _Mode)
{
	DEVMODE dm;
	oCheck(EnumDisplaySettings(_DeviceName, ENUM_CURRENT_SETTINGS, &dm), std::errc::no_such_device, "");

	// ensure only what we want to set is set
	dm.dmFields = DM_PELSWIDTH|DM_PELSHEIGHT|DM_BITSPERPEL|DM_DISPLAYFREQUENCY;

	#define SET(x, y) if (_Mode.y != oDEFAULT) dm.x = _Mode.y
		SET(dmPelsWidth, width);
		SET(dmPelsHeight, height);
		SET(dmDisplayFrequency, refresh_rate);
		SET(dmBitsPerPel, depth);
	#undef SET

	LONG result = ChangeDisplaySettingsEx(_DeviceName, &dm, 0, CDS_TEST, 0);
	switch (result)
	{
		case DISP_CHANGE_BADDUALVIEW: 
		case DISP_CHANGE_BADFLAGS: 
		case DISP_CHANGE_BADMODE: 
		case DISP_CHANGE_BADPARAM: 
		case DISP_CHANGE_FAILED: 
		case DISP_CHANGE_NOTUPDATED: 
		case DISP_CHANGE_RESTART:
			oThrow(std::errc::invalid_argument, "invalid mode");
		default: 
		case DISP_CHANGE_SUCCESSFUL:
			ChangeDisplaySettingsEx(_DeviceName, &dm, 0, CDS_FULLSCREEN, 0);
			break;
	}
}

void set_mode(id _ID, const mode_info& _Mode)
{
	DISPLAY_DEVICE dev;
	dev.cb = sizeof(dev);
	oCheck(EnumDisplayDevices(0, *(int*)&_ID, &dev, 0), std::errc::no_such_device, "");
	set_mode(dev.DeviceName, _Mode);
}

static void reset_mode(const char* _DeviceName)
{
	ChangeDisplaySettingsEx(_DeviceName, 0, 0, CDS_FULLSCREEN, 0);
}

void reset_mode(id _ID)
{
	DISPLAY_DEVICE dev;
	dev.cb = sizeof(dev);
	oCheck(EnumDisplayDevices(0, *(int*)&_ID, &dev, 0), std::errc::no_such_device, "");
	reset_mode(dev.DeviceName);
}

void virtual_rect(int* _pX, int* _pY, int* _pWidth, int* _pHeight)
{
	*_pX = GetSystemMetrics(SM_XVIRTUALSCREEN);
	*_pY = GetSystemMetrics(SM_YVIRTUALSCREEN);
	*_pWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	*_pHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
}

void taskbar_rect(int* _pX, int* _pY, int* _pWidth, int* _pHeight)
{
	APPBARDATA abd;
	abd.cbSize = sizeof(abd);
	SHAppBarMessage(ABM_GETTASKBARPOS, &abd);
	*_pX = abd.rc.left;
	*_pY = abd.rc.top;
	*_pWidth = abd.rc.right - abd.rc.left;
	*_pHeight = abd.rc.bottom - abd.rc.top;
}

void set_power_on(bool _On)
{
	static const LPARAM OFF = 2;
	static const LPARAM LOWPOWER = 1;
	static const LPARAM ON = -1;
	PostMessage(HWND_BROADCAST, WM_SYSCOMMAND, SC_MONITORPOWER, _On ? ON : LOWPOWER);
}

	} // namespace display
}
