// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oCore/countof.h>
#include <oCore/finally.h>
#include <oGUI/Windows/oWinWindowing.h>
#include <oGUI/Windows/win_gdi_bitmap.h>
#include <oGUI/Windows/oWinRect.h>
#include <oGUI/Windows/oWinStatusBar.h>
#include <oString/atof.h>

#undef interface
#include <oSystem/windows/win_error.h>
#include <oSystem/windows/win_util.h>
#include <oSystem/windows/win_version.h>

#include <SetupAPI.h>
#include <Shlwapi.h>
#include <devguid.h>
#include <devpkey.h>
#include <windowsx.h>

using namespace ouro;
using namespace ouro::windows::gdi;

// Raw Input (WM_INPUT support) These are not defined anywhere I can find in
// Windows headers.

enum oUS_USAGE_PAGE_VALUES
{
	oUS_USAGE_PAGE_GENERIC_DESKTOP = 1,
	oUS_USAGE_PAGE_SIMULATION = 2,
	oUS_USAGE_PAGE_VIRTUAL_REALITY = 3,
	oUS_USAGE_PAGE_SPORT = 4,
	oUS_USAGE_PAGE_GAME = 5,
	oUS_USAGE_PAGE_GENERIC_DEVICE = 6,
	oUS_USAGE_PAGE_KEYBOARD = 7,
	oUS_USAGE_PAGE_LEDS = 8,
	oUS_USAGE_PAGE_BUTTON = 9,
};

enum oUS_USAGE
{
	oUS_USAGE_UNDEFINED = 0,
	oUS_USAGE_POINTER = 1,
	oUS_USAGE_MOUSE = 2,
	oUS_USAGE_RESERVED = 3,
	oUS_USAGE_JOYSTICK = 4,
	oUS_USAGE_GAMEPAD = 5,
	oUS_USAGE_KEYBOARD = 6,
	oUS_USAGE_KEYPAD = 7,
	oUS_USAGE_MULTIAXIS_CONTROLLER = 8,
	oUS_USAGE_TABLET_CONTROLS = 9,
};

#define oWIN_CHECK(_hWnd) do \
	{	oCheck(oWinExists(_hWnd), std::errc::invalid_argument, "Invalid HWND 0x%x specified", _hWnd); \
		oCheck(oWinIsWindowThread(_hWnd), std::errc::operation_not_permitted, "This function must be called on the window thread %d for HWND 0x%x", asdword(std::this_thread::get_id()), _hWnd); \
	} while (false)

static const char* kRegisteredWindowMessages[] = 
{
	"oWM_peripheral_status",
};
match_array(kRegisteredWindowMessages, oWM_REGISTERED_COUNT);

class oWinRegisteredMessages
{
public:
	oWinRegisteredMessages()
	{
		for (auto i = 0; i < countof(kRegisteredWindowMessages); i++)
			RegisteredMessages[i] = RegisterWindowMessage(kRegisteredWindowMessages[i]);
	}

	UINT Translate(UINT _uMsg)
	{
		if (oWinIsRegisteredMessage(_uMsg))
		{
			for (auto i = 0; i < countof(RegisteredMessages); i++)
				if (RegisteredMessages[i] == _uMsg)
					return (UINT)(oWM_REGISTERED_FIRST + i);
		}
		return _uMsg;
	}

private:
	UINT RegisteredMessages[oWM_REGISTERED_COUNT];
};

UINT oWinGetNativeRegisteredMessage(oWM _RegisteredMessage)
{
	return RegisterWindowMessage(kRegisteredWindowMessages[_RegisteredMessage - oWM_REGISTERED_FIRST]);
}

// copy from DEVPKEY.H
static const DEVPROPKEY oDEVPKEY_Device_DeviceDesc = { { 0xa45c254e, 0xdf1c, 0x4efd, { 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0 } }, 2 };
static const DEVPROPKEY oDEVPKEY_Device_HardwareIds = { { 0xa45c254e, 0xdf1c, 0x4efd, { 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0 } }, 3 };
static const DEVPROPKEY oDEVPKEY_Device_Class = { { 0xa45c254e, 0xdf1c, 0x4efd, { 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0 } }, 9 };
static const DEVPROPKEY oDEVPKEY_Device_PDOName = { { 0xa45c254e, 0xdf1c, 0x4efd, { 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0 } }, 16 };
static const DEVPROPKEY oDEVPKEY_Device_Parent = { { 0x4340a6c5, 0x93fa, 0x4706, { 0x97, 0x2c, 0x7b, 0x64, 0x80, 0x08, 0xa5, 0xa7 } }, 8 }; 
static const DEVPROPKEY oDEVPKEY_Device_InstanceId = { { 0x78c34fc8, 0x104a, 0x4aca, { 0x9e, 0xa4, 0x52, 0x4d, 0x52, 0x99, 0x6e, 0x57 } }, 256 };
static const DEVPROPKEY oDEVPKEY_Device_Siblings = { { 0x4340a6c5, 0x93fa, 0x4706, { 0x97, 0x2c, 0x7b, 0x64, 0x80, 0x08, 0xa5, 0xa7 } }, 10 };

#if 0
// if _EnumerateAll is false, then only existing/plugged in devices are searched.
// A single PnP device can support multiple device types, so this sets the 
// array of types the PnP device represents.

static char* oWinPnPEnumeratorToSetupClass(char* _StrDestination, size_t _SizeofStrDestination, const char* _PnPEnumerator)
{
	if (strlen(_PnPEnumerator) <= 4) return nullptr;
	size_t len = strlcpy(_StrDestination, _PnPEnumerator + 4, _SizeofStrDestination);
	if (len >= _SizeofStrDestination) return nullptr;
	char* hash = strrchr(_StrDestination, '#');
	if (!hash) return nullptr;
	*hash = '\0';
	char* end = _StrDestination + len;
	transform(_StrDestination, end, _StrDestination, [=](char c) { return (c == '#') ? '\\' : c; });
	transform(_StrDestination, end, _StrDestination, toupper<const char&>);
	char* first_slash = strchr(_StrDestination, '\\');
	if (!first_slash) return nullptr;
	*first_slash = '\0';
	return _StrDestination;
}

template<size_t size> char* oWinPnPEnumeratorToSetupClass(char (&_StrDestination)[size], const char* _PnPEnumerator) { return oWinPnPEnumeratorToSetupClass(_StrDestination, size, _PnPEnumerator); }
template<typename charT, size_t capacity> char* oWinPnPEnumeratorToSetupClass(fixed_string<charT, capacity>& _StrDestination, const char* _PnPEnumerator) { return oWinPnPEnumeratorToSetupClass(_StrDestination, _StrDestination.capacity(), _PnPEnumerator); }
#endif

input_type oWinGetDeviceTypeFromClass(const char* _Class)
{
	static const char* DeviceClass[] = 
	{
		"Unknown",
		"Keyboard",
		"Mouse",
		"Pad",
		"Skeleton",
		"Voice",
		"Touch", // not sure what this is called generically
		"Hotkey", // won't ever get these from HW config
		"Menu", //    "
		"Control", // "
	};
	match_array_e(DeviceClass, input_type);
	for (int i = 0; i < countof(DeviceClass); i++)
		if (!strcmp(DeviceClass[i], _Class))
			return input_type(i);
	return input_type::unknown;
}

static void oWinSetupGetString(mstring& _StrDestination, HDEVINFO _hDevInfo, SP_DEVINFO_DATA* _pSPDID, const DEVPROPKEY* _pPropKey)
{
	DEVPROPTYPE dpType;
	DWORD size = 0;
	SetupDiGetDevicePropertyW(_hDevInfo, _pSPDID, _pPropKey, &dpType, nullptr, 0, &size, 0);
	if (size)
	{
		mwstring buffer;
		oAssert(buffer.capacity() > (size / sizeof(mwstring::char_type)), "");
		oVB(SetupDiGetDevicePropertyW(_hDevInfo, _pSPDID, _pPropKey, &dpType, (BYTE*)buffer.c_str(), size, &size, 0) && dpType == DEVPROP_TYPE_STRING);
		_StrDestination = buffer;
	}
	else
		_StrDestination.clear();
}

// _NumStrings must be initialized to maximum capacity
static void oWinSetupGetStringList(mstring* _StrDestination, size_t& _NumStrings, HDEVINFO _hDevInfo, SP_DEVINFO_DATA* _pSPDID, const DEVPROPKEY* _pPropKey)
{
	DEVPROPTYPE dpType;
	DWORD size = 0;
	SetupDiGetDevicePropertyW(_hDevInfo, _pSPDID, _pPropKey, &dpType, nullptr, 0, &size, 0);
	if (size)
	{
		xlwstring buffer;
		oAssert(buffer.capacity() > size, "");
		oVB(SetupDiGetDevicePropertyW(_hDevInfo, _pSPDID, _pPropKey, &dpType, (BYTE*)buffer.c_str(), size, &size, 0) && dpType != DEVPROP_TYPE_STRING_LIST);

		const wchar_t* c = buffer.c_str();
		size_t i = 0;
		for (; i < _NumStrings; i++)
		{
			_StrDestination[i] = c;
			c += wcslen(c) + 1;

			if (!*c)
			{
				_NumStrings = i + 1;
				break;
			}
		}

		if (i > _NumStrings)
		{
			while (*c)
			{
				c += wcslen(c) + 1;
				i++;
			}

			_NumStrings = i + 1;
			oThrow(std::errc::no_buffer_space, "not enough strings");
		}
	}
	else
		_NumStrings = 0;
}

struct oWINDOWS_HID_DESC
{
	oWINDOWS_HID_DESC()
		: Type(input_type::unknown)
		, DevInst(0)
	{}

	input_type Type;
	DWORD DevInst;
	mstring ParentDeviceInstancePath;
	mstring DeviceInstancePath;
};

static void oWinEnumInputDevices(bool _EnumerateAll, const char* _Enumerator, const std::function<void(const oWINDOWS_HID_DESC& _HIDDesc)>& _Visitor)
{
	if (!_Visitor)
		oThrow(std::errc::invalid_argument, "must specify a visitor");

	HDEVINFO hDevInfo = INVALID_HANDLE_VALUE;
	oFinally 
	{ 
		if (hDevInfo != INVALID_HANDLE_VALUE) 
			SetupDiDestroyDeviceInfoList(hDevInfo);
	};

	DWORD dwFlag = DIGCF_ALLCLASSES;
	if (!_EnumerateAll)
		dwFlag |= DIGCF_PRESENT;

	hDevInfo = SetupDiGetClassDevsA(nullptr, _Enumerator, nullptr, dwFlag);
	oVB(INVALID_HANDLE_VALUE != hDevInfo);

	SP_DEVINFO_DATA SPDID;
	memset(&SPDID, 0, sizeof(SPDID));
	SPDID.cbSize = sizeof(SPDID);
	int DeviceIndex = 0;
	bool KnownTypeFound = false;

	oWINDOWS_HID_DESC HIDDesc;

	mstring KinectCameraSibling;
	mstring KinectCameraParent;

	while (SetupDiEnumDeviceInfo(hDevInfo, DeviceIndex, &SPDID))
	{
		DeviceIndex++;

		mstring ClassValue;
		oWinSetupGetString(ClassValue, hDevInfo, &SPDID, &oDEVPKEY_Device_Class);
		oWinSetupGetString(HIDDesc.ParentDeviceInstancePath, hDevInfo, &SPDID, &oDEVPKEY_Device_Parent);

		// Fall back on generic desc string for Kinect, which apparently is in a 
		// class by itself at the moment.
		if (!strcmp("Microsoft Kinect", ClassValue))
		{
			oWinSetupGetString(ClassValue, hDevInfo, &SPDID, &oDEVPKEY_Device_DeviceDesc);

			if (strstr(ClassValue, "Camera"))
			{
				ClassValue = "Skeleton";
				HIDDesc.DeviceInstancePath = HIDDesc.ParentDeviceInstancePath;

				mstring Strings[2];
				size_t Capacity = countof(Strings);
				oWinSetupGetStringList(Strings, Capacity, hDevInfo, &SPDID, &oDEVPKEY_Device_Siblings);
				if (Capacity != 1)
					oThrow(std::errc::invalid_argument, "expected one result");

				KinectCameraSibling = Strings[0];
				KinectCameraParent = HIDDesc.ParentDeviceInstancePath;
			}

			else if (strstr(ClassValue, "Audio"))
			{
				ClassValue = "Voice";
				if (HIDDesc.ParentDeviceInstancePath == KinectCameraSibling)
					HIDDesc.DeviceInstancePath = KinectCameraParent;

				else
					HIDDesc.DeviceInstancePath = HIDDesc.ParentDeviceInstancePath;
			}
			else
				ClassValue = "Unknown";
		}

		else
			oWinSetupGetString(HIDDesc.DeviceInstancePath, hDevInfo, &SPDID, &oDEVPKEY_Device_InstanceId);

		// There are often meta-classes to be ignored, so don't flag unknown unless
		// every other type is not present.
		HIDDesc.Type = oWinGetDeviceTypeFromClass(ClassValue);
		HIDDesc.DevInst = SPDID.DevInst;

		_Visitor(HIDDesc);
	}
}

// Translate a PnP enumerator name (DEV_BROADCAST_DEVICEINTERFACE_A::dbcc_name)
// to one or more unique names based on the peripheral_status::TYPEs represented
// by the device. Devices now are often hybrid, such as a KB/mouse combo USB or 
// a voice/skeleton Kinect device. This uses SetupAPI to do the discovery.
static void oWinEnumInputDevices(bool _EnumerateAll, const std::function<void(const oWINDOWS_HID_DESC& _HIDDesc)>& _Visitor)
{
	oWinEnumInputDevices(_EnumerateAll, "HID", _Visitor);
	oWinEnumInputDevices(_EnumerateAll, "USB", _Visitor);
}

struct oWIN_DEVICE_CHANGE_CONTEXT
{
	oWIN_DEVICE_CHANGE_CONTEXT()
	{
		LastChangeTimestamp.fill(0);
	}

	std::vector<RAWINPUTDEVICELIST> RawInputs;
	std::vector<mstring> RawInputInstanceNames;
	std::array<unsigned int, (size_t)peripheral_status::count> LastChangeTimestamp;
};

// Register the specified window to receive WM_input_type_CHANGE events. This
// also uses oWM_input_type_CHANGE to emulate the on-creation events fired for
// non-KB/mouse devices like the Kinect.
static void oWinRegisterDeviceChangeEvents(HWND _hWnd)
{
	RAWINPUTDEVICE RID[] =
	{
		{ oUS_USAGE_PAGE_GENERIC_DESKTOP, oUS_USAGE_KEYBOARD, RIDEV_DEVNOTIFY, _hWnd },
		{ oUS_USAGE_PAGE_GENERIC_DESKTOP, oUS_USAGE_MOUSE, RIDEV_DEVNOTIFY, _hWnd },
	};
	oVB(RegisterRawInputDevices(RID, countof(RID), sizeof(RAWINPUTDEVICE)));

	// Windows sends KB and mouse device notifications automatically, so make up the
	// difference here.
	oWinEnumInputDevices(false, [&](const oWINDOWS_HID_DESC& _HIDDesc)
	{
		switch(_HIDDesc.Type)
		{
			case input_type::keypress: case input_type::mouse: case input_type::unknown: break;
			default: SendMessage(_hWnd, oWM_INPUT_DEVICE_CHANGE, MAKEWPARAM(_HIDDesc.Type, peripheral_status::full_power), (LPARAM)_HIDDesc.DeviceInstancePath.c_str());
		}
	});
}

