// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Functions for working with the GUI's taskbar notification area 
// (Window's tray).

#pragma once

#include <oGUI/oGUI.h>

namespace ouro { namespace notification_area {

// Return the ouro::window_handle of the "system tray" or "notification area"	
ouro::window_handle native_handle();

// Sets the focus on the tray itself, not any icon in it
void focus();

// Get the rectangle of the specified icon. Returns true if the rect is valid, 
// or false if the icon doesn't exist.
void icon_rect(ouro::window_handle _hWnd, unsigned int _ID, int* _pX, int* _pY, int* _pWidth, int* _pHeight);

// Returns true of the tray icon exists
bool exists(ouro::window_handle _hWnd, unsigned int _ID);

// Icons are identified by the ouro::window_handle and the ID. If _CallbackMessage is not zero 
// then this is a message that can be handled in the ouro::window_handle's WNDPROC. Use 
// WM_USER+n for the custom code. If ouro::icon_handle is nullptr the icon from the ouro::window_handle 
// will be used. All lifetime management of a valid ouro::icon_handle must be handled by 
// client code.
void show_icon(ouro::window_handle _hWnd, unsigned int _ID, unsigned int _CallbackMessage, ouro::icon_handle _hIcon, bool _Show);

// Once an icon has been created with show_icon, use this to display a message 
// on it. If _hIcon is nullptr then the _hWnd's icon is used.
void show_message(ouro::window_handle _hWnd, unsigned int _ID, ouro::icon_handle _hIcon, unsigned int _TimeoutMS, const char* _Title, const char* _Message);

// Minimize a window to the tray (animates a window to the tray)
void minimize(ouro::window_handle _hWnd, unsigned int _CallbackMessage, ouro::icon_handle _hIcon);

// Animates an existing tray icon to a restored window
void restore(ouro::window_handle _hWnd);

#if defined(_WIN32) || defined(_WIN64)
	// Use this in a windows message handler to decode the parameters for a callback
	// message as passed to one of the above APIs.
	void decode_callback_message_params(uintptr_t _wParam, uintptr_t _lParam
		, unsigned int* _pNotificationEvent, unsigned int* _pID, int* _pX, int* _pY);
#endif

}}
