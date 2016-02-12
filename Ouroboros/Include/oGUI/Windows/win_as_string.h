// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.
// Debug utils for converting common Windows enums and messages 
// into strings.
#pragma once
#ifndef oGUI_win_as_string_h
#define oGUI_win_as_string_h

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace ouro { namespace windows { namespace as_string {

const char* HT(const int& htcode);
const char* SC(const int& sccode);
const char* SW(const int& swCode);
const char* WM(const int& umsg);
const char* WS(const int& wsflag);
const char* WSEX(const int& wsexflag);
const char* WA(const int& wacode);
const char* BST(const int& bstCode);
const char* NM(const int& nmcode);
const char* SWP(const int& swpcode);
const char* GWL(const int& gwlcode);
const char* GWLP(const int& glwpcode);
const char* TCN(const int& tcncode);
const char* CDERR(const int& cderrcode); // common dialog errors
const char* DBT(const int& dbtevent);
const char* DBTDT(const int& dbtdevtype);
const char* SPDRP(const int& spdrpvalue);

char* style_flags(char* dst, size_t dst_size, UINT wsflags);
template<size_t size> inline char* style_flags(char (&dst)[size], UINT wsflags) { return style_flags(dst, size, wsflags); }

char* style_ex_flags(char* dst, size_t dst_size, UINT wsexflags);
template<size_t size> inline char* style_ex_flags(char (&dst)[size], UINT wsexflags) { return style_ex_flags(dst, size, wsexflags); }

char* swp_flags(char* dst, size_t dst_size, UINT swpflags);
template<size_t size> inline char* swp_flags(char (&dst)[size], UINT swpflags) { return swp_flags(dst, size, swpflags); }

		} // namespace as_string

// Fills dst with a string of the WM_* message and details 
// about its parameters. This can be useful for printing out debug details.
char* parse_wm_message(char* dst, size_t dst_size, HWND hwnd, uint32_t msg, WPARAM wparam, LPARAM lparam);
template<size_t size> inline char* parse_wm_message(char (&dst)[size], HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) { return parse_wm_message(dst, size, hwnd, msg, wparam, lparam); }

inline char* parse_wm_message(char* dst, size_t dst_size, const CWPSTRUCT* cwp) { return parse_wm_message(dst, dst_size, cwp->hwnd, cwp->message, cwp->wParam, cwp->lParam); }
template<size_t size> inline char* parse_wm_message(char (&dst)[size], const CWPSTRUCT* cwp) { return parse_wm_message(dst, size, cwp); }

}}

#endif
