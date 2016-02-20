// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.
// Utilities for working with Windows RECTs

#pragma once
#include <oGUI/oGUI.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

inline int4 oRect(const RECT& rect)                                         { int4 r; r.xy() = int2(rect.left, rect.top); r.zw() = int2(rect.right, rect.bottom); return r; }
inline RECT oWinRect(const int4& rect)                                      { RECT r; r.left = rect.x; r.top = rect.y; r.right = rect.z; r.bottom = rect.w; return r; }
inline RECT oWinRect(int left, int top, int right, int bottom)              { RECT r; r.left = __min(left, right); r.top = __min(top, bottom); r.right = __max(left, right); r.bottom = __max(top, bottom); return r; }
inline RECT oWinRect(const int2& _TopLeft, const int2 bottom_right)         { return oWinRect(_TopLeft.x, _TopLeft.y, bottom_right.x, bottom_right.y); }
inline RECT oWinRectWH(int left, int top, int width, int height)            { return oWinRect(left, top, left + width, top + height); }
inline RECT oWinRectWH(const int2& position, const int2& size)              { return oWinRectWH(position.x, position.y, size.x, size.y); }
inline int  oWinRectW(const RECT& rect)                                     { return rect.right - rect.left; }
inline int  oWinRectH(const RECT& rect)                                     { return rect.bottom - rect.top; }
inline int2 oWinRectPosition(const RECT& rect)                              { return int2(rect.left, rect.top); }
inline int2 oWinRectSize(const RECT& rect)                                  { return int2(oWinRectW(rect), oWinRectH(rect)); }
inline RECT oWinClip(const RECT& rect_container, const RECT& to_be_clipped) { RECT r = to_be_clipped; r.left = __max(r.left, rect_container.left); r.top = __max(r.top, rect_container.top); r.right = __min(r.right, rect_container.right); r.bottom = __min(r.bottom, rect_container.bottom); return r; }
inline int2 oWinClip(const RECT& rect_container, const int2& to_be_clipped) { int2 r = to_be_clipped; r.x = __min(__max(r.x, rect_container.left), rect_container.right); r.y = __min(__max(r.y, rect_container.top), rect_container.bottom); return r; }
