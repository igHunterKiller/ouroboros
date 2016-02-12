// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oCore/countof.h>
#include <oGUI/msgbox.h>
#include <oGUI/cursor.h>
#include <oGUI/notification_area.h>
#include <oGUI/Windows/oWinDialog.h>
#include <oGUI/Windows/oWinRect.h>
#include <oGUI/Windows/oWinWindowing.h>
#include <oSystem/windows/win_error.h>

// Secret function that is not normally exposed in headers.
// Typically pass 0 for wLanguageId, and specify a timeout
// for the dialog in milliseconds, returns MB_TIMEDOUT if
// the timeout is reached.
int MessageBoxTimeoutA(IN HWND hWnd, IN LPCSTR lpText, IN LPCSTR lpCaption, IN UINT uType, IN WORD wLanguageId, IN DWORD dwMilliseconds);
int MessageBoxTimeoutW(IN HWND hWnd, IN LPCWSTR lpText, IN LPCWSTR lpCaption, IN UINT uType, IN WORD wLanguageId, IN DWORD dwMilliseconds);

#ifdef UNICODE
	#define MessageBoxTimeout MessageBoxTimeoutW
#else
	#define MessageBoxTimeout MessageBoxTimeoutA
#endif 

#define MB_TIMEDOUT 32000

// Link to MessageBoxTimeout based on code from:
// http://www.codeproject.com/KB/cpp/MessageBoxTimeout.aspx

//Functions & other definitions required-->
typedef int (__stdcall *MSGBOXAAPI)(IN HWND hWnd, 
        IN LPCSTR lpText, IN LPCSTR lpCaption, 
        IN UINT uType, IN WORD wLanguageId, IN DWORD dwMilliseconds);
typedef int (__stdcall *MSGBOXWAPI)(IN HWND hWnd, 
        IN LPCWSTR lpText, IN LPCWSTR lpCaption, 
        IN UINT uType, IN WORD wLanguageId, IN DWORD dwMilliseconds);

int MessageBoxTimeoutA(HWND hWnd, LPCSTR lpText, 
    LPCSTR lpCaption, UINT uType, WORD wLanguageId, 
    DWORD dwMilliseconds)
{
    static MSGBOXAAPI MsgBoxTOA = nullptr;

    if (!MsgBoxTOA)
    {
        HMODULE hUser32 = GetModuleHandle("user32.dll");
        if (hUser32)
        {
            MsgBoxTOA = (MSGBOXAAPI)GetProcAddress(hUser32, 
                                      "MessageBoxTimeoutA");
            //fall through to 'if (MsgBoxTOA)...'
        }
        else
        {
            //stuff happened, add code to handle it here 
            //(possibly just call MessageBox())
            return 0;
        }
    }

    if (MsgBoxTOA)
    {
        return MsgBoxTOA(hWnd, lpText, lpCaption, 
              uType, wLanguageId, dwMilliseconds);
    }

    return 0;
}

int MessageBoxTimeoutW(HWND hWnd, LPCWSTR lpText, 
    LPCWSTR lpCaption, UINT uType, WORD wLanguageId, DWORD dwMilliseconds)
{
    static MSGBOXWAPI MsgBoxTOW = nullptr;

    if (!MsgBoxTOW)
    {
        HMODULE hUser32 = GetModuleHandle("user32.dll");
        if (hUser32)
        {
            MsgBoxTOW = (MSGBOXWAPI)GetProcAddress(hUser32, 
                                      "MessageBoxTimeoutW");
            //fall through to 'if (MsgBoxTOW)...'
        }
        else
        {
            //stuff happened, add code to handle it here 
            //(possibly just call MessageBox())
            return 0;
        }
    }

    if (MsgBoxTOW)
    {
        return MsgBoxTOW(hWnd, lpText, lpCaption, 
               uType, wLanguageId, dwMilliseconds);
    }

    return 0;
}

