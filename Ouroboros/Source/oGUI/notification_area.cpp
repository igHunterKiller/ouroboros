// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oGUI/notification_area.h>
#include <oGUI/windows/oWinWindowing.h>
#include <oCore/assert.h>
#include <oCore/countof.h>

#include <oCore/algorithm.h>

#include <oSystem/reporting.h>
#include <oSystem/windows/win_error.h>
#include <oSystem/windows/win_version.h>
// Because this can call a timeout value that can occur after a synchronous 
// report leaks, ensure we wait for it before reporting a false positive...
#include <oSystem/windows/win_crt_leak_tracker.h>

#include <shellapi.h>
#include <windowsx.h>
#include <commctrl.h>

#if (defined(NTDDI_WIN7) && (NTDDI_VERSION >= NTDDI_WIN7))
	#define oWINDOWS_HAS_TRAY_NOTIFYICONIDENTIFIER
	#define oWINDOWS_HAS_TRAY_QUIETTIME
#endif

using namespace std;

namespace ouro {
	namespace notification_area {

struct REMOVE_TRAY_ICON
{
	ouro::window_handle hWnd;
	unsigned int ID;
	unsigned int TimeoutMS;
};

bool operator==(const REMOVE_TRAY_ICON& _RTI1, const REMOVE_TRAY_ICON& _RTI2) { return _RTI1.hWnd == _RTI2.hWnd && _RTI1.ID == _RTI2.ID; }

class cleanup
{
public:

	static cleanup& singleton();

	void register_window(ouro::window_handle _hWnd, UINT _ID)
	{
		lock_guard<shared_mutex> lock(Mutex);
		if (!AllowInteraction)
			return;

		if (Removes.size() < Removes.capacity())
		{
			REMOVE_TRAY_ICON rti;
			rti.hWnd = _hWnd;
			rti.ID = _ID;
			rti.TimeoutMS = 0;
			Removes.push_back(rti);
		}
		else
			OutputDebugStringA("--- Too many tray icons registered for cleanup: ignoring. ---");
	}

	void unregister_window(ouro::window_handle _hWnd, UINT _ID)
	{
		lock_guard<shared_mutex> lock(Mutex);
		if (!AllowInteraction)
			return;

		REMOVE_TRAY_ICON rti;
		rti.hWnd = _hWnd;
		rti.ID = _ID;
		rti.TimeoutMS = 0;

		find_and_erase(Removes, rti);
	}

	void register_thread(thread&& _DeferredHideIconThread)
	{
		lock_guard<shared_mutex> lock(Mutex);
		DeferredHideIconThreads.push_back(std::move(_DeferredHideIconThread));
	}

	void unregister_thread(const thread::id& _DeferredHideIconThreadID)
	{
		lock_guard<shared_mutex> lock(Mutex);
		for (auto& t : DeferredHideIconThreads)
		{
			if (_DeferredHideIconThreadID == t.get_id())
			{
				t = thread();
				break;
			}
		}
	}

private:
	cleanup()
		: AllowInteraction(true)
	{
		reporting::ensure_initialized();
	}

	~cleanup()
	{
		{
			lock_guard<shared_mutex> lock(Mutex);
			AllowInteraction = false;
		}

		for (auto& t : DeferredHideIconThreads)
			t.join();

		if (!Removes.empty())
		{
			xlstring buf;
			mstring exec;
			oTRACE("Cleaning up tray icons");
		}

		for (size_t i = 0; i < Removes.size(); i++)
			show_icon(Removes[i].hWnd, Removes[i].ID, 0, 0, false);

		Removes.clear();
	}

