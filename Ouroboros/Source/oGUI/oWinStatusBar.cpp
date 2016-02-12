// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oCore/stringf.h>
#include <oGUI/Windows/oWinStatusBar.h>
#include <oGUI/Windows/oWinRect.h>
#include <oGUI/Windows/oWinWindowing.h>
#include <oSystem/windows/win_error.h>
#include <CommCtrl.h>

using namespace ouro;

HWND oWinStatusBarCreate(HWND _hParent, HMENU _ID, int _MinHeight)
{
	HWND hWnd = CreateWindowEx(
		0
		, STATUSCLASSNAME
		, ""
		, WS_VISIBLE|WS_CHILD|SBARS_SIZEGRIP|SBARS_TOOLTIPS
		, 0
		, 0
		, 0
		, 0
		, (HWND)_hParent
		, _ID
		, nullptr
		, nullptr);

	if (_MinHeight != oDEFAULT)
		SendMessage(hWnd, SB_SETMINHEIGHT, (WPARAM)_MinHeight, 0);
	SendMessage(hWnd, WM_SIZE, 0, 0);
	return hWnd;
}

bool oWinIsStatusBar(HWND _hStatusBar)
{
	sstring ClassName;
	if (!GetClassName(_hStatusBar, ClassName.c_str(), (int)ClassName.capacity()))
		return false;

	return !strcmp(ClassName, STATUSCLASSNAME);
}

void oWinStatusBarSetMinHeight(HWND _hStatusBar, int _MinHeight)
{
	SendMessage(_hStatusBar, SB_SETMINHEIGHT, (WPARAM)_MinHeight, 0);
}

void oWinStatusBarSetNumItems(HWND _hStatusBar, const int* _pItemWidths, size_t _NumItems)
{
	std::array<int, 256> CoordOfRight;

	int LastLeft = 0;
	const int n = (int)_NumItems;
	for (int i = 0; i < n; i++)
	{
		LastLeft += _pItemWidths[i];
		
		if (_pItemWidths[i] == -1)
		{
			CoordOfRight[i] = -1;
			break;
		}
		CoordOfRight[i] = LastLeft;
	}

	oVB(SendMessage(_hStatusBar, SB_SETPARTS, (WPARAM)_NumItems, (LPARAM)CoordOfRight.data()));
}

int oWinStatusBarGetNumItems(HWND _hStatusBar, int* _pItemWidths, size_t _MaxNumItemWidths)
{
	return (int)SendMessage(_hStatusBar, SB_GETPARTS, (WPARAM)(_MaxNumItemWidths == 0 ? INT_MAX : _MaxNumItemWidths), (LPARAM)_pItemWidths);
}

RECT oWinStatusBarGetItemRect(HWND _hStatusBar, int _ItemIndex)
{
	RECT r;
	oVB(SendMessage(_hStatusBar, SB_GETRECT, (WPARAM)_ItemIndex, (LPARAM)&r));
	return r;
}

static int oWinStatusBarGetStyle(ouro::border_style::value _Style)
{
	switch (_Style)
	{
		case ouro::border_style::sunken: return 0;
		case ouro::border_style::flat: return SBT_NOBORDERS;
		case ouro::border_style::raised: return SBT_POPOUT;
		default: throw std::invalid_argument(stringf("unexpected style %d", _Style));
	}
}

void oWinStatusBarSetText(HWND _hStatusBar, int _ItemIndex, ouro::border_style::value _BorderStyle, const char* _Format, va_list _Args)
{
	lstring s;
	vsnprintf(s, _Format, _Args);
	ellipsize(s);
	WPARAM w = (_ItemIndex & 0xff) | oWinStatusBarGetStyle(_BorderStyle);
	if (!SendMessage(_hStatusBar, SB_SETTEXT, w, (LPARAM)s.c_str()))
	{
		int nParts = (int)SendMessage(_hStatusBar, SB_GETPARTS, INT_MAX, 0);
		if (_ItemIndex >= nParts)
			throw std::system_error(std::errc::no_buffer_space, std::system_category(), stringf("The specified status bar item index %d is out of range (number of items %d)", _ItemIndex, nParts));
		else
			oVB(false);
	}
}

char* oWinStatusBarGetText(char* _StrDestination, size_t _SizeofStrDestination, HWND _hStatusBar, int _ItemIndex)
{
	LRESULT lResult = SendMessage(_hStatusBar, SB_GETTEXTLENGTH, (WPARAM)_ItemIndex, 0);
	size_t len = LOWORD(lResult);
	if (len >= _SizeofStrDestination)
		throw std::system_error(std::errc::no_buffer_space, std::system_category());
	SendMessage(_hStatusBar, SB_GETTEXT, (WPARAM)_ItemIndex, (LPARAM)_StrDestination);
	return _StrDestination;
}

void oWinStatusBarSetTipText(HWND _hStatusBar, int _ItemIndex, const char* _Text)
{
	SendMessage(_hStatusBar, SB_SETTEXT, (WPARAM)_ItemIndex, (LPARAM)_Text);
}

char* oWinStatusBarGetTipText(char* _StrDestination, size_t _SizeofStrDestination, HWND _hStatusBar, int _ItemIndex)
{
	UINT w = (UINT)(uint16_t(_SizeofStrDestination) << 16) | (UINT)(_ItemIndex & 0xffff);
	SendMessage(_hStatusBar, SB_GETTIPTEXT, (WPARAM)w, (LPARAM)_StrDestination);
	return _StrDestination;
}

void oWinStatusBarSetIcon(HWND _hStatusBar, int _ItemIndex, HICON _hIcon)
{
	oVB(SendMessage(_hStatusBar, SB_SETICON, (WPARAM)_ItemIndex, (LPARAM)_hIcon));
}

HICON oWinStatusBarGetIcon(HWND _hStatusBar, int _ItemIndex)
{
	return (HICON)SendMessage(_hStatusBar, SB_GETICON, (WPARAM)_ItemIndex, 0);
}