namespace ouro {

static msg_result as_action(WORD _ID)
{
	switch (_ID)
	{
		case IDCANCEL: return msg_result::abort;
		case IDRETRY: return msg_result::debug;
		default: 
		case IDCONTINUE: return msg_result::ignore;
		case IDIGNORE: return msg_result::ignore_always;
	}
}

static UINT as_flags(msg_type type)
{
	switch (type)
	{
		default:
		case msg_type::info: return MB_ICONINFORMATION|MB_OK;
		case msg_type::warn: return MB_ICONWARNING|MB_OK;
		case msg_type::yesno: return MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON1;
		case msg_type::error: return MB_ICONERROR|MB_OK|MB_TASKMODAL;
		case msg_type::debug: return MB_ICONERROR|MB_ABORTRETRYIGNORE;
	}
}

static msg_result get_result(int _MessageBoxResult)
{
	switch (_MessageBoxResult)
	{
		default:
		case IDOK: return msg_result::yes;
		case IDABORT: return msg_result::abort;
		case IDIGNORE: return msg_result::ignore; 
		case IDRETRY: return msg_result::debug;
		case IDYES: return msg_result::yes;
		case IDNO: return msg_result::no;
	}
}

static HICON load_icon(msg_type _Type)
{
	static LPCSTR map[] =
	{
		IDI_INFORMATION,
		IDI_WARNING,
		IDI_QUESTION,
		IDI_ERROR,
		IDI_ERROR,
		IDI_INFORMATION,
		IDI_INFORMATION,
		IDI_WARNING,
		IDI_ERROR,
	};
	match_array_e(map, msg_type);
	return LoadIconA(0, map[(int)_Type]);
}

#define IDMESSAGE 20
#define IDCOPYTOCLIPBOARD 21
#define IDICON 22


static VOID CALLBACK WndDialogTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	switch (idEvent)
	{
		case 1:
			EndDialog(hwnd, IDCANCEL);
			break;
		case 2:
			EnableWindow(GetDlgItem(hwnd, IDCONTINUE), TRUE);
			EnableWindow(GetDlgItem(hwnd, IDIGNORE), TRUE);
			KillTimer(hwnd, idEvent);
			break;
	}
}

struct DLGAssert
{
	DLGAssert(HICON _hIcon, unsigned int _TimeoutMS, unsigned int _DisableTimeoutMS)
		: hIcon(_hIcon)
		, TimeoutMS(static_cast<UINT>(_TimeoutMS))
		, DisableTimeoutMS(static_cast<UINT>(_DisableTimeoutMS))
		, hBGBrush(nullptr/*CreateSolidBrush(RGB(255,255,255))*/) // I want to make the top part of the dialog white and bottom gray like in Win7, but I can't change the BG color of a read-only editbox since the WM_CTLCOLOREDIT doesn't get sent
	{}

	~DLGAssert()
	{
		DeleteObject(hBGBrush);
	}

	oDECLARE_DLGPROC(, WndProc);
	oDECLARE_DLGPROC(static, StaticWndProc);

	HICON hIcon;
	UINT TimeoutMS;
	UINT DisableTimeoutMS;
	HBRUSH hBGBrush;
};

oDEFINE_DLGPROC(DLGAssert, StaticWndProc);

INT_PTR CALLBACK DLGAssert::WndProc(HWND _hWnd, UINT _uMsg, WPARAM _wParam, LPARAM _lParam)
{
	//char desc[512];
	//oGetWMDesc(desc, _hWnd, _uMsg, _wParam, _lParam);
	//oTRACE("%s", desc);

	switch (_uMsg)
	{
		case WM_INITDIALOG:
		{
			if (TimeoutMS != ~0u)
				SetTimer(_hWnd, 1, TimeoutMS, WndDialogTimerProc);
			if (DisableTimeoutMS != 0 && DisableTimeoutMS != ~0u)
				SetTimer(_hWnd, 2, DisableTimeoutMS, WndDialogTimerProc);
			SendDlgItemMessage(_hWnd, IDICON, STM_SETICON, (WPARAM)hIcon, 0);
			return false;
		}

		case WM_SETCURSOR:
			while (ShowCursor(true) < 0) {}
			SetCursor(LoadCursor(nullptr, IDC_ARROW));
			return true;

		case WM_COMMAND:
			switch (_wParam)
			{
				case IDCANCEL:
				case IDRETRY:
				case IDCONTINUE:
				case IDIGNORE:
					EndDialog(_hWnd, _wParam);
					return true;
				case IDCOPYTOCLIPBOARD:
					SendDlgItemMessage(_hWnd, IDMESSAGE, EM_SETSEL, 0, -1);
					SendDlgItemMessage(_hWnd, IDMESSAGE, WM_COPY, 0, 0);
					return true;
				default:
					break;
			}

		//case WM_CTLCOLORDLG:
		//	return (INT_PTR)hBGBrush;

		//case WM_CTLCOLOREDIT:
		//{
		//	if (HWND(_lParam) == GetDlgItem(_hWnd, IDMESSAGE))
		//	{
		//		SetBkMode((HDC)_wParam, TRANSPARENT);
		//		return (INT_PTR)hBGBrush;
		//	}
		//	break;
		//}

		//case WM_CTLCOLORBTN:
		//	break;

		//case WM_CTLCOLORSTATIC:
		//	break;

		default:
			break;
	}

	return false;
}

