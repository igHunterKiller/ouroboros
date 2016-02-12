// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.
#include <oGUI/Windows/win_gdi.h>

namespace ouro {
	namespace windows {
		namespace gdi {
		
int point_to_logical_height(HDC _hDC, int _Point)
{
	return -MulDiv(_Point, GetDeviceCaps(_hDC, LOGPIXELSY), 72);
}

int point_to_logical_heightf(HDC _hDC, float _Point)
{
	return point_to_logical_height(_hDC, static_cast<int>(_Point + 0.5f));
}

int logical_height_to_point(HDC _hDC, int _Height)
{
	return MulDiv(_Height, 72, GetDeviceCaps(_hDC, LOGPIXELSY));
}

float logical_height_to_pointf(HDC _hDC, int _Height)
{
	return (_Height * 72.0f) / (float)GetDeviceCaps(_hDC, LOGPIXELSY);
}

float2 dpi_scale(HDC _hDC)
{
	return float2(GetDeviceCaps(_hDC, LOGPIXELSX) / 96.0f, GetDeviceCaps(_hDC, LOGPIXELSY) / 96.0f);
}

int estimate_point_size(int _PixelHeight)
{
	// http://reeddesign.co.uk/test/points-pixels.html
	switch (_PixelHeight)
	{
		case 8: return 6;
		case 9: return 7;
		case 11: return 8;
		case 12: return 9;
		case 13: return 10;
		case 15: return 11;
		case 16: return 12;
		case 17: return 13;
		case 19: return 14;
		case 21: return 15;
		case 22: return 16;
		case 23: return 17;
		case 24: return 18;
		case 26: return 20;
		case 29: return 22;
		case 32: return 24;
		case 35: return 26;
		case 36: return 27;
		case 37: return 28;
		case 38: return 29;
		case 40: return 30;
		case 42: return 32;
		case 45: return 34;
		case 48: return 36;
		default: break;
	}

	if (_PixelHeight < 8)
		return 4;
	
	return static_cast<int>((_PixelHeight * 36.0f / 48.0f) + 0.5f);
}
		} // namespace gdi
	}
}
