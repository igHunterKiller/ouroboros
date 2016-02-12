// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Standard structs and definitions for the bitmap (.bmp) image file format.

#pragma once
#include <cstdint>
#include <cassert>

static const uint16_t bmp_signature = 0x4d42; // 'BM'

enum bmp_compression
{
	rgb = 0,
	rle8 = 1,
	rle4 = 2,
	bitfields = 3,
	jpeg = 4,
	png = 5,
};

struct bmp_rgbquad
{
	uint8_t rgbBlue;
	uint8_t rgbGreen;
	uint8_t rgbRed;
	uint8_t rgbReserved;
};

struct bmp_ciexyz
{
	int32_t ciexyzX;
	int32_t ciexyzY;
	int32_t ciexyzZ;
};

struct bmp_ciexyztriple
{
	bmp_ciexyz ciexyzRed;
	bmp_ciexyz ciexyzGreen;
	bmp_ciexyz ciexyzBlue;
};

struct bmp_infoheader
{
	uint32_t biSize;
	int32_t biWidth;
	int32_t biHeight;
	uint16_t biPlanes;
	uint16_t biBitCount;
	uint32_t biCompression;
	uint32_t biSizeImage;
	int32_t biXPelsPerMeter;
	int32_t biYPelsPerMeter;
	uint32_t biClrUsed;
	uint32_t biClrImportant;
};

struct bmp_v4infoheader
{
	uint32_t bV4Size;
	int32_t bV4Width;
	int32_t bV4Height;
	uint16_t bV4Planes;
	uint16_t bV4BitCount;
	uint32_t bV4V4Compression;
	uint32_t bV4SizeImage;
	int32_t bV4XPelsPerMeter;
	int32_t bV4YPelsPerMeter;
	uint32_t bV4ClrUsed;
	uint32_t bV4ClrImportant;
	uint32_t bV4RedMask;
	uint32_t bV4GreenMask;
	uint32_t bV4BlueMask;
	uint32_t bV4AlphaMask;
	uint32_t bV4CSType;
	bmp_ciexyztriple bV4Endpoints;
	uint32_t bV4GammaRed;
	uint32_t bV4GammaGreen;
	uint32_t bV4GammaBlue;
};

struct bmp_v5infoheader
{
	uint32_t bv5Size;
	int32_t bv5Width;
	int32_t bv5Height;
	uint16_t bv5Planes;
	uint16_t bv5BitCount;
	uint32_t bv5v5Compression;
	uint32_t bv5SizeImage;
	int32_t bv5XPelsPerMeter;
	int32_t bv5YPelsPerMeter;
	uint32_t bv5ClrUsed;
	uint32_t bv5ClrImportant;
	uint32_t bv5RedMask;
	uint32_t bv5GreenMask;
	uint32_t bv5BlueMask;
	uint32_t bv5AlphaMask;
	uint32_t bv5CSType;
	bmp_ciexyztriple bv5Endpoints;
	uint32_t bv5GammaRed;
	uint32_t bv5GammaGreen;
	uint32_t bv5GammaBlue;
	uint32_t bv5Intent;
	uint32_t bv5ProfileData;
	uint32_t bv5ProfileSize;
	uint32_t bv5Reserved;
};

struct bmp_info
{
	bmp_infoheader bmiHeader;
	bmp_rgbquad bmiColors[1];
};

#pragma pack(push,2)
struct bmp_header
{
	uint16_t bfType;
	uint32_t bfSize;
	uint16_t bfReserved1;
	uint16_t bfReserved2;
	uint32_t bfOffBits;
};
#pragma pack(pop)

static_assert(sizeof(bmp_infoheader) == 40, "size mismatch");
static_assert(sizeof(bmp_v4infoheader) == 108, "size mismatch");
static_assert(sizeof(bmp_v5infoheader) == 124, "size mismatch");
static_assert(sizeof(bmp_info) == 44, "size mismatch");
static_assert(sizeof(bmp_header) == 14, "size mismatch");