static void calc_string_rect(RECT& rString, const char* string, LONG minW, LONG minH, LONG maxW, LONG maxH)
{
	rString.left = 0;
	rString.top = 0;
	rString.right = 1;
	rString.bottom = 1;
	HDC hDC = CreateCompatibleDC(0);
	DrawTextA(hDC, string, -1, &rString, DT_CALCRECT|DT_LEFT|DT_NOPREFIX|DT_WORDBREAK);
	DeleteDC(hDC);
	// use a fudge-factor for the not-so-nice numbers that come out of DrawText
	rString.bottom = __min(maxH, __max(minH, rString.bottom * 3/4));
	rString.right = __min(maxW, __max(minW, rString.right * 3/4));

	if (rString.bottom == maxH)
		rString.right = maxW;
}

msg_result assert_dialog(msg_type _Type, const char* _Caption, const char* _String, unsigned int _DialogTimeoutMS, unsigned int _IgnoreDisableTimeoutMS)
{
	const LONG FrameSpacingX = 5;
	const LONG FrameSpacingY = 6;
	const LONG BtnSpacingX = 3;
	const LONG BtnW = 42;
	const LONG BtnH = 13;
	const LONG BtnCopyW = 65;
	const LONG BtnPanelW = 4 * BtnW + 3 * BtnSpacingX;
	const LONG MinW = BtnCopyW + BtnSpacingX + BtnPanelW + 2 * FrameSpacingX;
	const LONG MinH = 50;
	const LONG MaxW = MinW + 150;
	const LONG MaxH = MinH + 150;

	RECT rString;
	std::vector<char> string(128 * 1024);

	replace(string.data(), string.size(), _String, "\n", "\r\n");
	calc_string_rect(rString, string.data(), MinW, MinH, MaxW, MaxH);

	// Figure out where interface goes based on string RECT
	const LONG BtnPanelLeft = (rString.right - BtnPanelW) - FrameSpacingX;
	const LONG BtnPanelTop = rString.bottom;
	const LONG BtnPanelBottom = BtnPanelTop + BtnH;

	// Offset string box to make room for icon on left
	RECT rIcon = { FrameSpacingX, FrameSpacingY, rIcon.left + (GetSystemMetrics(SM_CXICON)/2), rIcon.top + (GetSystemMetrics(SM_CYICON)/2) };
	rString.left += rIcon.right + FrameSpacingX + 2;
	rString.top += FrameSpacingY;
	rString.right -= FrameSpacingX;
	rString.bottom -= FrameSpacingY;

	// Assign the bounds for the rest of the dialog items

	// 4 main control options are right-aligned, but spaced evenly
	RECT rAbort = { BtnPanelLeft, BtnPanelTop, rAbort.left + BtnW, BtnPanelBottom };
	RECT rBreak = { rAbort.right + BtnSpacingX, BtnPanelTop, rBreak.left + BtnW, BtnPanelBottom }; 
	RECT rContinue = { rBreak.right + BtnSpacingX, BtnPanelTop, rContinue.left + BtnW, BtnPanelBottom };
	RECT rIgnore = { rContinue.right + BtnSpacingX, BtnPanelTop, rIgnore.left + BtnW, BtnPanelBottom };

	// copy to clipboard is left-aligned
	RECT rCopyToClipboard = { FrameSpacingX, BtnPanelTop, rCopyToClipboard.left + BtnCopyW, rCopyToClipboard.top + BtnH };

	// Make the overall dialog
	RECT rDialog = { 0, 0, __max(rString.right, rIgnore.right + FrameSpacingX), __max(rString.bottom, rIgnore.bottom + FrameSpacingY) };

	bool TimedoutControlledEnable = (_IgnoreDisableTimeoutMS == 0);
	const oWINDOWS_DIALOG_ITEM items[] = 
	{
		{ "&Abort", oDLG_BUTTON, IDCANCEL, rAbort, true, true, true },
		{ "&Break", oDLG_BUTTON, IDRETRY, rBreak, true, true, true },
		{ "&Continue", oDLG_BUTTON, IDCONTINUE, rContinue, TimedoutControlledEnable, true, true },
		{ "I&gnore", oDLG_BUTTON, IDIGNORE, rIgnore, TimedoutControlledEnable, true, true },
		{ "Copy &To Clipboard", oDLG_BUTTON, IDCOPYTOCLIPBOARD, rCopyToClipboard, true, true, true },
		{ string.data(), oDLG_LARGELABEL, IDMESSAGE, rString, true, true, true },
		{ "", oDLG_ICON, IDICON, rIcon, true, true, false },
	};

	oWINDOWS_DIALOG_DESC dlg;
	dlg.Font = "Tahoma";
	dlg.Caption = _Caption;
	dlg.pItems = items;
	dlg.NumItems = countof(items);
	dlg.FontPointSize = 8;
	dlg.Rect = rDialog;
	dlg.Center = true;
	dlg.SetForeground = true;
	dlg.Enabled = true;
	dlg.Visible = true;
	dlg.AlwaysOnTop = true;

	LPDLGTEMPLATE lpDlgTemplate = oDlgNewTemplate(dlg);

	HICON hIcon = load_icon(_Type);

	DLGAssert Dialog(hIcon, _DialogTimeoutMS, _IgnoreDisableTimeoutMS);
	INT_PTR int_ptr = DialogBoxIndirectParam(GetModuleHandle(0), lpDlgTemplate, GetDesktopWindow(), DLGAssert::StaticWndProc, (LPARAM)&Dialog);
	
	oDlgDeleteTemplate(lpDlgTemplate);

	if (int_ptr == -1)
	{
		std::string msg = windows::category().message(GetLastError());
		oTRACE("DialogBoxIndirectParam failed. %s\n", msg.c_str());
		__debugbreak(); // debug msgbox called from oASSERTs, so don't recurse into it
	}

	DeleteObject(hIcon);

	msg_result result = as_action(static_cast<WORD>(int_ptr));
	return result;
}