// There's a lot of code to parse a device change message, so encapsulate it 
// here. Basically when devices are removed, there's no name passed to the 
// event of what was removed, so basically a record of all devices needs to be 
// kept and then scanned for differences. That's what the context is. Useage 
// should be to create a context for a window that will support 
// WM_input_type_CHANGE and then call oWinTranslateDeviceChange in that event
// to translate it into the simpler oWM_input_type_CHANGE message. Remember to 
// call oWinRegisterDeviceChangeEvents, else the WM_input_type_CHANGE event 
// will not be fired and this hook will not execute.
oDECLARE_HANDLE(HDEVICECHANGE)
static HDEVICECHANGE oWinDeviceChangeCreate()
{
	return (HDEVICECHANGE)new oWIN_DEVICE_CHANGE_CONTEXT();
}

static void oWinDeviceChangeDestroy(HDEVICECHANGE _hDeviceChance)
{
	oWIN_DEVICE_CHANGE_CONTEXT* ctx = (oWIN_DEVICE_CHANGE_CONTEXT*)_hDeviceChance;
	delete ctx;
}

static input_type oWinGetTypeFromRIM(DWORD _dwRIMType)
{
	switch (_dwRIMType)
	{
		case RIM_TYPEKEYBOARD: return input_type::keypress;
		case RIM_TYPEMOUSE: return input_type::mouse;
		default: break;
	}
	return input_type::unknown;
}

static bool oWinTranslateDeviceChange(HWND _hWnd, WPARAM _wParam, LPARAM _lParam, HDEVICECHANGE _hDeviceChance)
{
	oWIN_DEVICE_CHANGE_CONTEXT* ctx = (oWIN_DEVICE_CHANGE_CONTEXT*)_hDeviceChance;
	input_type Type = input_type::unknown;
	peripheral_status Status = _wParam == GIDC_ARRIVAL ? peripheral_status::full_power : peripheral_status::not_ready;
	mstring InstanceName;

	switch (_wParam)
	{
		// We have full and direct information.
		case GIDC_ARRIVAL:
		{
			RID_DEVICE_INFO RIDDI;
			RIDDI.cbSize = sizeof(RIDDI);
			UINT Size = sizeof(RIDDI);
			UINT NameCapacity = uint32_t(InstanceName.capacity());
			oVB(GetRawInputDeviceInfoA((HANDLE)_lParam, RIDI_DEVICEINFO, &RIDDI, &Size));
			oVB(GetRawInputDeviceInfoA((HANDLE)_lParam, RIDI_DEVICENAME, InstanceName.c_str(), &NameCapacity));

			// Refresh record keeping in case these are new devices
			UINT RawCapacity = 0;
			GetRawInputDeviceList(nullptr, &RawCapacity, sizeof(RAWINPUTDEVICELIST));
			ctx->RawInputs.resize(RawCapacity);
			ctx->RawInputInstanceNames.resize(RawCapacity);
			UINT RawCount = GetRawInputDeviceList(ctx->RawInputs.data(), &RawCapacity, sizeof(RAWINPUTDEVICELIST));
			for (UINT i = 0; i < RawCount; i++)
			{
				UINT Capacity = uint32_t(ctx->RawInputInstanceNames[i].capacity());
				oVB(GetRawInputDeviceInfoA(ctx->RawInputs[i].hDevice, RIDI_DEVICENAME, ctx->RawInputInstanceNames[i].c_str(), &Capacity));
			}

			oAssert(RawCount == RawCapacity, "size mismatch");
			Type = oWinGetTypeFromRIM(RIDDI.dwType);
			break;
		}

		// Handle is correct, but device info won't be, not even type, so
		// look it up in app bookkeeping.
		case GIDC_REMOVAL:
		{
			for (size_t i = 0; i < ctx->RawInputs.size(); i++)
			{
				if (ctx->RawInputs[i].hDevice == (HANDLE)_lParam)
				{
					Type = oWinGetTypeFromRIM(ctx->RawInputs[i].dwType);
					InstanceName = ctx->RawInputInstanceNames[i];
					break;
				}
			}

			break;
		}
	}

	const unsigned int Timestamp = (unsigned int)GetMessageTime();
	if (ctx->LastChangeTimestamp[(int)Type] != Timestamp)
	{
		ctx->LastChangeTimestamp[(int)Type] = Timestamp;
		SendMessage(_hWnd, oWM_INPUT_DEVICE_CHANGE, MAKEWPARAM(Type, Status), (LPARAM)InstanceName.c_str());
	}

	return true;
}

static BOOL CALLBACK oWinEnumWindowsProc(HWND _hWnd, LPARAM _lParam)
{
	return (*(std::function<bool(HWND _hWnd)>*)_lParam)(_hWnd) ? TRUE : FALSE;
}

void oWinEnumWindows(const std::function<bool(HWND _hWnd)>& _Enumerator)
{
	EnumWindows(oWinEnumWindowsProc, (LPARAM)&_Enumerator);
}

void oWinGetProcessTopWindowAndThread(process::id _ProcessID
	, HWND* _pHWND
	, std::thread::id* _pThreadID
	, const char* _pWindowName)
{
	HWND Hwnd = 0;
	unsigned int ThreadID = 0;

	unsigned int PID = *(unsigned int*)&_ProcessID;

	oWinEnumWindows(
		[&](HWND _Hwnd)->bool
	{
		unsigned int HWNDProcessID;
		auto NewThreadID = GetWindowThreadProcessId(_Hwnd, (DWORD*)&HWNDProcessID);

		if (HWNDProcessID != PID)
			return true;

		// Look for windows that might get injected into our process that we would never want to target
		const char* WindowsToSkip[] = {"Default IME", "MSCTFIME UI"};
		mstring WindowText;
		// For some reason in release builds Default IME can take a while to respond to this. so wait longer unless the app appears to be hung.
		if (SendMessageTimeout(_Hwnd, WM_GETTEXT, mstring::Capacity - 1, (LPARAM)WindowText.c_str(), SMTO_ABORTIFHUNG, 2000, nullptr) == 0)
			WindowText = "";

		if (!WindowText.empty())
		{
			for (auto& w : WindowsToSkip)
				if( 0 == strcmp(WindowText.c_str(), w) )
					return true;
		}

		// If the caller specified a window name make certain this is the right window or else keep searching
		if(_pWindowName && 0 != strcmp(_pWindowName, WindowText))
			return true;

		Hwnd = _Hwnd;
		ThreadID = NewThreadID;
		return false;
	});

	oCheck(Hwnd, std::errc::no_such_process, "");

	*_pHWND = Hwnd;
	*_pThreadID = astid(ThreadID);
}

// @tony: Confirmation of hWnd being on the specified thread is disabled for 
// now... I think it might be trying to access the HWND before it's fully
// constructed. First get the massive integration done, then come back to this.

#define oWINVP(_hWnd) \
	if (!oWinExists(_hWnd)) oThrow(std::errc::invalid_argument, "Invalid HWND %p specified", _hWnd); \
	if (!oWinIsWindowThread(_hWnd)) oThrow(std::errc::invalid_argument, "This function must be called on the window thread %d for %p", asdword(std::this_thread::get_id()), _hWnd)

#define oTHROW_BAD_TYPE(_hControl, _Type) oThrow(std::errc::invalid_argument, "The specified %s %p (%d) is not valid for this operation", as_string(_Type), _hControl, GetDlgCtrlID(_hControl))
#define oTHROW_BAD_INDEX(_hControl, _Type, _SubItemIndex) oThrow(std::errc::invalid_argument, "_SubItemIndex %d was not found in %s %p (%d)", _SubItemIndex, as_string(_Type), _hControl, GetDlgCtrlID(_hControl))

inline HWND oWinControlGetBuddy(HWND _hControl) { return (HWND)SendMessage(_hControl, UDM_GETBUDDY, 0, 0); }

static DWORD oWinGetStyle(window_style _Style, bool _HasParent)
{
	switch (_Style)
	{
		case window_style::borderless: return _HasParent ? WS_CHILD : WS_POPUP;
		case window_style::dialog: return WS_CAPTION;
		case window_style::fixed: 
		case window_style::fixed_with_menu:
		case window_style::fixed_with_statusbar:
		case window_style::fixed_with_menu_and_statusbar: return WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX;
		default: break;
	}
	return WS_OVERLAPPEDWINDOW;
}

// Returns the height of the status bar if the specified window has one. If no
// status bar is present or the status bar is hidden, this returns 0.
static int oWinGetStatusBarHeight(HWND _hWnd)
{
	int h = 0;
	if (oWinStatusBarShown(_hWnd))
	{
		RECT rStatusBar;
		HWND hStatusBar = oWinGetStatusBar(_hWnd);
		GetClientRect(hStatusBar, &rStatusBar);
		h = oWinRectH(rStatusBar);
	}
	return h;
}

struct oWndExtra
{
	HMENU hMenu; // ::GetMenu() returns the /current/ state. Because we want to show/hide the menu, we need to keep the real one around
	HWND hStatusBar; // same as above
	oWinRegisteredMessages* pRegisteredMessages; // Needs to keep the list of register messages so they can be translated
	HDEVICECHANGE hDeviceChange; // for device change monitoring, a significant context is needed, this is it (all ouro-defined for kinect/skeleton support)
	LONG_PTR RestoredPosition; // MAKELPARAM(x,y)
	LONG_PTR RestoredSize; // MAKELPARAM(w,h)
	LONG_PTR PreviousState; // window_state
	LONG_PTR PreviousStyle; // window_style
	LONG_PTR ExtraFlags;
	intptr_t TempCallCounter;
	uintptr_t LastShowTimestamp;
};

// This is set when the handler should ignore the event because its about to be 
// followed up with the same event with more meaningful data.
#define oWNDEXTRA_FLAGS_TEMP_CHANGE (0x1)

#define oGWLP_MENU (offsetof(oWndExtra, hMenu))
#define oGWLP_STATUSBAR (offsetof(oWndExtra, hStatusBar))
#define oGWLP_REGISTERED_MESSAGES (offsetof(oWndExtra, pRegisteredMessages))
#define oGWLP_DEVICE_CHANGE (offsetof(oWndExtra, hDeviceChange))
#define oGWLP_RESTORED_POSITION (offsetof(oWndExtra, RestoredPosition)) // use GET_X_LPARAM and GET_Y_LPARAM to decode
#define oGWLP_RESTORED_SIZE (offsetof(oWndExtra, RestoredSize)) // use GET_X_LPARAM and GET_Y_LPARAM to decode
#define oGWLP_PREVIOUS_STATE (offsetof(oWndExtra, PreviousState)) // window_state
#define oGWLP_PREVIOUS_STYLE (offsetof(oWndExtra, PreviousStyle)) // window_style
#define oGWLP_EXTRA_FLAGS (offsetof(oWndExtra, ExtraFlags))
#define oGWLP_TEMP_CALL_COUNTER (offsetof(oWndExtra, TempCallCounter))
#define oGWLP_LAST_SHOW_TIMESTAMP (offsetof(oWndExtra, LastShowTimestamp))

#define oEF_RENDER_TARGET (1<<0)
#define oEF_FULLSCREEN_EXCLUSIVE (1<<1)
#define oEF_FULLSCREEN_COOPERATIVE (1<<2)
#define oEF_NO_SAVE_RESTORED_POSITION_SIZE (1<<3)
#define oEF_ALT_F4_ENABLED (1<<4)
#define oEF_THREAD_OWNER (1<<5)

HWND oWinCreate(HWND _hParent
	, const char* _Title
	, window_style _Style
	, const int2& _ClientPosition
	, const int2& _ClientSize
	, WNDPROC _Wndproc
	, void* _pInit
	, void* _pThis)
{
	sstring ClassName;
	WNDCLASSEXA wc;
	ZeroMemory(&wc, sizeof(WNDCLASSEX));
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.hInstance = GetModuleHandle(0);
	wc.lpfnWndProc = _Wndproc ? _Wndproc : DefWindowProc;
	// for "native" support for menus and status bars and somewhere to store 
	// restored pos/size
	wc.cbWndExtra = sizeof(oWndExtra);
	wc.lpszClassName = (LPCSTR)oWinMakeClassName(ClassName, _Wndproc);
	wc.style = CS_BYTEALIGNCLIENT|CS_HREDRAW|CS_VREDRAW|CS_OWNDC|CS_DBLCLKS;
	wc.hCursor = NULL;//LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
	oVB(0 != RegisterClassEx(&wc) || GetLastError() == ERROR_CLASS_ALREADY_EXISTS);

	// Resolve initial position and size
	int2 NewPosition = _ClientPosition;
	int2 NewSize = _ClientSize;
	if (NewPosition.x == oDEFAULT || NewPosition.y == oDEFAULT || NewSize.x == oDEFAULT || NewSize.y == oDEFAULT)
	{
		display::info di = display::get_info(display::primary_id());
		const RECT rPrimaryWorkarea = oWinRectWH(int2(di.workarea_x, di.workarea_y), int2(di.workarea_width, di.workarea_height));
		const int2 DefaultSize = oWinRectSize(rPrimaryWorkarea) / 4; // 25% of parent window by default
		NewSize = resolve_default(NewSize, DefaultSize);
		const RECT rCenteredClient = oWinRect(resolve_rect(oRect(rPrimaryWorkarea), int2(0, 0), NewSize, alignment::middle_center, false));
		const int2 DefaultPosition = oWinRectPosition(rCenteredClient);
		NewPosition = resolve_default(NewPosition, DefaultPosition);
	}

	const DWORD dwInitialStyleEx = WS_EX_ACCEPTFILES|WS_EX_APPWINDOW;
	const DWORD dwInitialStyle = oWinGetStyle(_Style, false);

	RECT rWindow = oWinRectWH(NewPosition, NewSize);
	oVB(AdjustWindowRectEx(&rWindow, dwInitialStyle, false, dwInitialStyleEx));
	oWIN_CREATESTRUCT wcs;
	wcs.Shape.state = window_state::hidden;
	wcs.Shape.style = _Style;
	wcs.Shape.client_position = NewPosition;
	wcs.Shape.client_size = NewSize;
	wcs.pThis = _pThis;
	wcs.pInit = _pInit;

	// catch any exception thrown by a user's WM_CREATE since oGUI/oWindow passes
	// const events around and thus doesn't know what gets returned.
	HWND hWnd = CreateWindowEx(dwInitialStyleEx, ClassName, oSAFESTRN(_Title), dwInitialStyle
			, rWindow.left, rWindow.top, oWinRectW(rWindow), oWinRectH(rWindow)
			, _hParent, nullptr, nullptr, &wcs);

	if (!hWnd)
	{
		if (GetLastError() == S_OK)
			oThrow(std::errc::protocol_error, "CreateWindowEx returned a null HWND (failure condition) for '%s' but GetLastError is S_OK. This implies that user handling of a WM_CREATE message failed, so start looking there.", oSAFESTRN(_Title));
		else
			throw windows::error();
	}

	// only top-level windows should be allowed to quit the message loop
	SetWindowLongPtr(hWnd, oGWLP_EXTRA_FLAGS, GetWindowLongPtr(hWnd, oGWLP_EXTRA_FLAGS) | oEF_THREAD_OWNER);

	oWinRegisterDeviceChangeEvents(hWnd);

	return hWnd;
}

