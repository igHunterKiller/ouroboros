// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.
// Utilities for working with Windows RECTs
#pragma once
#ifndef oWinRect_h
#define oWinRect_h

#include <oGUI/oGUI.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

inline int4 oRect(const RECT& _Rect) { int4 r; r.xy() = int2(_Rect.left, _Rect.top); r.zw() = int2(_Rect.right, _Rect.bottom); return r; }


inline RECT oWinRect(const int4& _Rect) { RECT r; r.left = _Rect.x; r.top = _Rect.y; r.right = _Rect.z; r.bottom = _Rect.w; return r; }
inline RECT oWinRect(int _Left, int _Top, int _Right, int _Bottom) { RECT r; r.left = __min(_Left, _Right); r.top = __min(_Top, _Bottom); r.right = __max(_Left, _Right); r.bottom = __max(_Top, _Bottom); return r; }
inline RECT oWinRect(const int2& _TopLeft, const int2 _BottomRight) { return oWinRect(_TopLeft.x, _TopLeft.y, _BottomRight.x, _BottomRight.y); }
inline RECT oWinRectWH(int _Left, int _Top, int _Width, int _Height) { return oWinRect(_Left, _Top, _Left + _Width, _Top + _Height); }
inline RECT oWinRectWH(const int2& _Position, const int2& _Size) { return oWinRectWH(_Position.x, _Position.y, _Size.x, _Size.y); }
inline int oWinRectW(const RECT& _Rect) { return _Rect.right - _Rect.left; }
inline int oWinRectH(const RECT& _Rect) { return _Rect.bottom - _Rect.top; }
inline int2 oWinRectPosition(const RECT& _Rect) { return int2(_Rect.left, _Rect.top); }
inline int2 oWinRectSize(const RECT& _Rect) { return int2(oWinRectW(_Rect), oWinRectH(_Rect)); }

inline RECT oWinClip(const RECT& _rContainer, const RECT& _ToBeClipped) { RECT r = _ToBeClipped; r.left = __max(r.left, _rContainer.left); r.top = __max(r.top, _rContainer.top); r.right = __min(r.right, _rContainer.right); r.bottom = __min(r.bottom, _rContainer.bottom); return r; }
inline int2 oWinClip(const RECT& _rContainer, const int2& _ToBeClipped) { int2 r = _ToBeClipped; r.x = __min(__max(r.x, _rContainer.left), _rContainer.right); r.y = __min(__max(r.y, _rContainer.top), _rContainer.bottom); return r; }

// Returns a RECT that is position to accommodate _Size and _Alignment relative 
// to _Anchor according to _Alignment. For example, a middle center alignment
// would have a rect whose center was at _Anchor.
RECT oWinRectResolve(const int2& _Anchor, const int2& _Size, ouro::alignment::value _Alignment);

#endif