namespace WFNWCV
{
	static oTHREAD_LOCAL HWND hParent = nullptr;
	static oTHREAD_LOCAL WNDPROC OrigWndProc = nullptr;
	static oTHREAD_LOCAL HHOOK Hook = nullptr;

	LRESULT CALLBACK WndProc(HWND _hWnd, UINT _uMsg, WPARAM _wParam, LPARAM _lParam)
	{
		LRESULT result = CallWindowProc(OrigWndProc, _hWnd, _uMsg, _wParam, _lParam);

		switch (_uMsg)
		{
			case WM_ACTIVATE:
			{
				if (hParent)
				{
					RECT rParent, rChild;
					GetWindowRect(_hWnd, &rChild);
					GetWindowRect(hParent, &rParent);
					POINT p = {0,0};
					ClientToScreen(hParent, &p);
					int2 MsgBoxPosition = int2(p.x, p.y) + (oWinRectSize(rParent) - oWinRectSize(rChild)) / 2;
					SetWindowPos(_hWnd, nullptr, MsgBoxPosition.x, MsgBoxPosition.y, 0, 0, SWP_NOZORDER|SWP_NOSIZE);
				}
				break;
			}

			case WM_SETCURSOR:
			{
				cursor::set(_hWnd, mouse_cursor::arrow);
				break;
			}

			case WM_NCDESTROY:
			{
				oVB(UnhookWindowsHookEx(Hook));
				hParent = nullptr;
				Hook = nullptr;
				OrigWndProc = nullptr;
				break;
			}
		}

		return result;
	}

	LRESULT install(int _nCode, WPARAM _wParam, LPARAM _lParam)
	{
		CWPSTRUCT& cwp = *(CWPSTRUCT*)_lParam;
		if (_nCode == HC_ACTION && cwp.message == WM_INITDIALOG && !OrigWndProc)
			OrigWndProc = (WNDPROC)SetWindowLongPtr(cwp.hwnd, GWLP_WNDPROC, (LONG_PTR)WndProc);
		return CallNextHookEx(WFNWCV::Hook, _nCode, _wParam, _lParam);
	}

} // WFNWCV

void set_next_MessageBox_WndProc(HWND _hParent)
{
	WFNWCV::hParent = _hParent;
	if (WFNWCV::hParent)
		WFNWCV::Hook = SetWindowsHookEx(WH_CALLWNDPROC, (HOOKPROC)WFNWCV::install, nullptr, GetWindowThreadProcessId(_hParent, nullptr));
}

msg_result msgboxv(msg_type _Type, ouro::window_handle _hParent, const char* _Title, const char* _Format, va_list _Args)
{
	static const unsigned int _TimeoutMS = INFINITE;

	std::vector<char> msg(128 * 1024);
	vsnprintf(msg.data(), msg.size(), _Format, _Args);
	msg_result result = msg_result::yes;
	HICON hIcon = nullptr;

	std::thread::id ThreadID;

	HWND hWnd = (HWND)_hParent;

	try
	{
		if (!hWnd)
			oWinGetProcessTopWindowAndThread(this_process::get_id(), &hWnd, &ThreadID);
	}
	catch (std::exception&)
	{
		// continue on
	}

	switch (_Type)
	{
		case msg_type::debug:
			result = assert_dialog(_Type, _Title, msg.data(), _TimeoutMS, 0);
			break;
		
		case msg_type::notify_info:
		case msg_type::notify_warn:
		case msg_type::notify_error:
			hIcon = load_icon(_Type);
			// pass thru

		case msg_type::notify:
			notification_area::show_message((ouro::window_handle)hWnd, 0, (ouro::icon_handle)hIcon, __max(2000, _TimeoutMS), _Title, msg.data());
			result = msg_result::ignore;
			break;

		default:
		{
			set_next_MessageBox_WndProc(hWnd);
			result = get_result(MessageBoxTimeout(hWnd, msg.data(), _Title, as_flags(_Type), 0, _TimeoutMS));
			break;
		}
	}

	return result;
}

}