void oWinDestroy(HWND _hWnd)
{
	if (oWinIsWindowThread(_hWnd))
		DestroyWindow(_hWnd);
	else
		PostMessage(_hWnd, oWM_DESTROY, 0, 0);
}

char* oWinMakeClassName(char* _StrDestination, size_t _SizeofStrDestination, WNDPROC _Wndproc)
{
	int written = ouro::snprintf(_StrDestination, _SizeofStrDestination, "Ouroboros.Window.WndProc.%x", _Wndproc);
	return (size_t)written < _SizeofStrDestination ? _StrDestination : nullptr;
}

bool oWinIsClass(HWND _hWnd, WNDPROC _Wndproc)
{
	sstring ClassName, ExpectedClassName;
	if (!oWinMakeClassName(ExpectedClassName, _Wndproc)) return false;
	if (!GetClassName(_hWnd, ClassName.c_str(), int(ClassName.capacity()))) return false;
	return !strcmp(ClassName, ExpectedClassName);
}

bool oWinSetTempChange(HWND _hWnd, bool _IsTemp)
{
	oWIN_CHECK(_hWnd);
	intptr_t Counter = (intptr_t)GetWindowLongPtr(_hWnd, oGWLP_TEMP_CALL_COUNTER);
	Counter = __max(0, Counter + (_IsTemp ? 1 : -1));
	SetWindowLongPtr(_hWnd, oGWLP_TEMP_CALL_COUNTER, (LONG_PTR)Counter);
	return true;
}

bool oWinIsTempChange(HWND _hWnd)
{
	oWIN_CHECK(_hWnd);
	intptr_t Counter = (intptr_t)GetWindowLongPtr(_hWnd, oGWLP_TEMP_CALL_COUNTER);
	return Counter != 0;
}

static void oWinSaveRestoredPosSize(HWND _hWnd)
{
	if (!IsIconic(_hWnd) && !IsZoomed(_hWnd) && !oWinIsTempChange(_hWnd))
	{
		LONG_PTR extra = GetWindowLongPtr(_hWnd, oGWLP_EXTRA_FLAGS);
		if (0 == (extra & (oEF_FULLSCREEN_COOPERATIVE|oEF_FULLSCREEN_COOPERATIVE|oEF_NO_SAVE_RESTORED_POSITION_SIZE)))
		{
			window_shape s = oWinGetShape(_hWnd);
			SetWindowLongPtr(_hWnd, oGWLP_RESTORED_POSITION, (LONG_PTR)MAKELPARAM(s.client_position.x, s.client_position.y));
			SetWindowLongPtr(_hWnd, oGWLP_RESTORED_SIZE, (LONG_PTR)MAKELPARAM(s.client_size.x, s.client_size.y));
		}
	}
}

LRESULT CALLBACK oWinWindowProc(HWND _hWnd, UINT _uMsg, WPARAM _wParam, LPARAM _lParam)
{
	if (_hWnd)
	{
		switch (_uMsg)
		{
			case WM_ERASEBKGND:
				// if a render target, don't erase with GDI and cause flickering
				if (oWinIsRenderTarget(_hWnd))
					return 1;
				break;

			case WM_CREATE:
			{
				// 'this' pointer was passed during the call to CreateWindow, so put 
				// that in userdata.
				CREATESTRUCTA* cs = (CREATESTRUCTA*)_lParam;
				oWIN_CREATESTRUCT* wcs = (oWIN_CREATESTRUCT*)cs->lpCreateParams;

				// Set up internal/custom data.
				SetWindowLongPtr(_hWnd, oGWLP_REGISTERED_MESSAGES, (LONG_PTR)new oWinRegisteredMessages);
				SetWindowLongPtr(_hWnd, oGWLP_DEVICE_CHANGE, (LONG_PTR)oWinDeviceChangeCreate());
				SetWindowLongPtr(_hWnd, oGWLP_MENU, (LONG_PTR)CreateMenu());
				SetWindowLongPtr(_hWnd, oGWLP_STATUSBAR, (LONG_PTR)oWinStatusBarCreate(_hWnd, (HMENU)0x00005747));
				oWinShowStatusBar(_hWnd, has_statusbar(wcs->Shape.style));

				if (has_statusbar(wcs->Shape.style))
					oVB(SetWindowPos(_hWnd, 0, cs->x, cs->y, cs->cx, cs->cy + oWinGetStatusBarHeight(_hWnd), SWP_NOZORDER|SWP_FRAMECHANGED|SWP_NOMOVE));

				oWinSaveRestoredPosSize(_hWnd);
				oWinSetTempChange(_hWnd, wcs->Shape.state != window_state::maximized);
				oWinShowMenu(_hWnd, has_menu(wcs->Shape.style));
				oWinSetTempChange(_hWnd, false);

				SetWindowLongPtr(_hWnd, GWLP_USERDATA, (LONG_PTR)wcs->pThis);
				break;
			}

			case WM_INITDIALOG:
				// dialogs don't use CREATESTRUCT, so assume it's directly the context
				SetWindowLongPtr(_hWnd, GWLP_USERDATA, (LONG_PTR)_lParam);
				break;

			case WM_DESTROY:
			{
				// If GetMenu returns the menu, then Windows will handle its destruction
				if (!::GetMenu(_hWnd))
					DestroyMenu((HMENU)SetWindowLongPtr(_hWnd, oGWLP_MENU, (LONG_PTR)nullptr));

				// As a child window, the status bar is implicitly destroyed.

				// minimizes "randomly set focus to some window other than parent"
				// @tony: Revisit this: this causes a child window to be reset in 
				// position and looks ugly. It may be true that position needs to be 
				// set here as well.
				//oWinSetOwner(_hWnd, nullptr);

				oWinDeviceChangeDestroy((HDEVICECHANGE)GetWindowLongPtr(_hWnd, oGWLP_DEVICE_CHANGE));
				SetWindowLongPtr(_hWnd, oGWLP_DEVICE_CHANGE, (LONG_PTR)nullptr);

				oWinRegisteredMessages* pRegisteredMessages = (oWinRegisteredMessages*)GetWindowLongPtr(_hWnd, oGWLP_REGISTERED_MESSAGES);
				delete pRegisteredMessages;
				SetWindowLongPtr(_hWnd, oGWLP_REGISTERED_MESSAGES, (LONG_PTR)nullptr);

				if (oWinIsThreadOwner(_hWnd))
					PostQuitMessage(0);
				break;
			}

			case WM_SYSKEYDOWN:
			{
				if (_wParam == VK_F4 && !oWinAltF4IsEnabled(_hWnd))
					return true;
				break;
			}

			case WM_SYSCOMMAND:
			{
				// Take over control of syscommands to resize since oWinSetShape does
				// extra work DefWindowProc is not aware of.

				window_shape s;
				switch (_wParam)
				{
					case SC_MAXIMIZE:
						s.state = window_state::maximized;
						oWinSetShape(_hWnd, s);
						return 0;
					case SC_MINIMIZE:
						s.state = window_state::minimized;
						oWinSetShape(_hWnd, s);
						return 0;
					case SC_RESTORE:
					{
						window_shape Old = oWinGetShape(_hWnd);
						if (Old.state == window_state::minimized)
						{
							s.state = (window_state)GetWindowLongPtr(_hWnd, oGWLP_PREVIOUS_STATE);
							if (s.state == window_state::hidden)
								s.state = window_state::restored;
						}
						else 
							s.state = window_state::restored;

						oWinSetShape(_hWnd, s);
						return 0;
					}
					default:
						break;
				}

				break;
			}

			case WM_WINDOWPOSCHANGED:
			{
				if (IsWindowVisible(_hWnd))
					SetWindowLongPtr(_hWnd, oGWLP_LAST_SHOW_TIMESTAMP, (ULONG_PTR)timer::now_msi());
				else
					SetWindowLongPtr(_hWnd, oGWLP_LAST_SHOW_TIMESTAMP, (ULONG_PTR)0);
				break;
			}

			case WM_MOVE:
				oWinSaveRestoredPosSize(_hWnd);
				break;

			case WM_SIZE:
			{
				HWND hStatusBar = (HWND)GetWindowLongPtr(_hWnd, oGWLP_STATUSBAR);
				SendMessage(hStatusBar, WM_SIZE, 0, 0);
				oWinSaveRestoredPosSize(_hWnd);
				break;
			}

			case oWM_INPUT_DEVICE_CHANGE:
				oVB(oWinTranslateDeviceChange(_hWnd, _wParam, _lParam, (HDEVICECHANGE)GetWindowLongPtr(_hWnd, oGWLP_DEVICE_CHANGE)));
				break;

			default:
				break;
		}
	}
	return -1;
}

void* oWinGetThis(HWND _hWnd)
{
	oWIN_CHECK(_hWnd);
	return (void*)GetWindowLongPtr(_hWnd, GWLP_USERDATA);
}

UINT oWinTranslateMessage(HWND _hWnd, UINT _uMsg)
{
	oWinRegisteredMessages* p = (oWinRegisteredMessages*)GetWindowLongPtr(_hWnd, oGWLP_REGISTERED_MESSAGES);
	return p ? p->Translate(_uMsg) : _uMsg;
}

std::thread::id oWinGetWindowThread(HWND _hWnd)
{
	return astid(GetWindowThreadProcessId(_hWnd, nullptr));
}

bool oWinIsOpaque(HWND _hWnd)
{
	// @tony: I can't find API to ask about the opacity of an HWND, so just wait 
	// for a while.
	oWIN_CHECK(_hWnd);
	static const uintptr_t kFadeInTime = 200;
	uintptr_t LastShowTimestamp = (uintptr_t)GetWindowLongPtr(_hWnd, oGWLP_LAST_SHOW_TIMESTAMP);
	if (LastShowTimestamp == 0)
		return false;
	uintptr_t Now = timer::now_msi();
	return (LastShowTimestamp + kFadeInTime) < Now;
}

void oWinRegisterTouchEvents(HWND _hWnd, bool _Registered)
{
	#ifdef oWINDOWS_HAS_REGISTERTOUCHWINDOW
		windows::version v = windows::get_version();
		if (v >= windows::version::win7)
		{
			if (_Registered)
				oVB(RegisterTouchWindow(_hWnd, 0));
			else
				oVB(UnregisterTouchWindow(_hWnd));
		}
		else
	#endif
	oThrow(std::errc::not_supported, "Windows 7 is the minimum required version for touch support");
}

// A wrapper for MS's GetWindow() that changes the rules of traversal. The rules 
// for the "next" window are: _hWnd's first child, if not then the next 
// sibling, and if at end of sibling list then the first child of _hWnd's parent 
// (what should be the first sibling in _hWnd's sibling list. If the next window 
// has a "buddy" window (UPDOWN control), then the buddy becomes the next, since 
// it controls the events of the other buddy anyway.
static HWND oWinGetNextWindow(HWND _hWnd)
{
	oWINVP(_hWnd);
	HWND hNext = GetWindow(_hWnd, GW_CHILD);
	if (!hNext)
		hNext = GetWindow(_hWnd, GW_HWNDNEXT);
	if (!hNext) // try wrap around
	{
		HWND hParent;
		hParent = GetParent(_hWnd);
		if (!GetParent(hParent)) // if is top-level window bounce back down to first child
			hNext = GetWindow(hParent, GW_CHILD);
		else
			hNext = GetWindow(hParent, GW_HWNDNEXT);

		// handle case where there is exactly one control
		if (_hWnd == hNext)
			return nullptr;
	}

	if (hNext)
	{
		HWND hBuddy = oWinControlGetBuddy(hNext);
		if (hBuddy)
			hNext = hBuddy;
	}
	return hNext;
}

// Returns the last HWND in the specified _hWnd's child list. If not a parent, 
// then this returns nullptr.
static HWND oWinGetLastChild(HWND _hWnd)
{
	oWINVP(_hWnd);
	HWND hChild = GetWindow(_hWnd, GW_CHILD);
	if (hChild)
	{
		HWND hLastChild = GetWindow(hChild, GW_HWNDLAST);
		if (hLastChild) hChild = hLastChild;
	}
	return hChild;
}

// A wrapper for MS's GetWindow() that changes the rules of traversal. The rules
// for the "prev" window are: _hWnd's previous sibling, if at start of sibling 
// list, then get the last child of _hWnd, else get the last child of _hWnd's
// parent.
static HWND oWinGetPrevWindow(HWND _hWnd)
{
	oWINVP(_hWnd);
	HWND hPrev = GetWindow(_hWnd, GW_HWNDPREV);
	if (!hPrev)
	{
		hPrev = GetParent(_hWnd);
		if (hPrev && !GetParent(hPrev)) // don't pop up to top-level window, stay in controls
			hPrev = nullptr;
	}

	else
	{
		HWND hLastChild = oWinGetLastChild(hPrev);
		if (hLastChild) hPrev = hLastChild;
	}

	if (!hPrev) // try wrap around
	{
		hPrev = GetWindow(_hWnd, GW_HWNDLAST);
		HWND hLastChild = oWinGetLastChild(hPrev);
		if (hLastChild) hPrev = hLastChild;
	}

	return hPrev;
}

// Cycles through all next windows until the next valid tabstop is found
static HWND oWinGetNextTabStop()
{
	HWND hCurrent = GetFocus();
	HWND hNext = oWinGetNextWindow(hCurrent);
	while (hNext != hCurrent && hNext && !oWinControlIsTabStop(hNext))
		hNext = oWinGetNextWindow(hNext);
	return hNext;
}

// Cycles through all prev windows until the prev valid tabstop is found
static HWND oWinGetPrevTabStop()
{
	HWND hCurrent = GetFocus();
	HWND hPrev = oWinGetPrevWindow(hCurrent);
	while (hPrev != hCurrent && hPrev && !oWinControlIsTabStop(hPrev))
		hPrev = oWinGetPrevWindow(hPrev);
	return hPrev;
}

// No Wisdom from Google has yielded a rational Microsoft-way of handling 
// Ctrl-Tab and Shift-Ctrl-Tab in a way consistent with Microsoft-system 
// dialogs. This function wraps IsDialogMessage in a way that seems to conform
// more to MS standards than their own function by intercepting WM_KEYDOWN for
// VK_TAB.
static bool oIsDialogMessageEx(HWND _hWnd, MSG* _pMsg)
{
	oWIN_CHECK(_hWnd);
	if (_pMsg->message == WM_KEYDOWN && _pMsg->wParam == VK_TAB)
	{
		bool CtrlDown = (GetKeyState(VK_LCONTROL) & 0x1000) || (GetKeyState(VK_RCONTROL) & 0x1000);
		bool ShiftDown = (GetKeyState(VK_LSHIFT) & 0x1000) || (GetKeyState(VK_RSHIFT) & 0x1000);
		if (CtrlDown)
		{
			// @tony: Hmm, this won't behave well with multiple tab controls in
			// a single window
			HWND hTab = FindWindowEx(_hWnd, nullptr, WC_TABCONTROL, nullptr);
			while (hTab && (!IsWindowEnabled(hTab) || !IsWindowVisible(hTab)))
				hTab = FindWindowEx(_hWnd, hTab, WC_TABCONTROL, nullptr);

			if (hTab)
			{
				NMHDR n;
				n.hwndFrom = hTab;
				n.idFrom = GetDlgCtrlID(hTab);
				n.code = TCN_SELCHANGING;
				// @tony: Not sure what this should be because the docs say it 
				// isn't the case that n.idFrom is the ID to use, but I always observe 
				// the values being the same in debug spew
				WPARAM w = n.idFrom;
				SendMessage(_hWnd, WM_NOTIFY, w, (LPARAM)&n);
				oWinControlSelectSubItemRelative(hTab, ShiftDown ? -1 : 1);
				n.code = TCN_SELCHANGE;
				SendMessage(_hWnd, WM_NOTIFY, w, (LPARAM)&n);
				return true;
			}
		}

		else
		{
			HWND hNext = ShiftDown ? oWinGetPrevTabStop() : oWinGetNextTabStop();
			if (hNext)
			{
				oWinSetFocus(hNext);
				return true;
			}
		}
	}

	return !!IsDialogMessage(_hWnd, _pMsg);
}

