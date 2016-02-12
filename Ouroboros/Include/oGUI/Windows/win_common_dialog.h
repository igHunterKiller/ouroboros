// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.
// Utilities for working with Window's common dialogs.
#pragma once
#ifndef oGUI_win_common_dialog_h
#define oGUI_win_common_dialog_h

#include <oString/path.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace ouro { namespace windows { namespace common_dialog {

// All functions return true if successful or false if the user canceled the dialog
// and thus there is no error but values are not valid.
// If there is a system error an exception is thrown.

// Filter pairs are delimited by '|' and are the description, and the wildcard:
// i.e. "Text Files|*.txt|Bitmap Files|*.bmp"
// If _Path is not empty it will be used as the initial starting folder.
bool open_path(path_t& _Path, const char* _DialogTitle, const char* _FilterPairs, HWND _hParent = nullptr);
bool save_path(path_t& _Path, const char* _DialogTitle, const char* _FilterPairs, HWND _hParent = nullptr);

// *inout_argb is used as the initial value of the dialog
bool pick_color(uint32_t* inout_argb, HWND hparent = nullptr);

// *_pLogicalFont is used to initialize the dialog
bool pick_font(LOGFONT* _pLogicalFont, uint32_t* inout_argb, HWND hparent);

}}}

#endif
