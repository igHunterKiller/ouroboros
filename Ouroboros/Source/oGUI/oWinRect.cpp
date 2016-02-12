// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.
#include <oGUI/Windows/oWinRect.h>

RECT oWinRectResolve(const int2& _Anchor, const int2& _Size, ouro::alignment::value _Alignment)
{
	int2 offset(0,0);
	int2 code = int2(_Alignment % 3, _Alignment / 3);

	// center/middle
	if (code.x == 1) offset.x = -_Size.x / 2;
	if (code.y == 1) offset.y = -_Size.y / 2;

	// right/bottom
	if (code.x == 2) offset.x = -_Size.x;
	if (code.y == 2) offset.y = -_Size.y;

	RECT r = oWinRectWH(_Anchor, _Size);
	OffsetRect(&r, offset.x, offset.y);
	return r;
}