int oWinDispatchMessage(HWND _hWnd, HACCEL _hAccel, bool wait_for_next)
{
	MSG msg;
	bool HasMessage = false;
	if (wait_for_next)
	{
		int n = GetMessage(&msg, nullptr, 0, 0);
		if (n == 0 || msg.message == WM_QUIT || msg.message == oWM_QUIT)
			return -1;
		HasMessage = n > 0;
	}
	else
		HasMessage = !!PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE);

	if (HasMessage)
	{
		if (!_hWnd)
			_hWnd = msg.hwnd;

		oWIN_CHECK(_hWnd);

		HasMessage = !!TranslateAccelerator(_hWnd, _hAccel, &msg);
		if (!HasMessage)
			HasMessage = oIsDialogMessageEx(_hWnd, &msg);

		if (!HasMessage)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// Treat WM_NULL like the noop it is by indicating that there are no new messages
		if (msg.message != WM_NULL)
			return 1;
	}

	return 0;
}

HMENU oWinGetMenu(HWND _hWnd)
{
	oWIN_CHECK(_hWnd);
	return (HMENU)GetWindowLongPtr(_hWnd, oGWLP_MENU);
}

void oWinShowMenu(HWND _hWnd, bool _Show)
{
	oWIN_CHECK(_hWnd);
	HMENU hMenu = oWinGetMenu(_hWnd);
	oAssert(hMenu, "invalid menu");

	// If the top-level menu is empty then not even the bar gets drawn and the
	// client area is resized. So in this case preserve the sizing by avoiding 
	// the call.
	if (!_Show || GetMenuItemCount(hMenu) > 0)
		oVB(::SetMenu(_hWnd, _Show ? hMenu : nullptr));
}

bool oWinMenuShown(HWND _hWnd)
{
	oWIN_CHECK(_hWnd);
	return !!GetMenu(_hWnd);
}

HWND oWinGetStatusBar(HWND _hWnd)
{
	oWIN_CHECK(_hWnd);
	return (HWND)GetWindowLongPtr(_hWnd, oGWLP_STATUSBAR);
}

void oWinShowStatusBar(HWND _hWnd, bool _Show)
{
	oWIN_CHECK(_hWnd);
	HWND hStatusBar = oWinGetStatusBar(_hWnd);
	if (hStatusBar)
		if (!ShowWindow(hStatusBar, _Show ? SW_SHOWNOACTIVATE : SW_HIDE))
			oVB(RedrawWindow(hStatusBar, nullptr, nullptr, RDW_INVALIDATE|RDW_UPDATENOW));
}

bool oWinStatusBarShown(HWND _hWnd)
{
	oWIN_CHECK(_hWnd);
	HWND hStatusBar = oWinGetStatusBar(_hWnd);
	if (hStatusBar)
		return !!(GetWindowLongPtr(hStatusBar, GWL_STYLE) & WS_VISIBLE);
	return false;
}

bool oWinSetOwner(HWND _hWnd, HWND _hOwner)
{
	// Yes, you've read this right... Check the community add-ons:
	// http://msdn.microsoft.com/en-us/library/windows/desktop/ms633591(v=vs.85).aspx
	SetLastError(S_OK);
	oVB(SetWindowLongPtr(_hWnd, GWLP_HWNDPARENT, (LONG_PTR)_hOwner) || GetLastError() == S_OK);
	return true;
}

HWND oWinGetOwner(HWND _hWnd)
{
	return GetWindow(_hWnd, GW_OWNER);
}

void oWinSetParent(HWND _hWnd, HWND _hParent)
{
	oWIN_CHECK(_hWnd);
	if (_hParent)
	{
		DWORD dwStyle = (DWORD)GetWindowLongPtr(_hWnd, GWL_STYLE);
		dwStyle = (dwStyle & ~WS_POPUP) | WS_CHILD;
		SetWindowLongPtr(_hWnd, GWL_STYLE, dwStyle);
	}

	::SetParent(_hWnd, _hParent);

	if (!_hParent)
	{
		DWORD dwStyle = (DWORD)GetWindowLongPtr(_hWnd, GWL_STYLE);
		dwStyle = (dwStyle & ~WS_CHILD) | WS_POPUP;
		SetWindowLongPtr(_hWnd, GWL_STYLE, dwStyle);
	}
}

HWND oWinGetParent(HWND _hWnd)
{
	return ::GetParent(_hWnd);
}

bool oWinIsParent(HWND _hWnd)
{
	HWND hChild = GetWindow(_hWnd, GW_CHILD);
	return !!hChild;
}

struct OWNER_CTX
{
	OWNER_CTX(HWND _hOwner) : hOwner(_hOwner), IsOwner(false) {}
	HWND hOwner;
	bool IsOwner;
};

static BOOL CALLBACK CheckIsOwner(HWND _hWnd, LPARAM _lParam)
{
	// if there's even one child, then this is a parent.
	OWNER_CTX& ctx = *(OWNER_CTX*)_lParam;
	HWND hOwner = GetWindow(_hWnd, GW_OWNER);
	if (hOwner == ctx.hOwner)
		ctx.IsOwner = true;
	return !ctx.IsOwner;
}

bool oWinIsOwner(HWND _hWnd)
{
	oWIN_CHECK(_hWnd);
	OWNER_CTX ctx(_hWnd);
	EnumWindows(CheckIsOwner, (LPARAM)&ctx);
	return ctx.IsOwner;
}

bool oWinIsRenderTarget(HWND _hWnd)
{
	oWIN_CHECK(_hWnd);
	return !!(GetWindowLongPtr(_hWnd, oGWLP_EXTRA_FLAGS) & oEF_RENDER_TARGET);
}

bool oWinIsThreadOwner(HWND _hWnd)
{
	oWIN_CHECK(_hWnd);
	return !!(GetWindowLongPtr(_hWnd, oGWLP_EXTRA_FLAGS) & oEF_THREAD_OWNER);
}

bool oWinSetIsRenderTarget(HWND _hWnd, bool _IsRenderTarget)
{
	oWIN_CHECK(_hWnd);
	LONG_PTR flags = GetWindowLongPtr(_hWnd, oGWLP_EXTRA_FLAGS);
	if (_IsRenderTarget) flags |= oEF_RENDER_TARGET;
	else flags &=~ oEF_RENDER_TARGET;
	SetWindowLongPtr(_hWnd, oGWLP_EXTRA_FLAGS, flags);
	return true;
}

bool oWinIsFullscreenExclusive(HWND _hWnd)
{
	oWIN_CHECK(_hWnd);
	return !!(GetWindowLongPtr(_hWnd, oGWLP_EXTRA_FLAGS) & oEF_FULLSCREEN_EXCLUSIVE);
}

bool oWinSetIsFullscreenExclusive(HWND _hWnd, bool _IsFullscreenExclusive)
{
	oWIN_CHECK(_hWnd);
	LONG_PTR flags = GetWindowLongPtr(_hWnd, oGWLP_EXTRA_FLAGS);
	if (_IsFullscreenExclusive) flags |= oEF_FULLSCREEN_EXCLUSIVE;
	else flags &=~ oEF_FULLSCREEN_EXCLUSIVE;
	SetWindowLongPtr(_hWnd, oGWLP_EXTRA_FLAGS, flags);
	return true;
}

bool oWinAltF4IsEnabled(HWND _hWnd)
{
	oWIN_CHECK(_hWnd);
	return !!(GetWindowLongPtr(_hWnd, oGWLP_EXTRA_FLAGS) & oEF_ALT_F4_ENABLED);
}

void oWinAltF4Enable(HWND _hWnd, bool _Enabled)
{
	oWIN_CHECK(_hWnd);
	LONG_PTR flags = GetWindowLongPtr(_hWnd, oGWLP_EXTRA_FLAGS);
	if (_Enabled) flags |= oEF_ALT_F4_ENABLED;
	else flags &=~ oEF_ALT_F4_ENABLED;
	SetWindowLongPtr(_hWnd, oGWLP_EXTRA_FLAGS, flags);
}

bool oWinExists(HWND _hWnd)
{
	return !!::IsWindow(_hWnd);
}

bool oWinHasFocus(HWND _hWnd)
{
	oWIN_CHECK(_hWnd);
	return oWinExists(_hWnd) && _hWnd == ::GetForegroundWindow();
}

bool oWinSetFocus(HWND _hWnd, bool _Focus)
{
	// @oooii-kevin: Technically this can be called from other threads as we can give focus to another window
	// @tony: This shouldn't be true, there's several calls below so thus
	// this isn't atomic. See who complains by reenabling the check here.
	oWIN_CHECK(_hWnd);
	if (_Focus)
	{
		::SetForegroundWindow(_hWnd);
		::SetActiveWindow(_hWnd);
		::SetFocus(_hWnd);
		return true;
	}
	
	else
		return oWinSetFocus(GetNextWindow(_hWnd, GW_HWNDNEXT), true);
}

bool oWinIsEnabled(HWND _hWnd)
{
	oWIN_CHECK(_hWnd);
	oVB(IsWindowEnabled(_hWnd));
	return true;
}

bool oWinEnable(HWND _hWnd, bool _Enabled)
{
	oWIN_CHECK(_hWnd);
	EnableWindow(_hWnd, BOOL(_Enabled));
	return true;
}

bool oWinIsAlwaysOnTop(HWND _hWnd)
{
	oWIN_CHECK(_hWnd);
	return !!(GetWindowLong(_hWnd, GWL_EXSTYLE) & WS_EX_TOPMOST);
}

void oWinSetAlwaysOnTop(HWND _hWnd, bool _AlwaysOnTop)
{
	oWIN_CHECK(_hWnd);
	RECT r;
	oVB(GetWindowRect(_hWnd, &r));
	if (!::SetWindowPos(_hWnd, _AlwaysOnTop ? HWND_TOPMOST : HWND_TOP, r.left, r.top, oWinRectW(r), oWinRectH(r), IsWindowVisible(_hWnd) ? SWP_SHOWWINDOW : SWP_HIDEWINDOW))
		throw windows::error();
}

void oWinGetClientRect(HWND _hWnd, RECT* _pRect)
{
	oWIN_CHECK(_hWnd);

	if (!_pRect)
		oThrow(std::errc::invalid_argument, "");

	// Discount status bar from dimensions
	oVB(GetClientRect(_hWnd, _pRect));
	_pRect->bottom = __max(_pRect->top, _pRect->bottom - oWinGetStatusBarHeight(_hWnd));

	// Translate to proper offset
	POINT p = { _pRect->left, _pRect->top };
	HWND hParent = GetParent(_hWnd);
	if (!hParent)
		oVB(ClientToScreen(_hWnd, &p));
	else
	{
		SetLastError(S_OK); // differentiate between 0-means-failure, and 0,0 adjustment
		oVB(MapWindowPoints(_hWnd, hParent, &p, 1) || GetLastError() == S_OK);
	}
	
	OffsetRect(_pRect, p.x, p.y);
}

RECT oWinGetParentRect(HWND _hWnd, HWND _hExplicitParent)
{
	HWND hParent = _hExplicitParent ? _hExplicitParent : GetParent(_hWnd);
	RECT rParent;
	if (hParent)
		oWinGetClientRect(hParent, &rParent);
	else
	{
		display::info di = display::get_info(oWinGetDisplayId(_hWnd));
		rParent = oWinRectWH(int2(di.workarea_x, di.workarea_y), int2(di.workarea_width, di.workarea_height));
	}
	return rParent;
}

RECT oWinGetRelativeRect(HWND _hWnd, HWND _hExplicitParent)
{
	HWND hParent = _hExplicitParent ? _hExplicitParent : GetParent(_hWnd);
	if (!hParent)
		hParent = GetDesktopWindow();

	RECT r;
	GetWindowRect(_hWnd, &r);
	ScreenToClient(hParent, (POINT*)&r);
	ScreenToClient(hParent, &((POINT*)&r)[1]);
	return r;	
}

window_state oWinGetState(HWND _hWnd)
{
	if (!oWinExists(_hWnd)) return window_state::invalid;
	
	LONG_PTR style = GetWindowLongPtr(_hWnd, GWL_STYLE);
	if (!(style & WS_VISIBLE)) return window_state::hidden;

	LONG_PTR extra = GetWindowLongPtr(_hWnd, oGWLP_EXTRA_FLAGS);
	if (extra & (oEF_FULLSCREEN_EXCLUSIVE|oEF_FULLSCREEN_COOPERATIVE)) return window_state::fullscreen;
	
	if (IsIconic(_hWnd)) return window_state::minimized;
	if (IsZoomed(_hWnd)) return window_state::maximized;

	return window_state::restored;
}

static window_style oWinGetStyle(HWND _hWnd)
{
	#define oFIXED_STYLE (WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX)
	#define oSET(_Flag) ((style & (_Flag)) == (_Flag))

	oAssert(oWinExists(_hWnd) && oWinIsWindowThread(_hWnd), "function must be called on window thread");
	const bool HasMenu = !!GetMenu(_hWnd);
	const bool HasStatusBar = oWinStatusBarShown(_hWnd);

	if (HasStatusBar && oWinIsRenderTarget(_hWnd))
	{
		oTrace("HWND 0x%x is showing a status bar and is a render target, which is not allowed. The application will now terminate.", _hWnd);
		std::terminate();
	}

	LONG_PTR style = GetWindowLongPtr(_hWnd, GWL_STYLE);
	if (oSET(WS_OVERLAPPEDWINDOW))
	{
		if (HasMenu && HasStatusBar) return window_style::sizable_with_menu_and_statusbar;
		else if (HasMenu) return window_style::sizable_with_menu;
		else if (HasStatusBar) return window_style::sizable_with_statusbar;
		else return window_style::sizable;
	}
	else if (oSET(oFIXED_STYLE))
	{
		if (HasMenu && HasStatusBar) return window_style::fixed_with_menu_and_statusbar;
		else if (HasMenu) return window_style::fixed_with_menu;
		else if (HasStatusBar) return window_style::fixed_with_statusbar;
		else return window_style::fixed;
	}

	LONG_PTR exstyle = GetWindowLongPtr(_hWnd, GWL_EXSTYLE);
	if (exstyle & WS_EX_WINDOWEDGE) return window_style::dialog;
	return window_style::borderless;
	
	#undef oFIXED_STYLE
	#undef oSET
}

window_shape oWinGetShape(HWND _hWnd)
{
	window_shape s;
	RECT rClient;
	oWinGetClientRect(_hWnd, &rClient);
	s.client_size = oWinRectSize(rClient);
	s.client_position = oWinRectPosition(rClient);
	s.state = oWinGetState(_hWnd);
	s.style = oWinGetStyle(_hWnd);
	return s;
}

