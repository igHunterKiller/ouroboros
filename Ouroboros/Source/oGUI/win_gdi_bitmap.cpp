// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oGUI/Windows/win_gdi_bitmap.h>
#include <oSystem/windows/win_error.h>
#include <oMath/quantize.h>

namespace ouro { namespace windows { namespace gdi {
		
BITMAPINFOHEADER make_header(const surface::info_t& info, bool top_down)
{
	switch (info.format)
	{
		case surface::format::b8g8r8a8_unorm:
		case surface::format::b8g8r8_unorm:
		case surface::format::r8_unorm:
			break;
		default:
			throw std::invalid_argument("unsupported format");
	}

	if (info.array_size != 0 || info.dimensions.z != 1)
		throw std::invalid_argument("no support for 3D or array surfaces");

	if (info.dimensions.x <= 0 || info.dimensions.y <= 0)
		throw std::invalid_argument("invalid dimensions");

	BITMAPINFOHEADER h;
	h.biSize = sizeof(BITMAPINFOHEADER);
	h.biWidth = info.dimensions.x;
	h.biHeight = top_down ? -(int)info.dimensions.y : (int)info.dimensions.y;
	h.biPlanes = 1; 
	h.biBitCount = static_cast<WORD>(surface::bits(info.format));
	h.biCompression = BI_RGB;
	h.biSizeImage = surface::element_size(info.format) * info.dimensions.y;
	h.biXPelsPerMeter = 0;
	h.biYPelsPerMeter = 0;
	h.biClrUsed = 0;
	h.biClrImportant = 0;
	return h;
}

BITMAPV4HEADER make_headerv4(const surface::info_t& info, bool top_down)
{
	BITMAPV4HEADER v4;
	BITMAPINFOHEADER v3 = make_header(info, top_down);
	memset(&v4, 0, sizeof(v4));
  memcpy(&v4, &v3, sizeof(v3));
	v4.bV4Size = sizeof(v4);
	v4.bV4RedMask   = color::mask_red;
	v4.bV4GreenMask = color::mask_green;
	v4.bV4BlueMask  = color::mask_blue;
	v4.bV4AlphaMask = color::mask_alpha;
	return v4;
}

void fill_monochrone_palette(RGBQUAD* out_colors, uint32_t argb0, uint32_t argb1)
{
	float4 c0 = truetofloat4(argb0);
	float4 c1 = truetofloat4(argb1);

	for (uint8_t i = 0; i < 256; i++)
	{
		float4 c = ::lerp(c0, c1, n8tof32(i));
		RGBQUAD& q = out_colors[i];
		argb_channels ch;
		ch.argb       = float4totrue(c);
		q.rgbRed      = ch.r;
		q.rgbGreen    = ch.g;
		q.rgbBlue     = ch.b;
		q.rgbReserved = ch.a;
	}
}

surface::info_t get_info(const BITMAPINFOHEADER& header)
{
	surface::info_t si;
	si.dimensions.x = header.biWidth;
	si.dimensions.y = abs(header.biHeight); // ignore top v. bottom up here
	si.dimensions.z = 1;
	si.mip_layout = surface::mip_layout::none;
	si.array_size = 0;

	switch (header.biBitCount)
	{
		case 1: si.format = surface::format::r1_unorm; break;
		case 16: si.format = surface::format::b5g5r5a1_unorm; break;
		case 8: si.format = surface::format::r8_unorm; break;
		case 24: case 0: si.format = surface::format::b8g8r8_unorm; break;
		case 32: si.format = surface::format::b8g8r8a8_unorm; break;
		default: si.format = surface::format::unknown; break;
	}

	return si;
}

surface::info_t get_info(const BITMAPV4HEADER& header)
{
	return get_info((const BITMAPINFOHEADER&)header);
}

static size_t get_bitmapinfo_size(surface::format _Format)
{
	return surface::bits(_Format) == 8 ? (sizeof(BITMAPINFO) + sizeof(RGBQUAD) * 255) : sizeof(BITMAPINFO);
}

scoped_bitmap make_bitmap(const surface::image* img)
{
	const auto& si = img->info();

	if (si.format != ouro::surface::format::b8g8r8a8_unorm)
		throw std::invalid_argument("only b8g8r8a8_unorm currently supported");

	surface::shared_lock lock(img);
	return CreateBitmap(si.dimensions.x, si.dimensions.y, 1, 32, lock.mapped.data);
}

void memcpy2d(void* _pDestination, size_t _DestinationPitch, HBITMAP _hBmp, size_t _NumRows, bool _FlipVertically)
{
	scoped_getdc hDC(nullptr);

	struct BITMAPINFO_FULL
	{
		BITMAPINFOHEADER bmiHeader;
		RGBQUAD bmiColors[256];
	};

	BITMAPINFO_FULL bmif;
	memset(&bmif, 0, sizeof(bmif));
	bmif.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	GetDIBits(hDC, _hBmp, 0, 1, nullptr, (BITMAPINFO*)&bmif, DIB_RGB_COLORS);

	for (size_t y = 0; y < _NumRows; y++)
	{
		int nScanlinesRead = GetDIBits(hDC, _hBmp, static_cast<unsigned int>(_FlipVertically ? (_NumRows - 1 - y) : y), 1, (uint8_t*)_pDestination + y * _DestinationPitch, (BITMAPINFO*)&bmif, DIB_RGB_COLORS);
		if (nScanlinesRead == ERROR_INVALID_PARAMETER)
			throw std::invalid_argument("invalid argument passed to GetDIBtis");
		else if (!nScanlinesRead)
			throw std::system_error(std::errc::io_error, std::system_category(), "GetDIBits failed");
	}
}

void draw_bitmap(HDC _hDC, int _X, int _Y, HBITMAP _hBitmap, DWORD _dwROP)
{
	BITMAP Bitmap;
	if (!_hDC || !_hBitmap)
		throw std::invalid_argument("");
	GetObject(_hBitmap, sizeof(BITMAP), (LPSTR)&Bitmap);
	scoped_compat hDCBitmap(_hDC);
	scoped_select sel(hDCBitmap, _hBitmap);
	oVB(BitBlt(_hDC, _X, _Y, Bitmap.bmWidth, Bitmap.bmHeight, hDCBitmap, 0, 0, _dwROP));
}

void stretch_bitmap(HDC _hDC, int _X, int _Y, int _Width, int _Height, HBITMAP _hBitmap, DWORD _dwROP)
{
	BITMAP Bitmap;
	if (!_hDC || !_hBitmap)
		throw std::invalid_argument("");
	GetObject(_hBitmap, sizeof(BITMAP), (LPSTR)&Bitmap);
	scoped_compat hDCBitmap(_hDC);
	scoped_select sel(hDCBitmap, _hBitmap);
	oVB(StretchBlt(_hDC, _X, _Y, _Width, _Height, hDCBitmap, 0, 0, Bitmap.bmWidth, Bitmap.bmHeight, _dwROP));
}

void stretch_blend_bitmap(HDC _hDC, int _X, int _Y, int _Width, int _Height, HBITMAP _hBitmap)
{
	static const BLENDFUNCTION kBlend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
	BITMAP Bitmap;
	if (!_hDC || !_hBitmap)
		throw std::invalid_argument("");
	GetObject(_hBitmap, sizeof(BITMAP), (LPSTR)&Bitmap);
	scoped_compat hDCBitmap(_hDC);
	scoped_select sel(hDCBitmap, _hBitmap);
	oVB(AlphaBlend(_hDC, _X, _Y, _Width, _Height, hDCBitmap, 0, 0, Bitmap.bmWidth, Bitmap.bmHeight, kBlend));
}

void stretch_bits(HDC _hDC, const RECT& _DestRect, const int2& _SourceSize, surface::format _SourceFormat, const void* _pSourceBits, bool _FlipVertically)
{
	surface::info_t si;
	si.dimensions = int3(_SourceSize, 1);
	si.format = _SourceFormat;
	const size_t bitmapinfoSize = get_bitmapinfo_size(si.format);
	BITMAPINFO* pBMI = (BITMAPINFO*)alloca(bitmapinfoSize);
	pBMI->bmiHeader = make_header(si, !_FlipVertically);
	if (bitmapinfoSize != sizeof(BITMAPINFO))
		fill_monochrone_palette(pBMI->bmiColors);
	scoped_blt_mode Mode(_hDC, HALFTONE);
	oVB(StretchDIBits(_hDC, _DestRect.left, _DestRect.top, oWinRectW(_DestRect), oWinRectH(_DestRect), 0, 0, _SourceSize.x, _SourceSize.y, _pSourceBits, pBMI, DIB_RGB_COLORS, SRCCOPY));
}

void stretch_bits(HWND _hWnd, const RECT& _DestRect, const int2& _SourceSize, surface::format _SourceFormat, const void* _pSourceBits, bool _FlipVertically)
{
	RECT destRect;
	if (_DestRect.bottom == -1 || _DestRect.top == -1 ||
		_DestRect.left == -1 || _DestRect.right == -1)
	{
		RECT rClient;
		GetClientRect(_hWnd, &rClient);
		destRect = rClient;
	}
	else
		destRect = _DestRect;

	scoped_getdc hDC(_hWnd);
	stretch_bits(hDC, destRect, _SourceSize, _SourceFormat, _pSourceBits, _FlipVertically);
}

void stretch_bits(HWND _hWnd, const int2& _SourceSize, surface::format _SourceFormat, const void* _pSourceBits, bool _FlipVertically)
{
	RECT destRect;
	destRect.bottom = -1;
	destRect.left = -1;
	destRect.right = -1;
	destRect.top = -1;
	stretch_bits(_hWnd, destRect, _SourceSize, _SourceFormat, _pSourceBits, _FlipVertically);
}

int2 bitmap_dimensions(HDC _hDC)
{
	BITMAP BI;
	memset(&BI, 0, sizeof(BI));
	GetObject(current_bitmap(_hDC), sizeof(BITMAP), &BI);
	return int2(BI.bmWidth, BI.bmHeight);
}

int2 icon_dimensions(HICON _hIcon)
{
	ICONINFO ii;
	BITMAP b;
	if (GetIconInfo(_hIcon, &ii))
	{
		if (ii.hbmColor)
		{
			if (GetObject(ii.hbmColor, sizeof(b), &b))
				return int2(b.bmWidth, b.bmHeight);
		}

		else
		{
			if (GetObject(ii.hbmMask, sizeof(b), &b))
				return int2(b.bmWidth, b.bmHeight);
		}
	}

	return int2(-1,-1);
}

HBITMAP bitmap_from_icon(HICON _hIcon)
{
	scoped_compat hDC(nullptr);
	int2 Size = icon_dimensions(_hIcon);
	HBITMAP hBitmap = CreateCompatibleBitmap(hDC, Size.x, Size.y);
	scoped_select sel(hDC, hBitmap);
	oVB(DrawIconEx(hDC, 0, 0, _hIcon, 0, 0, 0, nullptr, DI_NORMAL));
	return hBitmap;
}

HICON bitmap_to_icon(HBITMAP _hBmp)
{
	HICON hIcon = nullptr;
	BITMAPINFO bi = {0};
	bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	if (GetDIBits(GetDC(0), _hBmp, 0, 0, 0, &bi, DIB_RGB_COLORS))
	{
		scoped_bitmap hMask(CreateCompatibleBitmap(GetDC(0), bi.bmiHeader.biWidth, bi.bmiHeader.biHeight));
		ICONINFO ii = {0};
		ii.fIcon = TRUE;
		ii.hbmColor = _hBmp;
		ii.hbmMask = hMask;
		hIcon = CreateIconIndirect(&ii);
		DeleteObject(hMask);
	}

	return hIcon;
}

}}}
