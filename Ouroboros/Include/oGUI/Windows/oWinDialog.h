// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.
// Wrappers for creating a Windows dialog.
#pragma once
#ifndef oWinDialog_h
#define oWinDialog_h

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

enum oWINDOWS_DIALOG_ITEM_TYPE
{
	oDLG_BUTTON,
	oDLG_EDITBOX,
	oDLG_LABEL_LEFT_ALIGNED,
	oDLG_LABEL_CENTERED,
	oDLG_LABEL_RIGHT_ALIGNED,
	oDLG_LARGELABEL,
	oDLG_ICON,
	oDLG_LISTBOX,
	oDLG_SCROLLBAR,
	oDLG_COMBOBOX,
};

struct oWINDOWS_DIALOG_ITEM
{
	const char* Text;
	oWINDOWS_DIALOG_ITEM_TYPE Type;
	WORD ItemID;
	RECT Rect;
	bool Enabled;
	bool Visible;
	bool TabStop;
};

struct oWINDOWS_DIALOG_DESC
{
	const char* Font;
	const char* Caption;
	const oWINDOWS_DIALOG_ITEM* pItems;
	UINT NumItems;
	UINT FontPointSize;
	RECT Rect;
	bool Center; // if true, ignores Rect.left, Rect.top positioning
	bool SetForeground;
	bool Enabled;
	bool Visible;
	bool AlwaysOnTop;
};

LPDLGTEMPLATE oDlgNewTemplate(const oWINDOWS_DIALOG_DESC& _Desc);
void oDlgDeleteTemplate(LPDLGTEMPLATE _lpDlgTemplate);

#endif