void oWinSetShape(HWND _hWnd, const window_shape& _Shape)
{
	oWIN_CHECK(_hWnd);

	window_shape New = _Shape;
	window_shape Old = oWinGetShape(_hWnd);

	if (Old.state == window_state::invalid || Old.style == window_style::default_style)
		oThrow(std::errc::invalid_argument, "");

	if (New.state == window_state::invalid)
		New.state = Old.state;

	if (New.style == window_style::default_style)
		New.style = Old.style;

	oCheck(!has_statusbar(New.style) || !oWinIsRenderTarget(_hWnd), std::errc::protocol_error, "HWND 0x%x is marked as a render target, disallowing status bar styles to be set", _hWnd);

	if (Old.state == window_state::restored)
		oWinSaveRestoredPosSize(_hWnd);

	oFinally { SetWindowLongPtr(_hWnd, oGWLP_EXTRA_FLAGS, GetWindowLongPtr(_hWnd, oGWLP_EXTRA_FLAGS) &~ oEF_NO_SAVE_RESTORED_POSITION_SIZE); };
	if (New.state == window_state::fullscreen || New.state == window_state::minimized || New.state == window_state::maximized)
		SetWindowLongPtr(_hWnd, oGWLP_EXTRA_FLAGS, GetWindowLongPtr(_hWnd, oGWLP_EXTRA_FLAGS) | oEF_NO_SAVE_RESTORED_POSITION_SIZE);

	if (Old.state != New.state)
		SetWindowLongPtr(_hWnd, oGWLP_PREVIOUS_STATE, (LONG_PTR)Old.state);

	if (Old.state != window_state::fullscreen)
		SetWindowLongPtr(_hWnd, oGWLP_PREVIOUS_STYLE, (LONG_PTR)Old.style);

	if (New.state == window_state::fullscreen)
	{
		New.style = window_style::borderless;
		LONG_PTR extra = GetWindowLongPtr(_hWnd, oGWLP_EXTRA_FLAGS);
		if (New.state == window_state::fullscreen)
			extra |= oEF_FULLSCREEN_COOPERATIVE;
		else
			extra &=~ oEF_FULLSCREEN_COOPERATIVE;
		SetWindowLongPtr(_hWnd, oGWLP_EXTRA_FLAGS, extra);
	}
	else if (Old.state == window_state::fullscreen)
	{
		New.style = (window_style)GetWindowLongPtr(_hWnd, oGWLP_PREVIOUS_STYLE);
		SetWindowLongPtr(_hWnd, oGWLP_EXTRA_FLAGS, GetWindowLongPtr(_hWnd, oGWLP_EXTRA_FLAGS) & ~(oEF_FULLSCREEN_COOPERATIVE|oEF_FULLSCREEN_EXCLUSIVE));
	}

	if (New.state == window_state::restored)
	{
		LONG_PTR lParam = GetWindowLongPtr(_hWnd, oGWLP_RESTORED_POSITION);
		const int2 RestoredClientPosition(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		New.client_position = resolve_default(New.client_position, RestoredClientPosition);
		lParam = GetWindowLongPtr(_hWnd, oGWLP_RESTORED_SIZE);
		const int2 RestoredClientSize(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		New.client_size = resolve_default(New.client_size, RestoredClientSize);
	}

	else if (New.state == window_state::fullscreen)
	{
		display::info di = display::get_info(oWinGetDisplayId(_hWnd));
		New.client_position = int2(di.x, di.y);
		New.client_size = int2(di.mode.width, di.mode.height);
	}

	else
	{
		New.client_position = resolve_default(New.client_position, Old.client_position);
		New.client_size = resolve_default(New.client_size, Old.client_size);
	}

	int StatusBarHeight = 0;
	{
		RECT rStatusBar;
		GetClientRect(oWinGetStatusBar(_hWnd), &rStatusBar);
		StatusBarHeight = oWinRectH(rStatusBar);
	}

	DWORD dwAllFlags = (DWORD)GetWindowLongPtr(_hWnd, GWL_STYLE);
	if (Old.style != New.style)
	{
		// Change only the bits we mean to change and preserve the others
		const bool HasParent = !!GetParent(_hWnd);
		DWORD dwCurrentStyleFlags = oWinGetStyle(Old.style, HasParent);
		dwAllFlags &=~ dwCurrentStyleFlags;
		dwAllFlags |= oWinGetStyle(New.style, HasParent);

		// When in maximized state and doing nothing more than toggling status bar
		// visibility, the client area has changed, so spoof a WM_SIZE to notify
		// the system of that change. If the menu changes, then allow the WM_SIZE to 
		// be sent by oWinShowMenu.
		oWinShowStatusBar(_hWnd, has_statusbar(New.style));

		if (Old.state == window_state::maximized && New.state == window_state::maximized 
			&& has_statusbar(New.style) != has_statusbar(Old.style)
			&& has_menu(New.style) == has_menu(Old.style))
		{
			int2 NewMaxSize = Old.client_size;
			if (has_statusbar(New.style))
				NewMaxSize.y -= StatusBarHeight;
			else
				NewMaxSize.y += StatusBarHeight;
			SendMessage(_hWnd, WM_SIZE, SIZE_MAXIMIZED, MAKELPARAM(NewMaxSize.x, NewMaxSize.y));
		}

		// This will send a temp WM_SIZE event. Another will be sent below, so 
		// squelch this one from making it all the way through to an event_type.
		// However, during a style change to a maximized window, that affects the 
		// menu other calls won't be made, so let this be the authority in that case.
		oWinSetTempChange(_hWnd, !(Old.state == window_state::maximized && New.state == window_state::maximized));
		oWinShowMenu(_hWnd, has_menu(New.style));
		oWinSetTempChange(_hWnd, false);
	}

	// Resolve position and size to a rectangle. Add extra room for the status bar.
	RECT r = oWinRectWH(New.client_position, New.client_size);
	if (has_statusbar(New.style))
		r.bottom += StatusBarHeight;

	// @tony: are these bit-clears needed?
	if (New.state != window_state::maximized)
		dwAllFlags &=~ WS_MAXIMIZE;

	if (New.state != window_state::minimized)
		dwAllFlags &=~ WS_MINIMIZE;

	// Transform the rectangle to with-border values for the new style
	oVB(AdjustWindowRect(&r, dwAllFlags, !!GetMenu(_hWnd)));

	// Update the flagging of the new style. This won't do anything without a call 
	// to SetWindowPos.
	SetLastError(0); // http://msdn.microsoft.com/en-us/library/ms644898(VS.85).aspx
	oWinSetTempChange(_hWnd, true);
	oVB(SetWindowLongPtr(_hWnd, GWL_STYLE, dwAllFlags));
	oWinSetTempChange(_hWnd, false);

	// Allow the system to calculate minimized/maximized sizes for us, so don't
	// modify them here, that will happen in SetWindowPlacement.
	if (!(Old.state == window_state::fullscreen && New.state == window_state::maximized))
	{
		UINT uFlags = SWP_NOZORDER|SWP_FRAMECHANGED;
		if (New.state == window_state::minimized || New.state == window_state::maximized)
			uFlags |= SWP_NOMOVE|SWP_NOSIZE;

		MINMAXINFO mmi = {0};
		SendMessage(_hWnd, WM_GETMINMAXINFO, 0, (LPARAM)&mmi);
		int width = __max(oWinRectW(r), mmi.ptMinTrackSize.x);
		int height = __max(oWinRectH(r), mmi.ptMinTrackSize.y);

		oVB(SetWindowPos(_hWnd, 0, r.left, r.top, width, height, uFlags));
	}

	// Now handle visibility, min/max/restore
	if (Old.state != New.state)
	{
		WINDOWPLACEMENT WP;
		WP.length = sizeof(WINDOWPLACEMENT);
		GetWindowPlacement(_hWnd, &WP);

		switch (New.state)
		{
			case window_state::invalid: case window_state::hidden:
				WP.showCmd = SW_HIDE;
				break;
			case window_state::minimized:
				WP.showCmd = SW_SHOWMINNOACTIVE;
				break;
			case window_state::maximized:
				WP.showCmd = SW_SHOWMAXIMIZED;
				break;
			case window_state::fullscreen:
			case window_state::restored:
				WP.showCmd = SW_RESTORE;
				WP.rcNormalPosition = r;
				break;
		}
		oVB(SetWindowPlacement(_hWnd, &WP));
	}
}

int2 oWinGetDecoratedWindowSize(HWND _hWnd, const int2& _ClientSize)
{
	RECT r = { 0, 0, _ClientSize.x, _ClientSize.y };
	r.bottom += oWinGetStatusBarHeight(_hWnd);
	AdjustWindowRect(&r, GetWindowLong(_hWnd, GWL_STYLE), !!GetMenu(_hWnd));
	return int2(oWinRectW(r), oWinRectH(r));
}

bool oWinRestore(HWND _hWnd)
{
	oWIN_CHECK(_hWnd);
	HWND hProgMan = FindWindow(0, "Program Manager");
	oAssert(hProgMan, "Program Manager not found");
	oWinSetFocus(hProgMan);
	oWinSetFocus(_hWnd);
	ShowWindow(_hWnd, SW_SHOWDEFAULT);
	return true;
}

bool oWinAnimate(HWND _hWnd, const RECT& _From, const RECT& _To)
{
	oWIN_CHECK(_hWnd);
	ANIMATIONINFO ai;
	ai.cbSize = sizeof(ai);
	SystemParametersInfo(SPI_GETANIMATION, sizeof(ai), &ai, 0);
	if (ai.iMinAnimate)
		return !!DrawAnimatedRects(_hWnd, IDANI_CAPTION, &_From, &_To);
	return true;
}

display::id oWinGetDisplayId(HWND _hWnd)
{
	return display::get_id(MonitorFromWindow(_hWnd, MONITOR_DEFAULTTONEAREST));
}

HBRUSH oWinGetBackgroundBrush(HWND _hWnd)
{
	return (HBRUSH)GetClassLongPtr(_hWnd, GCLP_HBRBACKGROUND);
}

HFONT oWinGetFont(HWND _hWnd)
{
	return (HFONT)SendMessage(_hWnd, WM_GETFONT, 0, 0);
}

HFONT oWinGetDefaultFont()
{
	return (HFONT)GetStockObject(DEFAULT_GUI_FONT);
}

bool oWinSetFont(HWND _hWnd, HFONT _hFont)
{
	oWIN_CHECK(_hWnd);
	if (!_hFont)
		_hFont = oWinGetDefaultFont();
	SendMessage(_hWnd, WM_SETFONT, (WPARAM)_hFont, TRUE);
	return true;
}

bool oWinSetText(HWND _hWnd, const char* _Text, int _SubItemIndex)
{
	oWIN_CHECK(_hWnd);
	control_type type = oWinControlGetType(_hWnd);
	switch (type)
	{
		case control_type::combotextbox:
			if (_SubItemIndex == -1)
				oVB(SetWindowText(_hWnd, _Text));
			// pass through
		case control_type::combobox:
			if (_SubItemIndex != -1)
			{
				oWinControlDeleteSubItem(_hWnd, _Text, _SubItemIndex);
				oWinControlInsertSubItem(_hWnd, _Text, _SubItemIndex);
			}
			break;

		case control_type::tab:
		{
			TCITEM tci;
			memset(&tci, 0, sizeof(tci));
			tci.dwStateMask = TCIF_TEXT;
			tci.pszText = (LPSTR)_Text;
			oVB(TabCtrl_SetItem(_hWnd, _SubItemIndex, &tci));
			break;
		}

		case control_type::floatbox:
		case control_type::floatbox_spinner:
			oThrow(std::errc::invalid_argument, "For FloatBoxes and FloatBoxSpinners, use oWinControlSetValue instead.");

		default: 
			oVB(SetWindowText(_hWnd, _Text));
	}

	return true;
}

size_t oWinGetTruncatedLength(HWND _hWnd, const char* _StrSource)
{
	xxlstring temp(_StrSource);

	RECT rClient;
	GetClientRect(_hWnd, &rClient);
	scoped_getdc hDC(_hWnd);

	// @tony: This constant was measured empirically. I think this should work without the +70, but truncation happens too aggressively.
	oCheck(PathCompactPathA(hDC, temp, oWinRectW(rClient) + 70), std::errc::no_buffer_space, "Buffer must be at least MAX_PATH (%u) chars big", MAX_PATH);

	return temp.length();
}

char* oWinTruncateLeft(char* _StrDestination, size_t _SizeofStrDestination, HWND _hWnd, const char* _StrSource)
{
	size_t TruncatedLength = oWinGetTruncatedLength(_hWnd, _StrSource);
	if (TruncatedLength)
	{
		uri_string Truncated;
		const char* pCopy = _StrSource + strlen(_StrSource) - TruncatedLength + 3;
		oAssert(pCopy >= _StrSource, "");
		oCheck(-1 == ouro::snprintf(_StrDestination, _SizeofStrDestination, "...%s", pCopy), std::errc::no_buffer_space, "");
	}

	return _StrDestination;
}

char* oWinTruncatePath(char* _StrDestination, size_t _SizeofStrDestination, HWND _hWnd, const char* _Path)
{
	oCheck(_SizeofStrDestination >= MAX_PATH, std::errc::no_buffer_space, "_SizeofStrDestination must be at least MAX_PATH (%d)", MAX_PATH); // can't pass this to the function
	strlcpy(_StrDestination, _Path, _SizeofStrDestination);
	RECT rClient;
	GetClientRect(_hWnd, &rClient);
	scoped_getdc hDC(_hWnd);
	// @tony: This constant was measured empirically. I think this should work without the +70, but truncation happens too aggressively.
	oCheck(PathCompactPathA(hDC, _StrDestination, oWinRectW(rClient) + 70), std::errc::no_buffer_space, "Buffer must be at least MAX_PATH (%u) chars big", MAX_PATH);
	return _StrDestination;
}

char* oWinGetText(char* _StrDestination, size_t _SizeofStrDestination, HWND _hWnd, int _SubItemIndex)
{
	oWINVP(_hWnd);
	control_type type = control_type::unknown;
	if (_SubItemIndex != -1)
		type = oWinControlGetType(_hWnd);
	switch (type)
	{
		case control_type::combobox:
		case control_type::combotextbox:
		{
			size_t len = (size_t)ComboBox_GetLBTextLen(_hWnd, _SubItemIndex);
			oCheck(len <= _SizeofStrDestination, std::errc::no_buffer_space, "");
			if (CB_ERR != ComboBox_GetLBText(_hWnd, _SubItemIndex, _StrDestination))
				return _StrDestination;
		}

		case control_type::tab:
		{
			TCITEM item;
			item.mask = TCIF_TEXT;
			item.pszText = (LPSTR)_StrDestination;
			item.cchTextMax = int(_SizeofStrDestination);
			oVB(TabCtrl_GetItem(_hWnd, _SubItemIndex, &item));
			return _StrDestination;
		}

		default:
			oVB(GetWindowText(_hWnd, _StrDestination, int(_SizeofStrDestination)));
			return _StrDestination;
	}
}

HICON oWinGetIcon(HWND _hWnd, bool _BigIcon)
{
	//oWINVP(_hWnd);
	control_type type = oWinControlGetType(_hWnd);
	switch (type)
	{
		case control_type::icon: return (HICON)SendMessage(_hWnd, STM_GETICON, 0, 0);
		default: return (HICON)SendMessage(_hWnd, WM_GETICON, (WPARAM)(_BigIcon ? ICON_BIG : ICON_SMALL), 0);
	}
}

// If on the windows thread, this uses SendMessage else this uses PostMessage
void send_message(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (::IsWindow(hwnd))
	{
		if (hwnd == (HWND)0xffff || GetCurrentThreadId() != GetWindowThreadProcessId(hwnd, nullptr))
			PostMessageA(hwnd, msg, wparam, lparam);
		else
			SendMessageA(hwnd, msg, wparam, lparam);
	}
}

void oWinSetIconAsync(HWND _hWnd, HICON _hIcon, bool _BigIcon)
{
	control_type type = oWinControlGetType(_hWnd);
	if (type == control_type::icon)
		send_message(_hWnd, STM_SETICON, (WPARAM)_hIcon, 0);
	else
		send_message(_hWnd, WM_SETICON, (WPARAM)(_BigIcon ? ICON_BIG : ICON_SMALL), (LPARAM)_hIcon);
}

void oWinSetIcon(HWND _hWnd, HICON _hIcon, bool _BigIcon)
{
	oWINVP(_hWnd);
	oWinSetIconAsync(_hWnd, _hIcon, _BigIcon);
}

static const char* oSubclassFloatBoxFormat = "%.04f";
static UINT_PTR oSubclassFloatBoxID = 0x13370000;
static LRESULT CALLBACK oSubclassProcFloatBox(HWND _hControl, UINT _uMsg, WPARAM _wParam, LPARAM _lParam, UINT_PTR _uIdSubclass, DWORD_PTR _dwRefData)
{
	if (_uIdSubclass == oSubclassFloatBoxID)
	{
		switch (_uMsg)
		{
			case WM_KILLFOCUS:
			{
				mstring text;
				GetWindowText(_hControl, text, (int)text.capacity());
				float f = 0.0f;
				atof(text, &f);
				snprintf(text, oSubclassFloatBoxFormat, f);
				SetWindowText(_hControl, text);
				break;
			}

			case WM_SETTEXT:
			{
				float f = 0.0f;
				mstring text = (const wchar_t*)_lParam; // handle wchar
				if (!atof(text, &f))
					return FALSE;
				
				// Ensure consistent formatting
				snprintf(text, oSubclassFloatBoxFormat, f);
				mwstring wtext = text;
				DefSubclassProc(_hControl, _uMsg, _wParam, (LPARAM)wtext.c_str());
				return FALSE;
			}

			case WM_GETDLGCODE:
			{
				// probably copy/paste, so let that through
				if ((GetKeyState(VK_LCONTROL) & 0x1000) || (GetKeyState(VK_RCONTROL) & 0x1000))
					return DLGC_WANTALLKEYS;

				mstring text;
				GetWindowText(_hControl, text, (int)text.capacity());

				// refresh/commit if the user presses enter
				if (_wParam == VK_RETURN)
				{
					SetWindowText(_hControl, text.c_str());
					SendMessage(_hControl, EM_SETSEL, text.length(), text.length());
					return DLGC_WANTARROWS;
				}

				// Only consider allowable keys at all
				static const WPARAM sAllowableKeys[] = 
				{
					'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '.', '-', '+',
					VK_BACK, VK_DELETE, VK_HOME, VK_END,
				};
				int i = 0;
				for (; i < countof(sAllowableKeys); i++)
					if (sAllowableKeys[i] == _wParam) break;
				
				if (i >= countof(sAllowableKeys))
					return DLGC_WANTARROWS;

				DWORD dwStart = ~0u, dwEnd = ~0u;
				SendMessage(_hControl, EM_GETSEL, (WPARAM)&dwStart, (LPARAM)&dwEnd);

				// allow only one '.', unless it's in the selecte d text about to be replaced
				bool selHasDot = false;
				for (DWORD j = dwStart; j < dwEnd; j++)
					if (text[j] == '.')
					{
						selHasDot = true;
						break;
					}

				if (!selHasDot && _wParam == '.' && strchr(text, '.'))
					return DLGC_WANTARROWS;

				// only one of either + or -, and it better be at the front
				if ((_wParam == '+' || _wParam == '-') && (dwStart != 0 || strchr(text, '+') || strchr(text, '-')))
					return DLGC_WANTARROWS;

				return DLGC_WANTALLKEYS;
			}

			default:
				break;
		}
	}

	return DefSubclassProc(_hControl, _uMsg, _wParam, _lParam);
}

static UINT_PTR oSubclassTabID = 0x13370001;
static LRESULT CALLBACK oSubclassProcTab(HWND _hControl, UINT _uMsg, WPARAM _wParam, LPARAM _lParam, UINT_PTR _uIdSubclass, DWORD_PTR _dwRefData)
{
	if (_uIdSubclass == oSubclassTabID)
	{
		switch (_uMsg)
		{
			case WM_CTLCOLORSTATIC:
				return (INT_PTR)0;
			case WM_COMMAND:
			case WM_NOTIFY:
				return SendMessage(GetParent(_hControl), _uMsg, _wParam, _lParam);
			default:
				break;
		}
	}

	return DefSubclassProc(_hControl, _uMsg, _wParam, _lParam);
}

static UINT_PTR oSubclassGroupID = 0x13370002;
static LRESULT CALLBACK oSubclassProcGroup(HWND _hControl, UINT _uMsg, WPARAM _wParam, LPARAM _lParam, UINT_PTR _uIdSubclass, DWORD_PTR _dwRefData)
{
	if (_uIdSubclass == oSubclassGroupID)
	{
		switch (_uMsg)
		{
			case WM_CTLCOLORSTATIC:
			{
				HBRUSH hBrush = oWinGetBackgroundBrush(_hControl);
				return (INT_PTR)hBrush;
			}
			case WM_COMMAND:
			case WM_NOTIFY:
				return SendMessage(GetParent(_hControl), _uMsg, _wParam, _lParam);

			default:
				break;
		}
	}

	return DefSubclassProc(_hControl, _uMsg, _wParam, _lParam);
}

static bool OnCreateAddSubItems(HWND _hControl, const control_info& _Desc)
{
	if (strchr(_Desc.text, '|'))
	{
		if (!oWinControlAddSubItems(_hControl, _Desc.text)) return false;
		if (!oWinControlSelectSubItem(_hControl, 0)) return false;
	}

	return true;
}

static bool OnCreateTab(HWND _hControl, const control_info& _Desc)
{
	if (!SetWindowSubclass(_hControl, oSubclassProcTab, oSubclassTabID, (DWORD_PTR)nullptr)) return false;
	return OnCreateAddSubItems(_hControl, _Desc);
}

static bool OnCreateGroup(HWND _hControl, const control_info& _Desc)
{
	return !!SetWindowSubclass(_hControl, oSubclassProcGroup, oSubclassGroupID, (DWORD_PTR)nullptr);
}

static bool OnCreateFloatBox(HWND _hControl, const control_info& _Desc)
{
	if (!SetWindowSubclass(_hControl, oSubclassProcFloatBox, oSubclassFloatBoxID, (DWORD_PTR)nullptr)) return false;
	SetWindowText(_hControl, _Desc.text);
	return true;
}

static bool OnCreateIcon(HWND _hControl, const control_info& _Desc)
{
	oWinControlSetIcon(_hControl, (HICON)_Desc.text);
	return true;
}

static bool OnCreateFloatBoxSpinner(HWND _hControl, const control_info& _Desc)
{
	control_info ActualFloatBoxDesc = _Desc;
	ActualFloatBoxDesc.type = control_type::floatbox;
	HWND hActualFloatBox = oWinControlCreate(ActualFloatBoxDesc);
	oVB(hActualFloatBox);
	SendMessage(_hControl, UDM_SETBUDDY, (WPARAM)hActualFloatBox, 0);
	SendMessage(_hControl, UDM_SETRANGE32, (WPARAM)std::numeric_limits<int>::lowest(), (LPARAM)std::numeric_limits<int>::max());
	return true;
}

static bool OnCreateMarquee(HWND _hControl, const control_info& _Desc)
{
	SendMessage(_hControl, PBM_SETMARQUEE, 1, 0);
	return true;
}

static int2 GetSizeExplicit(const control_info& _Desc)
{
	return _Desc.size;
}

static int2 GetSizeButtonDefault(const control_info& _Desc)
{
	return resolve_default(_Desc.size, int2(75, 23));
}

static int2 GetSizeIcon(const control_info& _Desc)
{
	int2 iconSize = icon_dimensions((HICON)_Desc.text);
	return resolve_default(_Desc.size, iconSize);
}

struct CONTROL_CREATION_DESC
{
	const char* ClassName;
	DWORD dwStyle;
	DWORD dwExStyle;
	bool SetText;
	bool (*FinishCreation)(HWND _hControl, const control_info& _Desc);
	int2 (*GetSize)(const control_info& _Desc);
};

static const CONTROL_CREATION_DESC& oWinControlGetCreationDesc(control_type _Type)
{
	// === IF ADDING A NEW TYPE, MAKE SURE YOU ADD AN ENTRY TO oWinControlGetType ===

	static const CONTROL_CREATION_DESC sDescs[] = 
	{
		{ "Unknown", 0, 0, false, nullptr, GetSizeExplicit, },
		{ WC_BUTTON, WS_VISIBLE|WS_CHILD|BS_GROUPBOX, 0, true, OnCreateGroup, GetSizeExplicit, },
		{ WC_BUTTON, WS_VISIBLE|WS_CHILD|WS_TABSTOP|BS_MULTILINE|BS_PUSHBUTTON, 0, true, nullptr, GetSizeButtonDefault, },
		{ WC_BUTTON, WS_VISIBLE|WS_CHILD|WS_TABSTOP|BS_MULTILINE|BS_TOP|BS_AUTOCHECKBOX, 0, true, nullptr, GetSizeExplicit, },
		{ WC_BUTTON, WS_VISIBLE|WS_CHILD|WS_TABSTOP|BS_MULTILINE|BS_TOP|BS_AUTORADIOBUTTON, 0, true, nullptr, GetSizeExplicit, },
		{ WC_STATIC, WS_VISIBLE|WS_CHILD|SS_EDITCONTROL, 0, true, nullptr, GetSizeExplicit, },
		{ WC_STATIC, WS_VISIBLE|WS_CHILD|SS_EDITCONTROL|SS_CENTER, 0, true, nullptr, GetSizeExplicit, },
		{ "SysLink", WS_VISIBLE|WS_CHILD|WS_TABSTOP|LWS_NOPREFIX, 0, true, nullptr, GetSizeExplicit, },
		{ WC_EDIT, WS_VISIBLE|WS_CHILD|WS_TABSTOP|ES_READONLY|ES_NOHIDESEL|ES_AUTOHSCROLL, 0, true, nullptr, GetSizeExplicit, },
		{ WC_STATIC, WS_VISIBLE|WS_CHILD|SS_ICON|SS_REALSIZECONTROL, 0, false, OnCreateIcon, GetSizeIcon, },
		{ WC_EDIT, WS_VISIBLE|WS_CHILD|WS_TABSTOP|ES_NOHIDESEL|ES_AUTOHSCROLL|ES_AUTOVSCROLL|ES_MULTILINE|ES_WANTRETURN, WS_EX_CLIENTEDGE, 0, nullptr, GetSizeExplicit, },
		{ WC_EDIT, WS_VISIBLE|WS_CHILD|WS_TABSTOP|ES_NOHIDESEL|ES_AUTOHSCROLL|ES_AUTOVSCROLL|WS_VSCROLL|ES_MULTILINE|ES_WANTRETURN, WS_EX_CLIENTEDGE, 0, nullptr, GetSizeExplicit, },
		{ WC_EDIT, WS_VISIBLE|WS_CHILD|WS_TABSTOP|ES_NOHIDESEL|ES_AUTOHSCROLL|ES_RIGHT, WS_EX_CLIENTEDGE, false, OnCreateFloatBox, GetSizeExplicit, },
		{ UPDOWN_CLASS, WS_VISIBLE|WS_CHILD|WS_TABSTOP|UDS_ALIGNRIGHT|UDS_ARROWKEYS|UDS_HOTTRACK|UDS_NOTHOUSANDS|UDS_WRAP, 0,false, OnCreateFloatBoxSpinner, GetSizeExplicit, },
		{ WC_COMBOBOX, WS_VISIBLE|WS_CHILD|WS_TABSTOP|CBS_DROPDOWNLIST|CBS_HASSTRINGS, 0, false, OnCreateAddSubItems, GetSizeExplicit, },
		{ WC_COMBOBOX, WS_VISIBLE|WS_CHILD|WS_TABSTOP|CBS_DROPDOWN|CBS_HASSTRINGS, 0, false, OnCreateAddSubItems, GetSizeExplicit, },
		{ PROGRESS_CLASS, WS_VISIBLE|WS_CHILD, 0, true, nullptr, GetSizeExplicit, },
		{ PROGRESS_CLASS, WS_VISIBLE|WS_CHILD|PBS_MARQUEE, 0, true, OnCreateMarquee, GetSizeExplicit, },
		{ WC_TABCONTROL, WS_VISIBLE|WS_CHILD|WS_TABSTOP, WS_EX_CONTROLPARENT, true, OnCreateTab, GetSizeExplicit, },
		{ TRACKBAR_CLASS, WS_VISIBLE|WS_CHILD|WS_TABSTOP|TBS_NOTICKS, 0, true, nullptr, GetSizeExplicit, },
		{ TRACKBAR_CLASS, WS_VISIBLE|WS_CHILD|WS_TABSTOP|TBS_NOTICKS|TBS_ENABLESELRANGE, 0, true, nullptr, GetSizeExplicit, },
		{ TRACKBAR_CLASS, WS_VISIBLE|WS_CHILD|WS_TABSTOP, 0, true, nullptr, GetSizeExplicit, }, 
		{ WC_LISTBOX, WS_VISIBLE|WS_CHILD|WS_TABSTOP|WS_VSCROLL|WS_HSCROLL|LBS_NOTIFY|LBS_NOINTEGRALHEIGHT|LBS_HASSTRINGS|LBS_USETABSTOPS|WS_BORDER, 0, true, OnCreateAddSubItems, GetSizeExplicit, }, 
	};
	match_array_e(sDescs, control_type);
	return sDescs[(int)_Type];
}

HWND oWinControlCreate(const control_info& _Desc)
{
	if (!_Desc.parent || _Desc.type == control_type::unknown)
		oThrow(std::errc::invalid_argument, "");
	oWINVP((HWND)_Desc.parent);

	const CONTROL_CREATION_DESC& CCDesc = oWinControlGetCreationDesc(_Desc.type);
	int2 size = CCDesc.GetSize(_Desc);

	HWND hWnd = CreateWindowEx(
		CCDesc.dwExStyle
		, CCDesc.ClassName
		, (CCDesc.SetText && _Desc.text) ? _Desc.text : ""
		, CCDesc.dwStyle | (_Desc.starts_new_group ? WS_GROUP : 0)
		, _Desc.position.x
		, _Desc.position.y
		, size.x
		, size.y
		, (HWND)_Desc.parent
		, (HMENU)_Desc.id
		, nullptr
		, nullptr);

	oVB(hWnd);

	oWinControlSetFont(hWnd, (HFONT)_Desc.font);
	if (CCDesc.FinishCreation)
	{
		if (!CCDesc.FinishCreation(hWnd, _Desc))
		{
			DestroyWindow(hWnd);
			hWnd = nullptr;
		}
	}

	return hWnd;
}

bool oWinControlDefaultOnNotify(HWND _hControl, const NMHDR& _NotifyMessageHeader, LRESULT* _plResult, control_type _Type)
{
	oWIN_CHECK(_hControl);
	bool ShortCircuit = false;
	if (_Type == control_type::unknown)
		_Type = oWinControlGetType(_NotifyMessageHeader.hwndFrom);
	switch (_Type)
	{
		case control_type::floatbox_spinner:
			if (_NotifyMessageHeader.code == UDN_DELTAPOS)
			{
				HWND hBuddyFloatBox = oWinControlGetBuddy(_NotifyMessageHeader.hwndFrom);
				if (hBuddyFloatBox)
				{
					const NMUPDOWN& ud = (const NMUPDOWN&)_NotifyMessageHeader;
					float f = oWinControlGetFloat(hBuddyFloatBox);
					oWinControlSetValue(hBuddyFloatBox, f + 0.01f * ud.iDelta);
				}
			}
			ShortCircuit = true;
			break;

		case control_type::hyperlabel:
		{
			if (_NotifyMessageHeader.code == NM_CLICK || _NotifyMessageHeader.code == NM_RETURN)
			{
				const NMLINK& NMLink = (const NMLINK&)_NotifyMessageHeader;
				xlstring asMultiByte(NMLink.item.szUrl);
				system::spawn_associated_application(asMultiByte);
			}

			ShortCircuit = true;
			break;
		}

		default: break;
	}

	if (_plResult)
		*_plResult = FALSE;

	return ShortCircuit;
}

bool oWinControlIsVisible(HWND _hControl)
{
	return !!(GetWindowLongPtr(_hControl, GWL_STYLE) & WS_VISIBLE);
}

bool oWinControlSetVisible(HWND _hControl, bool _Visible)
{
	if (!ShowWindow(_hControl, _Visible ? SW_SHOWNA : SW_HIDE))
		oVB(RedrawWindow(_hControl, nullptr, nullptr, RDW_INVALIDATE|RDW_UPDATENOW));
	return true;
}

int oWinControlGetNumSubItems(HWND _hControl)
{
	control_type type = oWinControlGetType(_hControl);
	switch (type)
	{
		case control_type::combobox:
		case control_type::combotextbox: return ComboBox_GetCount(_hControl);
		case control_type::listbox: return ListBox_GetCount(_hControl);
		case control_type::tab: return TabCtrl_GetItemCount(_hControl);
		default: return 0;
	}
}

bool oWinControlClearSubItems(HWND _hControl)
{
	oWIN_CHECK(_hControl);
	control_type type = oWinControlGetType(_hControl);
	switch (type)
	{
		case control_type::combobox:
		case control_type::combotextbox:
			ComboBox_ResetContent(_hControl);
			return true;
		case control_type::listbox:
			ListBox_ResetContent(_hControl);
			return true;
		case control_type::tab:
			oVB(TabCtrl_DeleteAllItems(_hControl));
			return true;
		default:
				break;
	}

	return true;
}

int oWinControlInsertSubItem(HWND _hControl, const char* _SubItemText, int _SubItemIndex)
{
	oWIN_CHECK(_hControl);
	control_type type = oWinControlGetType(_hControl);
	switch (type)
	{
		case control_type::combobox:
		case control_type::combotextbox:
		{
			int index = CB_ERR;
			if (_SubItemIndex >= 0 && _SubItemIndex < ComboBox_GetCount(_hControl))
				index = ComboBox_InsertString(_hControl, _SubItemIndex, _SubItemText);
			else
				index = ComboBox_AddString(_hControl, _SubItemText);

			oCheck(index != CB_ERRSPACE, std::errc::no_buffer_space, "String is too large");
			return index;
		}

		case control_type::listbox:
		{
			int index = LB_ERR;
			if (_SubItemIndex >= 0 && _SubItemIndex < ListBox_GetCount(_hControl))
				index = ListBox_InsertString(_hControl, _SubItemIndex, _SubItemText);
			else
				index = ListBox_AddString(_hControl, _SubItemText);

			oCheck(index != LB_ERRSPACE, std::errc::no_buffer_space, "String is too large");
			return index;
		}

		case control_type::tab:
		{
			TCITEM item;
			item.mask = TCIF_TEXT;
			item.pszText = (LPSTR)_SubItemText;
			int count = TabCtrl_GetItemCount(_hControl);
			int index = TabCtrl_InsertItem(_hControl, ((_SubItemIndex >= 0) ? _SubItemIndex : count), &item);
			if (CB_ERR == index)
				oTHROW_BAD_INDEX(_hControl, type, _SubItemIndex);
			return index;
		}

		default:
			break;
	}

	oTHROW_BAD_TYPE(_hControl, type);
	//return invalid;
}

void oWinControlDeleteSubItem(HWND _hControl, const char* _SubItemText, int _SubItemIndex)
{
	oWIN_CHECK(_hControl);
	control_type type = oWinControlGetType(_hControl);
	switch (type)
	{
		case control_type::combobox:
		case control_type::combotextbox:
			if (_SubItemIndex >= 0)
				ComboBox_DeleteString(_hControl, _SubItemIndex);
			break;

		case control_type::listbox:
			if (_SubItemIndex >= 0)
				ListBox_DeleteString(_hControl, _SubItemIndex);
			break;

		case control_type::tab:
			TabCtrl_DeleteItem(_hControl, _SubItemIndex);
			break;

			default:
				break;
	}
}

bool oWinControlAddSubItems(HWND _hControl, const char* _DelimitedString, char _Delimiter)
{
	oWIN_CHECK(_hControl);
	char delim[2] = { _Delimiter, 0 };
	char* ctx = nullptr;
	std::string copy(_DelimitedString);
	const char* tok = strtok_r((char*)copy.c_str(), delim, &ctx);
	while (tok)
	{
		oWinControlInsertSubItem(_hControl, tok, -1);
		tok = strtok_r(nullptr, delim, &ctx);
	}
	return true;
}

int oWinControlFindSubItem(HWND _hControl, const char* _SubItemText)
{
	int index = CB_ERR;
	control_type type = oWinControlGetType(_hControl);
	switch (type)
	{
		case control_type::combobox:
		case control_type::combotextbox:
		{
			index = ComboBox_FindStringExact(_hControl, 0, _SubItemText);
			oCheck(index != CB_ERR, std::errc::invalid_argument, "Text %s was not found in %s %p (%d)", oSAFESTRN(_SubItemText), as_string(type), _hControl, GetDlgCtrlID(_hControl));
			break;
		}

		case control_type::listbox:
		{
			index = ListBox_FindStringExact(_hControl, 0, _SubItemText);
			oCheck(index != CB_ERR, std::errc::invalid_argument, "Text %s was not found in %s %p (%d)", oSAFESTRN(_SubItemText), as_string(type), _hControl, GetDlgCtrlID(_hControl));
			break;
		}

		case control_type::tab:
		{
			mstring text;
			TCITEM item;
			item.mask = TCIF_TEXT;
			item.pszText = (LPSTR)text.c_str();
			item.cchTextMax = static_cast<int>(text.capacity());
			const int kCount = TabCtrl_GetItemCount(_hControl);
			for (int i = 0; i < kCount; i++)
			{
				if (TabCtrl_GetItem(_hControl, i, &item))
				{
					if (!strcmp(text, _SubItemText))
					{
						index = i;
						break;
					}
				}
			}

			break;
		}

	default:
		oTHROW_BAD_TYPE(_hControl, type);
		break;
	}

	return index;
}

bool oWinControlSelectSubItem(HWND _hControl, int _SubItemIndex)
{
	control_type type = oWinControlGetType(_hControl);
	switch (type)
	{
		case control_type::combobox:
		case control_type::combotextbox:
			if (CB_ERR == ComboBox_SetCurSel(_hControl, _SubItemIndex))
				oTHROW_BAD_INDEX(_hControl, type, _SubItemIndex);
			break;

		case control_type::listbox:
			if (CB_ERR == ListBox_SetCurSel(_hControl, _SubItemIndex))
				oTHROW_BAD_INDEX(_hControl, type, _SubItemIndex);
			break;

		case control_type::tab:
		{
			HWND hParent = GetParent(_hControl);
			int index = TabCtrl_GetCurSel(_hControl);
			NMHDR nm;
			nm.hwndFrom = _hControl;
			nm.idFrom = GetDlgCtrlID(_hControl);
			if (-1 != index)
			{
				nm.code = TCN_SELCHANGING;
				SendMessage(hParent, WM_NOTIFY, nm.idFrom, (LPARAM)&nm);
			}

			index = TabCtrl_SetCurSel(_hControl, _SubItemIndex);
			if (-1 == index)
				oTHROW_BAD_INDEX(_hControl, type, _SubItemIndex);

			nm.code = TCN_SELCHANGE;
			SendMessage(hParent, WM_NOTIFY, nm.idFrom, (LPARAM)&nm);

			break;
		}
		default:
			oTHROW_BAD_TYPE(_hControl, type);
	}

	return true;
}

bool oWinControlSelectSubItemRelative(HWND _hControl, int _Offset)
{
	control_type type = oWinControlGetType(_hControl);
	int next = -1, count = 0;
	switch (type)
	{
		case control_type::combobox:
		case control_type::combotextbox:
			count = ComboBox_GetCount(_hControl);
			next = ComboBox_GetCurSel(_hControl) + _Offset;
			while (next < 0) next += count;
			while (next >= count) next -= count;
			if (CB_ERR == ComboBox_SetCurSel(_hControl, next))
				oTHROW_BAD_INDEX(_hControl, type, next);
			break;

		case control_type::listbox:
			count = ListBox_GetCount(_hControl);
			next = ListBox_GetCurSel(_hControl) + _Offset;
			while (next < 0) next += count;
			while (next >= count) next -= count;
			if (CB_ERR == ListBox_SetCurSel(_hControl, next))
				oTHROW_BAD_INDEX(_hControl, type, next);
			break;

		case control_type::tab:
			count = TabCtrl_GetItemCount(_hControl);
			next = TabCtrl_GetCurSel(_hControl) + _Offset;
			while (next < 0) next += count;
			while (next >= count) next -= count;
			if (-1 == TabCtrl_SetCurSel(_hControl, next))
				oTHROW_BAD_INDEX(_hControl, type, next);
			break;

		default:
			oTHROW_BAD_TYPE(_hControl, type);
	}

	return true;
}

int oWinControlGetSelectedSubItem(HWND _hControl)
{
	control_type type = oWinControlGetType(_hControl);
	switch (type)
	{
		case control_type::combobox:
		case control_type::combotextbox: return ComboBox_GetCurSel(_hControl);
		case control_type::listbox: return ListBox_GetCurSel(_hControl);
		case control_type::tab: return TabCtrl_GetCurSel(_hControl);
		default: break;
	}
	return -1;
}

control_type oWinControlGetType(HWND _hControl)
{
	mstring ClassName;
	if (!GetClassName(_hControl, ClassName, static_cast<int>(ClassName.capacity())))
		return control_type::unknown;
	
	if (!_stricmp("Button", ClassName))
	{
		LONG dwStyle = 0xff & GetWindowLong(_hControl, GWL_STYLE);
		switch (dwStyle)
		{
			case BS_GROUPBOX: return control_type::group;
			case BS_PUSHBUTTON: 
			case BS_DEFPUSHBUTTON: return control_type::button;
			case BS_AUTOCHECKBOX: return control_type::checkbox;
			case BS_AUTORADIOBUTTON: return control_type::radio;
			default: return control_type::unknown;
		}
	}

	else if (!_stricmp("Static", ClassName))
	{
		LONG dwStyle = 0xff & GetWindowLong(_hControl, GWL_STYLE);
		if (dwStyle & SS_ICON)
			return control_type::icon;
		else if (dwStyle & SS_CENTER)
			return control_type::label_centered;
		else if (dwStyle & SS_SIMPLE)
			return control_type::label;
		else
			return control_type::unknown;
	}

	else if (!_stricmp("Edit", ClassName))
	{
		DWORD dwStyle = 0xffff & (DWORD)GetWindowLong(_hControl, GWL_STYLE);
		if (dwStyle == (dwStyle & oWinControlGetCreationDesc(control_type::label_selectable).dwStyle))
			return control_type::label_selectable;

			DWORD_PTR data;
		if (GetWindowSubclass(_hControl, oSubclassProcFloatBox, oSubclassFloatBoxID, &data))
			return control_type::floatbox;

		else if (dwStyle == (dwStyle & oWinControlGetCreationDesc(control_type::textbox).dwStyle))
			return control_type::textbox;

		else
			return control_type::unknown;
	}

	else if (!_stricmp("ComboBox", ClassName))
	{
		LONG dwStyle = 0xf & GetWindowLong(_hControl, GWL_STYLE);
		switch (dwStyle)
		{
			case CBS_DROPDOWNLIST: return control_type::combobox;
			case CBS_DROPDOWN: return control_type::combotextbox;
			default: return control_type::unknown;
		}
	}

	else if (!_stricmp(oWinControlGetCreationDesc(control_type::floatbox_spinner).ClassName, ClassName))
		return control_type::floatbox_spinner;

	else if (!_stricmp(oWinControlGetCreationDesc(control_type::hyperlabel).ClassName, ClassName))
		return control_type::hyperlabel;

	else if (!_stricmp(oWinControlGetCreationDesc(control_type::tab).ClassName, ClassName))
	{
		// Don't identify a generic tab as an oGUI tab, only ones that
		// behave properly
		DWORD_PTR data;
		if (GetWindowSubclass(_hControl, oSubclassProcTab, oSubclassTabID, &data))
			return control_type::tab;
		return control_type::unknown;
	}
	else if (!_stricmp(oWinControlGetCreationDesc(control_type::progressbar).ClassName, ClassName))
	{
		DWORD dwStyle = (DWORD)GetWindowLong(_hControl, GWL_STYLE);
		if (dwStyle & PBS_MARQUEE)
			return control_type::progressbar_unknown;
		return control_type::progressbar;
	}
	else if (!_stricmp(oWinControlGetCreationDesc(control_type::slider).ClassName, ClassName))
	{
		DWORD dwStyle = (DWORD)GetWindowLong(_hControl, GWL_STYLE);

		if (dwStyle & TBS_ENABLESELRANGE)
			return control_type::slider_selectable;
		else if (dwStyle & TBS_NOTICKS)
			return control_type::slider;
		else
			return control_type::slider_with_ticks;
	}
	else if (!_stricmp(oWinControlGetCreationDesc(control_type::listbox).ClassName, ClassName))
		return control_type::listbox;

	return control_type::unknown;
}

int2 oWinControlGetInitialSize(control_type _Type, const int2& _Size)
{
	control_info d;
	d.type = _Type;
	d.size = _Size;
	return oWinControlGetCreationDesc(_Type).GetSize(d);
}

bool oWinControlIsTabStop(HWND _hControl)
{
	oWIN_CHECK(_hControl);
	return IsWindowEnabled(_hControl) && IsWindowVisible(_hControl) && ((GetWindowLong(_hControl, GWL_STYLE) & WS_TABSTOP) == WS_TABSTOP);
}

int oWinControlGetID(HWND _hControl)
{
	return GetDlgCtrlID(_hControl);
}

HWND oWinControlGetFromID(HWND _hParent, unsigned short _ID)
{
	return GetDlgItem(_hParent, _ID);
}

RECT oWinControlGetRect(HWND _hControl)
{
	RECT r;
	GetClientRect(_hControl, &r);
	HWND hParent = GetParent(_hControl);
	if (hParent)
		MapWindowPoints(_hControl, hParent, (LPPOINT)&r, 2);
	return r;
}

char* oWinControlGetSelectedText(char* _StrDestination, size_t _SizeofStrDestination, HWND _hControl)
{
	oWINVP(_hControl);
	control_type type = oWinControlGetType(_hControl);
	switch (type)
	{
		case control_type::floatbox_spinner:
			_hControl = oWinControlGetBuddy(_hControl);
			// pass through
		case control_type::textbox:
		case control_type::textbox_scrollable:
		case control_type::combotextbox:
		case control_type::floatbox:
		{
			uint start = 0, end = 0;
			SendMessage(_hControl, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);
			size_t len = end - start;
			oCheck(len < _SizeofStrDestination, std::errc::no_buffer_space, "Buffer too small to receive string");
			oVB(GetWindowText(_hControl, _StrDestination, int(_SizeofStrDestination)));

			if (start != end)
			{
				if (start)
					strlcpy(_StrDestination, _StrDestination + start, _SizeofStrDestination);
				_StrDestination[len] = 0;
			}

			return _StrDestination;
		}

		case control_type::listbox:
		{
			int index = ListBox_GetCurSel(_hControl);
			if (index != LB_ERR)
				return nullptr;

			size_t len = ListBox_GetTextLen(_hControl, index);
			oCheck(len < _SizeofStrDestination, std::errc::no_buffer_space, "Buffer too small to receive string");
			oCheck(LB_ERR == ListBox_GetText(_hControl, index, _StrDestination), std::errc::protocol_error, "GetText failed");
			return _StrDestination;
		}

		default:
			oTHROW_BAD_TYPE(_hControl, type);
	}
}

bool oWinControlSelect(HWND _hControl, int _Start, int _Length)
{
	control_type type = oWinControlGetType(_hControl);
	switch (type)
	{
		case control_type::floatbox_spinner:
			_hControl = oWinControlGetBuddy(_hControl);
			// pass thru
		case control_type::floatbox:
		case control_type::textbox:
		case control_type::textbox_scrollable:
		case control_type::combotextbox:
		case control_type::label_selectable:
			Edit_SetSel(_hControl, _Start, _Start+_Length);
			return true;
		case control_type::slider_selectable:
			if (_Length)
			{
				SendMessage(_hControl, TBM_SETSELSTART, FALSE, _Start);
				SendMessage(_hControl, TBM_SETSELEND, TRUE, _Start+_Length);
			}
			else
				SendMessage(_hControl, TBM_CLEARSEL, TRUE, 0);
			return true;
		default:
			oTHROW_BAD_TYPE(_hControl, type);
	}
}

bool oWinControlSetValue(HWND _hControl, float _Value)
{
	oWIN_CHECK(_hControl);
	mstring text;
	control_type type = oWinControlGetType(_hControl);
	switch (type)
	{
		case control_type::floatbox_spinner:
			_hControl = oWinControlGetBuddy(_hControl);
			// pass through
		default:
			snprintf(text, oSubclassFloatBoxFormat, _Value);
			oVB(SetWindowText(_hControl, text));
			return true;
	}
}

float oWinControlGetFloat(HWND _hControl)
{
	mstring text;
	control_type type = oWinControlGetType(_hControl);
	switch (type)
	{
		case control_type::floatbox_spinner:
			_hControl = oWinControlGetBuddy(_hControl);
			// pass through
		default: 
			GetWindowText(_hControl, text.c_str(), (int)text.capacity());
			break;
	}

	float f = 0.0f;
	if (!atof(text, &f))
		return std::numeric_limits<float>::quiet_NaN();
	return f;
}

void oWinControlSetIcon(HWND _hControl, HICON _hIcon, int _SubItemIndex)
{
	control_type type = oWinControlGetType(_hControl);
	switch (type)
	{
		case control_type::icon:
			if (_SubItemIndex != -1)
				oThrow(std::errc::invalid_argument, "Invalid _SubItemIndex");
			SendMessage(_hControl, STM_SETICON, (WPARAM)_hIcon, 0);
			break;
		default:
			oTHROW_BAD_TYPE(_hControl, type);
	}
}

HICON oWinControlGetIcon(HWND _hControl, int _SubItemIndex)
{
	control_type type = oWinControlGetType(_hControl);
	switch (type)
	{
		case control_type::icon:
			if (_SubItemIndex != -1)
			{
				oThrow(std::errc::invalid_argument, "invalid _SubItemIndex");
				//return nullptr;
			}
			return oWinGetIcon(_hControl);
		default:
			oTHROW_BAD_TYPE(_hControl, type);
	}
}

bool oWinControlIsChecked(HWND _hControl)
{
	control_type type = oWinControlGetType(_hControl);
	switch (type)
	{
		case control_type::checkbox:
		case control_type::radio:
		{
			LRESULT State = Button_GetState(_hControl);
			return (State & BST_CHECKED) == BST_CHECKED;
		}
		default:
			oTHROW_BAD_TYPE(_hControl, type);
	}
}

bool oWinControlSetChecked(HWND _hControl, bool _Checked)
{
	oWIN_CHECK(_hControl);
	control_type type = oWinControlGetType(_hControl);
	switch (type)
	{
		case control_type::checkbox:
		case control_type::radio:
			Button_SetCheck(_hControl, _Checked ? BST_CHECKED : BST_UNCHECKED);
	return true;
		default:
			oTHROW_BAD_TYPE(_hControl, type);
	}
}

bool oWinControlSetRange(HWND _hControl, int _Min, int _Max)
{
	control_type type = oWinControlGetType(_hControl);
	switch (type)
	{
		case control_type::slider:
		case control_type::slider_with_ticks:
		case control_type::slider_selectable:
			SendMessage(_hControl, TBM_SETRANGEMIN, TRUE, _Min);
			SendMessage(_hControl, TBM_SETRANGEMAX, TRUE, _Max);
			return true;
		case control_type::progressbar:
			SendMessage(_hControl, PBM_SETRANGE32, _Min, _Max);
			return true;
		default:
			oTHROW_BAD_TYPE(_hControl, type);
	}
}

bool oWinControlGetRange(HWND _hControl, int* _pMin, int* _pMax)
{
	control_type type = oWinControlGetType(_hControl);
	switch (type)
	{
		case control_type::slider:
		case control_type::slider_with_ticks:
		case control_type::slider_selectable:
			if (_pMin) *_pMin = (int)SendMessage(_hControl, TBM_GETRANGEMIN, 0, 0);
			if (_pMax) *_pMax = (int)SendMessage(_hControl, TBM_GETRANGEMAX, 0, 0);
			return true;
		case control_type::progressbar:
		{
			PBRANGE pbr;
			SendMessage(_hControl, PBM_GETRANGE, 0, (LPARAM)&pbr);
			if (_pMin) *_pMin = pbr.iLow;
			if (_pMax) *_pMax = pbr.iHigh;
		}
		default:
			oTHROW_BAD_TYPE(_hControl, type);
	}
}

bool oWinControlSetRangePosition(HWND _hControl, int _Position, bool _bNotify /*=true*/)
{
	control_type type = oWinControlGetType(_hControl);
	switch (type)
	{
		case control_type::slider:
		case control_type::slider_with_ticks:
		case control_type::slider_selectable:
			SendMessage(_hControl, _bNotify ? TBM_SETPOSNOTIFY : TBM_SETPOS, 0, _Position);
			return true;
		case control_type::progressbar:
			SendMessage(_hControl, PBM_SETPOS, _Position, 0);
			return true;
		default:
			oTHROW_BAD_TYPE(_hControl, type);
	}
}

int oWinControlGetRangePosition(HWND _hControl)
{
	control_type type = oWinControlGetType(_hControl);
	switch (type)
	{
		case control_type::slider:
		case control_type::slider_with_ticks:
		case control_type::slider_selectable: return (int)SendMessage(_hControl, TBM_GETPOS, 0, 0);
		case control_type::progressbar: return (int)SendMessage(_hControl, PBM_GETPOS, 0, 0);
		default: oTHROW_BAD_TYPE(_hControl, type);
	}
}

bool oWinControlSetTick(HWND _hControl, int _Position)
{
	control_type type = oWinControlGetType(_hControl);
	switch(type)
	{
	case control_type::slider_with_ticks:
		SendMessage(_hControl, TBM_SETTIC, 0, _Position);
		return true;
	default:
		oTHROW_BAD_TYPE(_hControl, type);
	}
}

bool oWinControlClearTicks(HWND _hControl)
{
	control_type type = oWinControlGetType(_hControl);
	switch(type)
	{
	case control_type::slider_with_ticks:
		SendMessage(_hControl, TBM_CLEARTICS, 1, 0);
		return true;
	default:
		oTHROW_BAD_TYPE(_hControl, type);
	}
}

bool oWinControlSetErrorState(HWND _hControl, bool _InErrorState)
{
	control_type type = oWinControlGetType(_hControl);
	switch (type)
	{
		case control_type::progressbar:
			SendMessage(_hControl, PBM_SETSTATE, _InErrorState ? PBST_ERROR : PBST_NORMAL, 0);
			return true;
		default:
			oTHROW_BAD_TYPE(_hControl, type);
	}
}

bool oWinControlGetErrorState(HWND _hControl)
{
	control_type type = oWinControlGetType(_hControl);
	switch (type)
	{
		case control_type::progressbar:
			return PBST_ERROR == SendMessage(_hControl, PBM_GETSTATE, 0, 0);
		default:
			oTHROW_BAD_TYPE(_hControl, type);
	}
}

bool oWinControlClampPositionToSelected(HWND _hControl)
{
	control_type type = oWinControlGetType(_hControl);
	switch (type)
	{
		case control_type::slider_selectable:
		{
			int Position = (int)SendMessage(_hControl, TBM_GETPOS, 0, 0);
			int SelectionStart = (int)SendMessage(_hControl, TBM_GETSELSTART, 0, 0);
			int SelectionEnd = (int)SendMessage(_hControl, TBM_GETSELEND, 0, 0);
			if (Position < SelectionStart)
				SendMessage(_hControl, TBM_SETPOS, TRUE, SelectionStart);
			else if (Position > SelectionEnd)
				SendMessage(_hControl, TBM_SETPOS, TRUE, SelectionEnd);
		}

		default:
			oTHROW_BAD_TYPE(_hControl, type);
	}
}

void oGDIScreenCaptureWindow(HWND _hWnd, const RECT* _pRect, void* _pImageBuffer, size_t _SizeofImageBuffer, BITMAPINFO* _pBitmapInfo, bool _RedrawWindow, bool _FlipV)
{
	RECT r;
	if (_pRect)
	{
		// Find offset into client area
		POINT p = {0,0};
		ClientToScreen(_hWnd, &p);

		RECT wr;
		oVB(GetWindowRect(_hWnd, &wr));
		p.x -= wr.left;
		p.y -= wr.top;

		r.left = _pRect->left + p.x;
		r.right = _pRect->right + p.x;
		r.top = _pRect->top + p.y;
		r.bottom = _pRect->bottom + p.y;
	}

	else
	{
		RECT wr;
		oVB(GetWindowRect(_hWnd, &wr));
		r.left = 0;
		r.top = 0;
		r.right = oWinRectW(wr);
		r.bottom = oWinRectH(wr);
	}

	int2 size = oWinRectSize(r);

	if (size.x == 0 || size.y == 0)
		oThrow(std::errc::invalid_argument, "invalid size");

	WORD BitDepth = 0;
	{
		ouro::display::info di = ouro::display::get_info(oWinGetDisplayId(_hWnd));
		BitDepth = static_cast<WORD>(di.mode.depth);
		if (BitDepth == 32) BitDepth = 24;
	}

	if (!_pBitmapInfo)
		oThrow(std::errc::invalid_argument, "");
	const int BytesPerPixel = BitDepth / 8;
	const int AlignedWidth = align(size.x, 4);
	const int RowPitch = AlignedWidth * BytesPerPixel;

	memset(_pBitmapInfo, 0, sizeof(BITMAPINFO));
	_pBitmapInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	_pBitmapInfo->bmiHeader.biWidth = size.x;
	_pBitmapInfo->bmiHeader.biHeight = size.y;
	_pBitmapInfo->bmiHeader.biPlanes = 1;
	_pBitmapInfo->bmiHeader.biBitCount = BitDepth;
	_pBitmapInfo->bmiHeader.biCompression = BI_RGB;
	_pBitmapInfo->bmiHeader.biSizeImage = RowPitch * size.y;

	if (_pImageBuffer)
	{
		oCheck(_SizeofImageBuffer >= _pBitmapInfo->bmiHeader.biSizeImage, std::errc::no_buffer_space, "");

		HDC hDC = GetWindowDC(_hWnd);
		HDC hMemDC = CreateCompatibleDC(hDC);

		HBITMAP hBMP = CreateCompatibleBitmap(hDC, size.x, size.y);
		HBITMAP hOld = (HBITMAP)SelectObject(hMemDC, hBMP);
		if (_RedrawWindow)
			oVB(RedrawWindow(_hWnd, 0, 0, RDW_INVALIDATE|RDW_UPDATENOW));

		BitBlt(hMemDC, 0, 0, size.x, size.y, hDC, r.left, r.top, SRCCOPY);

		const int PadSize = AlignedWidth - size.x;
		for (int y = 0; y < size.y; y++)
		{
			GetDIBits(hMemDC, hBMP, _FlipV ? (size.y - 1 - y) : y, 1, (char*)_pImageBuffer + y * RowPitch, _pBitmapInfo, DIB_RGB_COLORS);
			char* pad = (char*)_pImageBuffer + (y*RowPitch) + (size.x*BytesPerPixel);
			switch (PadSize)
			{
				// Duff's device
				case 3: pad[2] = 0;
				case 2: pad[1] = 0;
				case 1: pad[0] = 0;
				default: break;
			}
		}

		SelectObject(hMemDC, hOld);
		DeleteDC(hMemDC);
		ReleaseDC(0, hDC);
	}
}

void oGDIScreenCaptureWindow(HWND _hWnd, bool _IncludeBorder, std::function<void*(size_t _Size)> _Allocate, void** _ppBuffer, size_t* _pBufferSize, bool _RedrawWindow, bool _FlipV)
{
	if (!_Allocate || !_ppBuffer || !_pBufferSize)
		oThrow(std::errc::invalid_argument, "");

	*_ppBuffer = nullptr;
	*_pBufferSize = 0;

	// Create a bmp in memory
	RECT r;
	GetClientRect(_hWnd, &r);
	BITMAPINFO bmi;

	RECT* pRect = &r;
	if (_IncludeBorder)
		pRect = nullptr;

	oGDIScreenCaptureWindow(_hWnd, pRect, nullptr, 0, &bmi, _RedrawWindow, _FlipV);
	BITMAPFILEHEADER bmfh;
	memset(&bmfh, 0, sizeof(bmfh));
	bmfh.bfType = 'MB';
	bmfh.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFO) + bmi.bmiHeader.biSizeImage;
	bmfh.bfOffBits = static_cast<DWORD>(sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFO));

	*_pBufferSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFO) + bmi.bmiHeader.biSizeImage;
	*_ppBuffer = _Allocate(*_pBufferSize);
	memcpy(*_ppBuffer, &bmfh, sizeof(bmfh));
	memcpy((uint8_t*)*_ppBuffer + sizeof(bmfh), &bmi, sizeof(bmi));
	oGDIScreenCaptureWindow(_hWnd, pRect, (uint8_t*)*_ppBuffer + sizeof(bmfh) + sizeof(bmi), bmi.bmiHeader.biSizeImage, &bmi, _RedrawWindow, _FlipV);
}