	fixed_vector<REMOVE_TRAY_ICON, 20> Removes;
	fixed_vector<thread, 20> DeferredHideIconThreads;
	shared_mutex Mutex;
	volatile bool AllowInteraction;
};

oDEFINE_PROCESS_SINGLETON("ouro::notification_area::cleanup", cleanup);

ouro::window_handle native_handle()
{
	static const char* sHierarchy[] = { "Shell_TrayWnd", "TrayNotifyWnd", "SysPager", "ToolbarWindow32", };
	size_t i = 0;
	HWND hWnd = FindWindow(sHierarchy[i++], nullptr);
	while (hWnd && i < countof(sHierarchy))
		hWnd = FindWindowEx(hWnd, nullptr, sHierarchy[i++], nullptr);
	return (ouro::window_handle)hWnd;
}

void focus()
{
	oV(Shell_NotifyIcon(NIM_SETFOCUS, nullptr));
}

#ifndef oWINDOWS_HAS_TRAY_NOTIFYICONIDENTIFIER
static bool Shell_NotifyIconGetRect_workaround(HWND _hWnd, UINT _ID, RECT* _pRect)
{
	// http://social.msdn.microsoft.com/forums/en-US/winforms/thread/4ac8d81e-f281-4b32-9407-e663e6c234ae/
	
	HWND hTray = (HWND)native_handle();
	DWORD TrayProcID;
	GetWindowThreadProcessId(hTray, &TrayProcID);
	HANDLE hTrayProc = OpenProcess(PROCESS_ALL_ACCESS, 0, TrayProcID);
	bool success = false;
	if (!hTrayProc)
	{
		TBBUTTON* lpBI = (TBBUTTON*)VirtualAllocEx(hTrayProc, nullptr, sizeof(TBBUTTON), MEM_COMMIT, PAGE_READWRITE);
		int nButtons = (int)SendMessage(hTray, TB_BUTTONCOUNT, 0, 0);
		for (int i = 0; i < nButtons; i++)
		{
			if (SendMessage(hTray, TB_GETBUTTON, i, (LPARAM)lpBI))
			{
				TBBUTTON bi;
				DWORD extraData[2];
				ReadProcessMemory(hTrayProc, lpBI, &bi, sizeof(TBBUTTON), nullptr);
				ReadProcessMemory(hTrayProc, (LPCVOID)bi.dwData, extraData, sizeof(DWORD) * 2, nullptr);
				HWND IconNotifiesThisHwnd = (HWND)extraData[0];
				UINT IconID = extraData[1];

				if (_hWnd == IconNotifiesThisHwnd && _ID == IconID)
				{
					RECT r;
					RECT* lpRect = (RECT*)VirtualAllocEx(hTrayProc, nullptr, sizeof(RECT), MEM_COMMIT, PAGE_READWRITE);
					SendMessage(hTray, TB_GETITEMRECT, i, (LPARAM)lpRect);
					ReadProcessMemory(hTrayProc, lpRect, &r, sizeof(RECT), nullptr);
					VirtualFreeEx(hTrayProc, lpRect, 0, MEM_RELEASE);
					MapWindowPoints(hTray, nullptr, (LPPOINT)&r, 2);
					success = true;
					break;
				}
			}
		}

		VirtualFreeEx(hTrayProc, lpBI, 0, MEM_RELEASE);
		CloseHandle(hTrayProc);
	}

	return success;
}
#endif

// returns true if the out params are valid, or false if _hWnd, _ID not found.
static bool icon_rect_internal(ouro::window_handle _hWnd, unsigned int _ID, RECT* _pRect)
{
	#ifdef oWINDOWS_HAS_TRAY_NOTIFYICONIDENTIFIER
		NOTIFYICONIDENTIFIER nii;
		memset(&nii, 0, sizeof(nii));
		nii.cbSize = sizeof(nii);
		nii.hWnd = (HWND)_hWnd;
		nii.uID = _ID;
		if (FAILED(Shell_NotifyIconGetRect(&nii, _pRect)))
			return false;
	#else
	if (!Shell_NotifyIconGetRect_workaround((HWND)_hWnd, _ID, _pRect))
		return false;
	#endif
	return true;
}

void icon_rect(ouro::window_handle _hWnd, unsigned int _ID, int* _pX, int* _pY, int* _pWidth, int* _pHeight)
{
	RECT r;
	if (!icon_rect_internal(_hWnd, _ID, &r))
		throw std::system_error(std::errc::no_such_device, std::system_category());
	*_pX = r.left;
	*_pY = r.top;
	*_pWidth = r.right - r.left;
	*_pHeight = r.bottom - r.top;
}

bool exists(ouro::window_handle _hWnd, unsigned int _ID)
{
	RECT r;
	return icon_rect_internal(_hWnd, _ID, &r);
}

static NOTIFYICONDATA init_basics(ouro::window_handle _hWnd, unsigned int _ID, unsigned int _CallbackMessage, ouro::icon_handle _hIcon)
{
	NOTIFYICONDATA nid;
	memset(&nid, 0, sizeof(nid));
	nid.cbSize = sizeof(nid);
	nid.hWnd = (HWND)_hWnd;
	nid.hIcon = _hIcon ? (HICON)_hIcon : oWinGetIcon((HWND)_hWnd, false);
	if (!nid.hIcon)
		nid.hIcon = oWinGetIcon((HWND)_hWnd, true);
	nid.uID = _ID;
	nid.uCallbackMessage = _CallbackMessage;
	nid.uFlags |= (_CallbackMessage ? NIF_MESSAGE : 0);
	return nid;
}

void show_icon(ouro::window_handle _hWnd, unsigned int _ID, unsigned int _CallbackMessage, ouro::icon_handle _hIcon, bool _Show)
{
	NOTIFYICONDATA nid = init_basics(_hWnd, _ID, _CallbackMessage, _hIcon);
	nid.uFlags |= NIF_ICON;
	nid.uVersion = NOTIFYICON_VERSION_4;
	oV(Shell_NotifyIcon(_Show ? NIM_ADD : NIM_DELETE, &nid));

	// Ensure we know exactly what version behavior we're dealing with
	oV(Shell_NotifyIcon(NIM_SETVERSION, &nid));
	cleanup::singleton().register_window(_hWnd, _ID);
}

static void hide_icon(ouro::window_handle _hWnd, unsigned int _ID, unsigned int _TimeoutMS)
{
	Sleep(_TimeoutMS);
	oTRACE("Auto-closing tray icon HWND=0x%p ID=%u", _hWnd, _ID);
	show_icon(_hWnd, _ID, 0, 0, false);
	cleanup::singleton().unregister_thread(this_thread::get_id());
	windows::crt_leak_tracker::release_delay();
}

static void schedule_icon_hide(ouro::window_handle _hWnd, unsigned int _ID, unsigned int _TimeoutMS)
{
	windows::crt_leak_tracker::add_delay();
	cleanup::singleton().register_thread(std::move(thread(hide_icon, _hWnd, _ID, _TimeoutMS)));
}

void show_message(ouro::window_handle _hWnd, unsigned int _ID, ouro::icon_handle _hIcon, unsigned int _TimeoutMS, const char* _Title, const char* _Message)
{
	NOTIFYICONDATA nid = init_basics(_hWnd, _ID, 0, _hIcon);
	nid.uFlags |= NIF_INFO;
	nid.uTimeout = __max(__min(_TimeoutMS, 30000), 10000);

	// MS recommends truncating at 200 for English: http://msdn.microsoft.com/en-us/library/bb773352(v=vs.85).aspx
	static const int MaxInfo = 201;
	strncpy(nid.szInfo, MaxInfo, _Message, MaxInfo - 1);
	ellipsize(nid.szInfo, MaxInfo);

	// MS recommends truncating at 48 for English: http://msdn.microsoft.com/en-us/library/bb773352(v=vs.85).aspx
	static const int MaxTitle = 49;
	strncpy(nid.szInfoTitle, MaxTitle, _Title, MaxTitle - 1);

	nid.dwInfoFlags = NIIF_NOSOUND;

	#ifdef oWINDOWS_HAS_TRAY_QUIETTIME
		nid.dwInfoFlags |= NIIF_RESPECT_QUIET_TIME;
	#endif

	RECT r;
	if (!icon_rect_internal(_hWnd, _ID, &r))
	{
		UINT timeout = 0;
		switch (windows::get_version())
		{
			case windows::version::win2000:
			case windows::version::xp:
			case windows::version::server_2003:
				timeout = nid.uTimeout;
				break;
			default:
			{
				ULONG duration = 0;
				oV(SystemParametersInfo(SPI_GETMESSAGEDURATION, 0, &duration, 0));
				timeout = (UINT)duration * 1000;
				break;
			};
		}

		show_icon(_hWnd, _ID, 0, _hIcon, true);
		if (timeout != ~0u)
			schedule_icon_hide(_hWnd, _ID, timeout);
	}

	oVB(Shell_NotifyIcon(NIM_MODIFY, &nid));
}

// _ToSysTray false means animate from sys tray out to window position
static void animate_window_respectful(ouro::window_handle _hWnd, bool _ToSysTray)
{
	RECT rDesktop, rWindow;
	GetWindowRect(GetDesktopWindow(), &rDesktop);
	GetWindowRect((HWND)_hWnd, &rWindow);
	rDesktop.left = rDesktop.right;
	rDesktop.top = rDesktop.bottom;
	const RECT& from = _ToSysTray ? rWindow : rDesktop;
	const RECT& to = _ToSysTray ? rDesktop : rWindow;
	oWinAnimate((HWND)_hWnd, from, to);
}

void minimize(ouro::window_handle _hWnd, unsigned int _CallbackMessage, ouro::icon_handle _hIcon)
{
	animate_window_respectful(_hWnd, true);
	ShowWindow((HWND)_hWnd, SW_HIDE);
	show_icon(_hWnd, 0, _CallbackMessage, _hIcon, true);
}

void restore(ouro::window_handle _hWnd)
{
	animate_window_respectful(_hWnd, false);
	ShowWindow((HWND)_hWnd, SW_SHOW);
	SetActiveWindow((HWND)_hWnd);
	SetForegroundWindow((HWND)_hWnd);
	show_icon(_hWnd, 0, 0, 0, false);
}

void decode_callback_message_params(uintptr_t _wParam, uintptr_t _lParam, unsigned int* _pNotificationEvent, unsigned int* _pID, int* _pX, int* _pY)
{
	// http://msdn.microsoft.com/en-us/library/bb773352(v=vs.85).aspx
	// Search for uCallbackMessage

	*_pNotificationEvent = LOWORD(_lParam);
	*_pID = HIWORD(_lParam);
	if (_pX)
		*_pX = GET_X_LPARAM(_wParam);
	if (_pY)
		*_pY = GET_Y_LPARAM(_wParam);
}

	} // namespace notification_area
}
