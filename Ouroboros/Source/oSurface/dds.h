// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Standard structs and definitions for the direct draw surface (.dds) 
// image file format.

// Note for testing: VS2012 loads even exotic formats like BC7, just 
// drag the dds into the menu/icon area.

#pragma once
#include <cstdint>

static const uint32_t dds_signature = 0x20534444; // "DDS "

// same as DXGI_FORMAT
enum DDS_FORMAT
{
	DDS_FORMAT_UNKNOWN	                   = 0,
	DDS_FORMAT_R32G32B32A32_TYPELESS       = 1,
	DDS_FORMAT_R32G32B32A32_FLOAT          = 2,
	DDS_FORMAT_R32G32B32A32_UINT           = 3,
	DDS_FORMAT_R32G32B32A32_SINT           = 4,
	DDS_FORMAT_R32G32B32_TYPELESS          = 5,
	DDS_FORMAT_R32G32B32_FLOAT             = 6,
	DDS_FORMAT_R32G32B32_UINT              = 7,
	DDS_FORMAT_R32G32B32_SINT              = 8,
	DDS_FORMAT_R16G16B16A16_TYPELESS       = 9,
	DDS_FORMAT_R16G16B16A16_FLOAT          = 10,
	DDS_FORMAT_R16G16B16A16_UNORM          = 11,
	DDS_FORMAT_R16G16B16A16_UINT           = 12,
	DDS_FORMAT_R16G16B16A16_SNORM          = 13,
	DDS_FORMAT_R16G16B16A16_SINT           = 14,
	DDS_FORMAT_R32G32_TYPELESS             = 15,
	DDS_FORMAT_R32G32_FLOAT                = 16,
	DDS_FORMAT_R32G32_UINT                 = 17,
	DDS_FORMAT_R32G32_SINT                 = 18,
	DDS_FORMAT_R32G8X24_TYPELESS           = 19,
	DDS_FORMAT_D32_FLOAT_S8X24_UINT        = 20,
	DDS_FORMAT_R32_FLOAT_X8X24_TYPELESS    = 21,
	DDS_FORMAT_X32_TYPELESS_G8X24_UINT     = 22,
	DDS_FORMAT_R10G10B10A2_TYPELESS        = 23,
	DDS_FORMAT_R10G10B10A2_UNORM           = 24,
	DDS_FORMAT_R10G10B10A2_UINT            = 25,
	DDS_FORMAT_R11G11B10_FLOAT             = 26,
	DDS_FORMAT_R8G8B8A8_TYPELESS           = 27,
	DDS_FORMAT_R8G8B8A8_UNORM              = 28,
	DDS_FORMAT_R8G8B8A8_UNORM_SRGB         = 29,
	DDS_FORMAT_R8G8B8A8_UINT               = 30,
	DDS_FORMAT_R8G8B8A8_SNORM              = 31,
	DDS_FORMAT_R8G8B8A8_SINT               = 32,
	DDS_FORMAT_R16G16_TYPELESS             = 33,
	DDS_FORMAT_R16G16_FLOAT                = 34,
	DDS_FORMAT_R16G16_UNORM                = 35,
	DDS_FORMAT_R16G16_UINT                 = 36,
	DDS_FORMAT_R16G16_SNORM                = 37,
	DDS_FORMAT_R16G16_SINT                 = 38,
	DDS_FORMAT_R32_TYPELESS                = 39,
	DDS_FORMAT_D32_FLOAT                   = 40,
	DDS_FORMAT_R32_FLOAT                   = 41,
	DDS_FORMAT_R32_UINT                    = 42,
	DDS_FORMAT_R32_SINT                    = 43,
	DDS_FORMAT_R24G8_TYPELESS              = 44,
	DDS_FORMAT_D24_UNORM_S8_UINT           = 45,
	DDS_FORMAT_R24_UNORM_X8_TYPELESS       = 46,
	DDS_FORMAT_X24_TYPELESS_G8_UINT        = 47,
	DDS_FORMAT_R8G8_TYPELESS               = 48,
	DDS_FORMAT_R8G8_UNORM                  = 49,
	DDS_FORMAT_R8G8_UINT                   = 50,
	DDS_FORMAT_R8G8_SNORM                  = 51,
	DDS_FORMAT_R8G8_SINT                   = 52,
	DDS_FORMAT_R16_TYPELESS                = 53,
	DDS_FORMAT_R16_FLOAT                   = 54,
	DDS_FORMAT_D16_UNORM                   = 55,
	DDS_FORMAT_R16_UNORM                   = 56,
	DDS_FORMAT_R16_UINT                    = 57,
	DDS_FORMAT_R16_SNORM                   = 58,
	DDS_FORMAT_R16_SINT                    = 59,
	DDS_FORMAT_R8_TYPELESS                 = 60,
	DDS_FORMAT_R8_UNORM                    = 61,
	DDS_FORMAT_R8_UINT                     = 62,
	DDS_FORMAT_R8_SNORM                    = 63,
	DDS_FORMAT_R8_SINT                     = 64,
	DDS_FORMAT_A8_UNORM                    = 65,
	DDS_FORMAT_R1_UNORM                    = 66,
	DDS_FORMAT_R9G9B9E5_SHAREDEXP          = 67,
	DDS_FORMAT_R8G8_B8G8_UNORM             = 68,
	DDS_FORMAT_G8R8_G8B8_UNORM             = 69,
	DDS_FORMAT_BC1_TYPELESS                = 70,
	DDS_FORMAT_BC1_UNORM                   = 71,
	DDS_FORMAT_BC1_UNORM_SRGB              = 72,
	DDS_FORMAT_BC2_TYPELESS                = 73,
	DDS_FORMAT_BC2_UNORM                   = 74,
	DDS_FORMAT_BC2_UNORM_SRGB              = 75,
	DDS_FORMAT_BC3_TYPELESS                = 76,
	DDS_FORMAT_BC3_UNORM                   = 77,
	DDS_FORMAT_BC3_UNORM_SRGB              = 78,
	DDS_FORMAT_BC4_TYPELESS                = 79,
	DDS_FORMAT_BC4_UNORM                   = 80,
	DDS_FORMAT_BC4_SNORM                   = 81,
	DDS_FORMAT_BC5_TYPELESS                = 82,
	DDS_FORMAT_BC5_UNORM                   = 83,
	DDS_FORMAT_BC5_SNORM                   = 84,
	DDS_FORMAT_B5G6R5_UNORM                = 85,
	DDS_FORMAT_B5G5R5A1_UNORM              = 86,
	DDS_FORMAT_B8G8R8A8_UNORM              = 87,
	DDS_FORMAT_B8G8R8X8_UNORM              = 88,
	DDS_FORMAT_R10G10B10_XR_BIAS_A2_UNORM  = 89,
	DDS_FORMAT_B8G8R8A8_TYPELESS           = 90,
	DDS_FORMAT_B8G8R8A8_UNORM_SRGB         = 91,
	DDS_FORMAT_B8G8R8X8_TYPELESS           = 92,
	DDS_FORMAT_B8G8R8X8_UNORM_SRGB         = 93,
	DDS_FORMAT_BC6H_TYPELESS               = 94,
	DDS_FORMAT_BC6H_UF16                   = 95,
	DDS_FORMAT_BC6H_SF16                   = 96,
	DDS_FORMAT_BC7_TYPELESS                = 97,
	DDS_FORMAT_BC7_UNORM                   = 98,
	DDS_FORMAT_BC7_UNORM_SRGB              = 99,
	DDS_FORMAT_AYUV                        = 100,
	DDS_FORMAT_Y410                        = 101,
	DDS_FORMAT_Y416                        = 102,
	DDS_FORMAT_NV12                        = 103,
	DDS_FORMAT_P010                        = 104,
	DDS_FORMAT_P016                        = 105,
	DDS_FORMAT_420_OPAQUE                  = 106,
	DDS_FORMAT_YUY2                        = 107,
	DDS_FORMAT_Y210                        = 108,
	DDS_FORMAT_Y216                        = 109,
	DDS_FORMAT_NV11                        = 110,
	DDS_FORMAT_AI44                        = 111,
	DDS_FORMAT_IA44                        = 112,
	DDS_FORMAT_P8                          = 113,
	DDS_FORMAT_A8P8                        = 114,
	DDS_FORMAT_B4G4R4A4_UNORM              = 115,
	DDS_FORMAT_FORCE_UINT                  = 0xffffffff,

	// Non-DXGI formats. Any loader will need to convert these for use by D3D.
	DDS_FORMAT_R8G8B8_TYPELESS             = 255,
	DDS_FORMAT_R8G8B8_UNORM                = 254,
	DDS_FORMAT_R8G8B8_UNORM_SRGB           = 253,
	DDS_FORMAT_B8G8R8_TYPELESS             = 252,
	DDS_FORMAT_B8G8R8_UNORM                = 251,
	DDS_FORMAT_B8G8R8_UNORM_SRGB           = 250,
};

enum DDS_RESOURCE_DIMENSION
{
	DDS_RESOURCE_DIMENSION_TEXTURE1D	= 2,
	DDS_RESOURCE_DIMENSION_TEXTURE2D	= 3,
	DDS_RESOURCE_DIMENSION_TEXTURE3D	= 4,
};

enum DDS_RESOURCE_MISC_FLAGS
{
	DDS_RESOURCE_MISC_TEXTURECUBE = 4,
};

#define DDS_FOURCC                  0x00000004
#define DDS_RGB                     0x00000040
#define DDS_RGBA                    0x00000041
#define DDS_LUMINANCE               0x00020000
#define DDS_LUMINANCEA              0x00020001
#define DDS_ALPHA                   0x00000002
#define DDS_PAL8                    0x00000020

#define DDS_HEADER_FLAGS_TEXTURE    0x00001007 // DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT 
#define DDS_HEADER_FLAGS_MIPMAP     0x00020000 // DDSD_MIPMAPCOUNT
#define DDS_HEADER_FLAGS_VOLUME     0x00800000 // DDSD_DEPTH
#define DDS_HEADER_FLAGS_PITCH      0x00000008 // DDSD_PITCH
#define DDS_HEADER_FLAGS_LINEARSIZE 0x00080000 // DDSD_LINEARSIZE

#define DDS_HEIGHT                  0x00000002 // DDSD_HEIGHT
#define DDS_WIDTH                   0x00000004 // DDSD_WIDTH

#define DDS_SURFACE_FLAGS_TEXTURE   0x00001000 // DDSCAPS_TEXTURE
#define DDS_SURFACE_FLAGS_MIPMAP    0x00400008 // DDSCAPS_COMPLEX | DDSCAPS_MIPMAP
#define DDS_SURFACE_FLAGS_COMPLEX   0x00000008 // DDSCAPS_COMPLEX
#define DDS_SURFACE_FLAGS_CUBEMAP   0x00000008 // DDSCAPS_COMPLEX

#define DDS_CUBEMAP_POSITIVEX       0x00000600 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEX
#define DDS_CUBEMAP_NEGATIVEX       0x00000a00 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEX
#define DDS_CUBEMAP_POSITIVEY       0x00001200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEY
#define DDS_CUBEMAP_NEGATIVEY       0x00002200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEY
#define DDS_CUBEMAP_POSITIVEZ       0x00004200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEZ
#define DDS_CUBEMAP_NEGATIVEZ       0x00008200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEZ

#define DDS_CUBEMAP                 0x00000200 // DDSCAPS2_CUBEMAP
#define DDS_FLAGS_VOLUME            0x00200000 // DDSCAPS2_VOLUME

#define DDS_CUBEMAP_ALLFACES ( DDS_CUBEMAP_POSITIVEX | DDS_CUBEMAP_NEGATIVEX |\
															 DDS_CUBEMAP_POSITIVEY | DDS_CUBEMAP_NEGATIVEY |\
															 DDS_CUBEMAP_POSITIVEZ | DDS_CUBEMAP_NEGATIVEZ )

#pragma pack(push,1)
struct dds_pixel_format
{
	uint32_t dwSize;
	uint32_t dwFlags;
	uint32_t dwFourCC;
	uint32_t dwRGBBitCount;
	uint32_t dwRBitMask;
	uint32_t dwGBitMask;
	uint32_t dwBBitMask;
	uint32_t dwABitMask;
};

struct dds_header
{
	uint32_t dwSize;
	uint32_t dwFlags;
	uint32_t dwHeight;
	uint32_t dwWidth;
	uint32_t dwPitchOrLinearSize;
	uint32_t dwDepth;
	uint32_t dwMipMapCount;
	uint32_t dwReserved1[11];
	dds_pixel_format ddspf;
	uint32_t dwCaps;
	uint32_t dwCaps2;
	uint32_t dwCaps3;
	uint32_t dwCaps4;
	uint32_t dwReserved2;
};

struct dds_header_dx10
{
	DDS_FORMAT dxgiFormat;
	DDS_RESOURCE_DIMENSION resourceDimension;
	uint32_t miscFlag;
	uint32_t arraySize;
	uint32_t miscFlags2;
};
#pragma pack(pop)

static_assert(sizeof(dds_header) == 124, "DDS Header size mismatch");
static_assert(sizeof(dds_header_dx10) == 20, "DDS DX10 Extended Header size mismatch");

#define ISBITMASK(r,g,b,a) (ddpf.dwRBitMask == r && ddpf.dwGBitMask == g && ddpf.dwBBitMask == b && ddpf.dwABitMask == a)

#define DDSPF_UNKNOWN           { sizeof(dds_pixel_format), DDS_FOURCC, MAKEFOURCC('D','X','1','0'), 0, 0, 0, 0, 0 }
#define DDSPF_BC1_UNORM         { sizeof(dds_pixel_format), DDS_FOURCC, MAKEFOURCC('D','X','T','1'), 0, 0, 0, 0, 0 }
//#define DDSPF_UNKNOWN         { sizeof(dds_pixel_format), DDS_FOURCC, MAKEFOURCC('D','X','T','2'), 0, 0, 0, 0, 0 }
#define DDSPF_BC2_UNORM         { sizeof(dds_pixel_format), DDS_FOURCC, MAKEFOURCC('D','X','T','3'), 0, 0, 0, 0, 0 }
//#define DDSPF_UNKNOWN         { sizeof(dds_pixel_format), DDS_FOURCC, MAKEFOURCC('D','X','T','4'), 0, 0, 0, 0, 0 }
#define DDSPF_BC3_UNORM         { sizeof(dds_pixel_format), DDS_FOURCC, MAKEFOURCC('D','X','T','5'), 0, 0, 0, 0, 0 }
#define DDSPF_BC4_UNORM         { sizeof(dds_pixel_format), DDS_FOURCC, MAKEFOURCC('B','C','4','U'), 0, 0, 0, 0, 0 }
#define DDSPF_BC4_SNORM         { sizeof(dds_pixel_format), DDS_FOURCC, MAKEFOURCC('B','C','4','S'), 0, 0, 0, 0, 0 }
#define DDSPF_BC5_UNORM         { sizeof(dds_pixel_format), DDS_FOURCC, MAKEFOURCC('B','C','5','U'), 0, 0, 0, 0, 0 }
#define DDSPF_BC5_SNORM         { sizeof(dds_pixel_format), DDS_FOURCC, MAKEFOURCC('B','C','5','S'), 0, 0, 0, 0, 0 }
#define DDSPF_R8G8_B8G8_UNORM   { sizeof(dds_pixel_format), DDS_FOURCC, MAKEFOURCC('R','G','B','G'), 0, 0, 0, 0, 0 }
#define DDSPF_G8R8_G8B8_UNORM   { sizeof(dds_pixel_format), DDS_FOURCC, MAKEFOURCC('G','R','G','B'), 0, 0, 0, 0, 0 }
#define DDSPF_YUY2              { sizeof(dds_pixel_format), DDS_FOURCC, MAKEFOURCC('Y','U','Y','2'), 0, 0, 0, 0, 0 }
#define DDSPF_B8G8R8A8_UNORM    { sizeof(dds_pixel_format), DDS_RGBA, 0, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 }
#define DDSPF_B8G8R8X8_UNORM    { sizeof(dds_pixel_format), DDS_RGB,  0, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000 }
#define DDSPF_R8G8B8A8_UNORM    { sizeof(dds_pixel_format), DDS_RGBA, 0, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000 }
#define DDSPF_R8G8B8_UNORM      { sizeof(dds_pixel_format), DDS_RGB,  0, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0x00000000 }
#define DDSPF_R16G16_UNORM      { sizeof(dds_pixel_format), DDS_RGB,  0, 32, 0x0000ffff, 0xffff0000, 0x00000000, 0x00000000 }
#define DDSPF_B5G6R5_UNORM      { sizeof(dds_pixel_format), DDS_RGB,  0, 16, 0x0000f800, 0x000007e0, 0x0000001f, 0x00000000 }
#define DDSPF_B5G5R5A1_UNORM    { sizeof(dds_pixel_format), DDS_RGBA, 0, 16, 0x00007c00, 0x000003e0, 0x0000001f, 0x00008000 }
#define DDSPF_B4G4R4A4_UNORM    { sizeof(dds_pixel_format), DDS_RGBA, 0, 16, 0x00000f00, 0x000000f0, 0x0000000f, 0x0000f000 }
#define DDSPF_B8G8R8_UNORM      { sizeof(dds_pixel_format), DDS_RGB,  0, 24, 0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000 }
#define DDSPF_R8_UNORM          { sizeof(dds_pixel_format), DDS_LUMINANCE, 0,  8, 0xff, 0x00, 0x00, 0x00 }
#define DDSPF_R16_UNORM         { sizeof(dds_pixel_format), DDS_LUMINANCE, 0, 16, 0xffff, 0x0000, 0x0000, 0x0000 }
#define DDSPF_R8G8_UNORM        { sizeof(dds_pixel_format), DDS_LUMINANCEA, 0, 16, 0x00ff, 0x0000, 0x0000, 0xff00 }
#define DDSPF_A8_UNORM          { sizeof(dds_pixel_format), DDS_ALPHA, 0, 8, 0x00, 0x00, 0x00, 0xff }

inline bool has_dx10_header(const dds_header& h)
{
	return (h.ddspf.dwFlags & DDS_FOURCC) && (MAKEFOURCC('D','X','1','0') == h.ddspf.dwFourCC);
}

inline DDS_FORMAT from_ddspf(const dds_pixel_format& ddpf)
{
	if (ddpf.dwFlags & DDS_RGB)
	{
		// sRGB formats are written using the "DX10" extended header
		switch (ddpf.dwRGBBitCount)
		{
			case 24:
				if (ISBITMASK(0x000000ff,0x0000ff00,0x00ff0000,0x00000000)) return DDS_FORMAT_R8G8B8_UNORM;
				if (ISBITMASK(0x00ff0000,0x0000ff00,0x000000ff,0x00000000)) return DDS_FORMAT_B8G8R8_UNORM;
				break;

			case 32:
				if (ISBITMASK(0x000000ff,0x0000ff00,0x00ff0000,0xff000000)) return DDS_FORMAT_R8G8B8A8_UNORM;
				if (ISBITMASK(0x00ff0000,0x0000ff00,0x000000ff,0xff000000)) return DDS_FORMAT_B8G8R8A8_UNORM;
				if (ISBITMASK(0x00ff0000,0x0000ff00,0x000000ff,0x00000000)) return DDS_FORMAT_B8G8R8X8_UNORM;
				// Many common DDS reader/writers (including D3DX) swap the
				// the RED/BLUE masks for 10:10:10:2 formats. We assume
				// below that the 'backwards' header mask is being used since it is most
				// likely written by D3DX. The more robust solution is to use the 'DX10'
				// header extension and specify the DDS_FORMAT_R10G10B10A2_UNORM format directly
				// For 'correct' writers, this should be 0x000003ff,0x000ffc00,0x3ff00000 for RGB data
				if (ISBITMASK(0x3ff00000,0x000ffc00,0x000003ff,0xc0000000)) return DDS_FORMAT_R10G10B10A2_UNORM;
				// No DXGI format maps to ISBITMASK(0x000003ff,0x000ffc00,0x3ff00000,0xc0000000) aka D3DFMT_A2R10G10B10
				if (ISBITMASK(0x0000ffff,0xffff0000,0x00000000,0x00000000))	return DDS_FORMAT_R16G16_UNORM;
				if (ISBITMASK(0xffffffff,0x00000000,0x00000000,0x00000000)) return DDS_FORMAT_R32_FLOAT; // D3DX writes this out as a FourCC of 114 (Only 32-bit color channel format in D3D9 was R32F)
				break;

			case 16:
				if (ISBITMASK(0x7c00,0x03e0,0x001f,0x8000)) return DDS_FORMAT_B5G5R5A1_UNORM;
				if (ISBITMASK(0xf800,0x07e0,0x001f,0x0000)) return DDS_FORMAT_B5G6R5_UNORM;
				// No DXGI format maps to ISBITMASK(0x7c00,0x03e0,0x001f,0x0000) aka D3DFMT_X1R5G5B5
				if (ISBITMASK(0x0f00,0x00f0,0x000f,0xf000)) return DDS_FORMAT_B4G4R4A4_UNORM;
				// No DXGI format maps to ISBITMASK(0x0f00,0x00f0,0x000f,0x0000) aka D3DFMT_X4R4G4B4
				// No 3:3:2, 3:3:2:8, or paletted DXGI formats aka D3DFMT_A8R3G3B2, D3DFMT_R3G3B2, D3DFMT_P8, D3DFMT_A8P8, etc.
				break;
		}
	}
	
	else if (ddpf.dwFlags & DDS_LUMINANCE)
	{
		if (8 == ddpf.dwRGBBitCount)
		{
			if (ISBITMASK(0x000000ff,0x00000000,0x00000000,0x00000000)) return DDS_FORMAT_R8_UNORM; // D3DX10/11 writes this out as DX10 extension
			// No DXGI format maps to ISBITMASK(0x0f,0x00,0x00,0xf0) aka D3DFMT_A4L4
		}

		if (16 == ddpf.dwRGBBitCount)
		{
			if (ISBITMASK(0x0000ffff,0x00000000,0x00000000,0x00000000)) return DDS_FORMAT_R16_UNORM; // D3DX10/11 writes this out as DX10 extension
			if (ISBITMASK(0x000000ff,0x00000000,0x00000000,0x0000ff00)) return DDS_FORMAT_R8G8_UNORM; // D3DX10/11 writes this out as DX10 extension
		}
	}

	else if (ddpf.dwFlags & DDS_ALPHA)
	{
		if (8 == ddpf.dwRGBBitCount) return DDS_FORMAT_A8_UNORM;
	}

	else if (ddpf.dwFlags & DDS_FOURCC)
	{
		if (MAKEFOURCC('D','X','T','1') == ddpf.dwFourCC) return DDS_FORMAT_BC1_UNORM;
		if (MAKEFOURCC('D','X','T','3') == ddpf.dwFourCC) return DDS_FORMAT_BC2_UNORM;
		if (MAKEFOURCC('D','X','T','5') == ddpf.dwFourCC) return DDS_FORMAT_BC3_UNORM;
		// While pre-mulitplied alpha isn't directly supported by the DXGI formats,
		// they are basically the same as these BC formats so they can be mapped
		if (MAKEFOURCC('D','X','T','2') == ddpf.dwFourCC) return DDS_FORMAT_BC2_UNORM;
		if (MAKEFOURCC('D','X','T','4') == ddpf.dwFourCC) return DDS_FORMAT_BC3_UNORM;
		if (MAKEFOURCC('A','T','I','1') == ddpf.dwFourCC) return DDS_FORMAT_BC4_UNORM;
		if (MAKEFOURCC('B','C','4','U') == ddpf.dwFourCC) return DDS_FORMAT_BC4_UNORM;
		if (MAKEFOURCC('B','C','4','S') == ddpf.dwFourCC) return DDS_FORMAT_BC4_SNORM;
		if (MAKEFOURCC('A','T','I','2') == ddpf.dwFourCC) return DDS_FORMAT_BC5_UNORM;
		if (MAKEFOURCC('B','C','5','U') == ddpf.dwFourCC) return DDS_FORMAT_BC5_UNORM;
		if (MAKEFOURCC('B','C','5','S') == ddpf.dwFourCC) return DDS_FORMAT_BC5_SNORM;
		// BC6H and BC7 are written using the "DX10" extended header
		if (MAKEFOURCC('R','G','B','G') == ddpf.dwFourCC) return DDS_FORMAT_R8G8_B8G8_UNORM;
		if (MAKEFOURCC('G','R','G','B') == ddpf.dwFourCC) return DDS_FORMAT_G8R8_G8B8_UNORM;
		if (MAKEFOURCC('Y','U','Y','2') == ddpf.dwFourCC) return DDS_FORMAT_YUY2;

	// Check for D3DFORMAT enums being set here
		switch (ddpf.dwFourCC)
		{
			case 36: return DDS_FORMAT_R16G16B16A16_UNORM; // D3DFMT_A16B16G16R16
			case 110: return DDS_FORMAT_R16G16B16A16_SNORM; // D3DFMT_Q16W16V16U16
			case 111: return DDS_FORMAT_R16_FLOAT; // D3DFMT_R16F
			case 112: return DDS_FORMAT_R16G16_FLOAT; // D3DFMT_G16R16F
			case 113: return DDS_FORMAT_R16G16B16A16_FLOAT; // D3DFMT_A16B16G16R16F
			case 114: return DDS_FORMAT_R32_FLOAT; // D3DFMT_R32F
			case 115: return DDS_FORMAT_R32G32_FLOAT; // D3DFMT_G32R32F
			case 116: return DDS_FORMAT_R32G32B32A32_FLOAT; // D3DFMT_A32B32G32R32F
		}
	}

	return DDS_FORMAT_UNKNOWN;
}

inline dds_pixel_format to_ddspf(const DDS_FORMAT& format)
{
	struct LUT { DDS_FORMAT format; dds_pixel_format ddsformat; };
	static const LUT sFormats[] = 
	{
		{ DDS_FORMAT_UNKNOWN,         DDSPF_UNKNOWN         }, { DDS_FORMAT_BC1_UNORM,       DDSPF_BC1_UNORM       },
		{ DDS_FORMAT_BC2_UNORM,       DDSPF_BC2_UNORM       }, { DDS_FORMAT_BC3_UNORM,       DDSPF_BC3_UNORM       },
		{ DDS_FORMAT_BC4_UNORM,       DDSPF_BC4_UNORM       }, { DDS_FORMAT_BC4_SNORM,       DDSPF_BC4_SNORM       },
		{ DDS_FORMAT_BC5_UNORM,       DDSPF_BC5_UNORM       }, { DDS_FORMAT_BC5_SNORM,       DDSPF_BC5_SNORM       },
		{ DDS_FORMAT_R8G8_B8G8_UNORM, DDSPF_R8G8_B8G8_UNORM }, { DDS_FORMAT_G8R8_G8B8_UNORM, DDSPF_G8R8_G8B8_UNORM },
		{ DDS_FORMAT_YUY2,            DDSPF_YUY2            }, { DDS_FORMAT_B8G8R8A8_UNORM,  DDSPF_B8G8R8A8_UNORM  },
		{ DDS_FORMAT_B8G8R8X8_UNORM,  DDSPF_B8G8R8X8_UNORM  }, { DDS_FORMAT_R8G8B8A8_UNORM,  DDSPF_R8G8B8A8_UNORM  },
		{ DDS_FORMAT_R16G16_UNORM,    DDSPF_R16G16_UNORM    }, { DDS_FORMAT_B5G6R5_UNORM,    DDSPF_B5G6R5_UNORM    },
		{ DDS_FORMAT_B5G5R5A1_UNORM,  DDSPF_B5G5R5A1_UNORM  }, { DDS_FORMAT_B4G4R4A4_UNORM,  DDSPF_B4G4R4A4_UNORM  },
		{ DDS_FORMAT_R8_UNORM,        DDSPF_R8_UNORM        }, { DDS_FORMAT_R16_UNORM,       DDSPF_R16_UNORM       },
		{ DDS_FORMAT_R8G8_UNORM,      DDSPF_R8G8_UNORM      }, { DDS_FORMAT_A8_UNORM,        DDSPF_A8_UNORM        },
	};

	for (const auto& lut : sFormats)
		if (format == lut.format)
			return lut.ddsformat;
	return sFormats[0].ddsformat;
}

inline bool dds_is_block_compressed(DDS_FORMAT format)
{
	return format >= DDS_FORMAT_BC1_TYPELESS && (format <= DDS_FORMAT_BC5_SNORM 
		|| (format >= DDS_FORMAT_BC6H_TYPELESS && format <= DDS_FORMAT_BC7_UNORM_SRGB));
}

inline uint32_t dds_element_size(DDS_FORMAT format)
{
	switch (format)
	{
		case DDS_FORMAT_R32G32B32A32_TYPELESS: case DDS_FORMAT_R32G32B32A32_FLOAT: case DDS_FORMAT_R32G32B32A32_UINT: case DDS_FORMAT_R32G32B32A32_SINT:
		case DDS_FORMAT_BC2_TYPELESS: case DDS_FORMAT_BC2_UNORM: case DDS_FORMAT_BC2_UNORM_SRGB:
		case DDS_FORMAT_BC3_TYPELESS: case DDS_FORMAT_BC3_UNORM: case DDS_FORMAT_BC3_UNORM_SRGB:
		case DDS_FORMAT_BC5_TYPELESS: case DDS_FORMAT_BC5_UNORM: case DDS_FORMAT_BC5_SNORM:
		case DDS_FORMAT_BC6H_TYPELESS: case DDS_FORMAT_BC6H_UF16: case DDS_FORMAT_BC6H_SF16:
		case DDS_FORMAT_BC7_TYPELESS: case DDS_FORMAT_BC7_UNORM: case DDS_FORMAT_BC7_UNORM_SRGB:
			return 16;
		case DDS_FORMAT_R32G32B32_TYPELESS: case DDS_FORMAT_R32G32B32_FLOAT: case DDS_FORMAT_R32G32B32_UINT: case DDS_FORMAT_R32G32B32_SINT:
			return 12;
		case DDS_FORMAT_R16G16B16A16_TYPELESS: case DDS_FORMAT_R16G16B16A16_FLOAT: case DDS_FORMAT_R16G16B16A16_UNORM: case DDS_FORMAT_R16G16B16A16_UINT: case DDS_FORMAT_R16G16B16A16_SNORM: case DDS_FORMAT_R16G16B16A16_SINT:
		case DDS_FORMAT_R32G32_TYPELESS: case DDS_FORMAT_R32G32_FLOAT: case DDS_FORMAT_R32G32_UINT: case DDS_FORMAT_R32G32_SINT:
		case DDS_FORMAT_BC1_TYPELESS: case DDS_FORMAT_BC1_UNORM: case DDS_FORMAT_BC1_UNORM_SRGB:
		case DDS_FORMAT_BC4_TYPELESS: case DDS_FORMAT_BC4_UNORM: case DDS_FORMAT_BC4_SNORM:
			return 8;
		case DDS_FORMAT_R8G8B8A8_TYPELESS: case DDS_FORMAT_R8G8B8A8_UNORM: case DDS_FORMAT_R8G8B8A8_UNORM_SRGB: case DDS_FORMAT_R8G8B8A8_UINT: case DDS_FORMAT_R8G8B8A8_SNORM: case DDS_FORMAT_R8G8B8A8_SINT:
		case DDS_FORMAT_R16G16_TYPELESS: case DDS_FORMAT_R16G16_FLOAT: case DDS_FORMAT_R16G16_UNORM: case DDS_FORMAT_R16G16_UINT: case DDS_FORMAT_R16G16_SNORM: case DDS_FORMAT_R16G16_SINT:
		case DDS_FORMAT_R32_TYPELESS: case DDS_FORMAT_D32_FLOAT: case DDS_FORMAT_R32_FLOAT: case DDS_FORMAT_R32_UINT: case DDS_FORMAT_R32_SINT:
		case DDS_FORMAT_B8G8R8A8_UNORM: case DDS_FORMAT_B8G8R8X8_UNORM:
		case DDS_FORMAT_R32G8X24_TYPELESS: case DDS_FORMAT_D32_FLOAT_S8X24_UINT: case DDS_FORMAT_R32_FLOAT_X8X24_TYPELESS: case DDS_FORMAT_X32_TYPELESS_G8X24_UINT: case DDS_FORMAT_R24G8_TYPELESS: case DDS_FORMAT_D24_UNORM_S8_UINT: case DDS_FORMAT_R24_UNORM_X8_TYPELESS: case DDS_FORMAT_X24_TYPELESS_G8_UINT:
		case DDS_FORMAT_R10G10B10A2_TYPELESS: case DDS_FORMAT_R10G10B10A2_UNORM: case DDS_FORMAT_R10G10B10A2_UINT: case DDS_FORMAT_R11G11B10_FLOAT: case DDS_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
		case DDS_FORMAT_B8G8R8A8_TYPELESS: case DDS_FORMAT_B8G8R8A8_UNORM_SRGB: case DDS_FORMAT_B8G8R8X8_TYPELESS: case DDS_FORMAT_B8G8R8X8_UNORM_SRGB:
		case DDS_FORMAT_R9G9B9E5_SHAREDEXP:
		case DDS_FORMAT_R8G8_B8G8_UNORM: case DDS_FORMAT_G8R8_G8B8_UNORM:
		case DDS_FORMAT_AYUV: case DDS_FORMAT_Y410: case DDS_FORMAT_Y416: case DDS_FORMAT_YUY2:
			return 4;

		case DDS_FORMAT_R8G8_TYPELESS: case DDS_FORMAT_R8G8_UNORM: case DDS_FORMAT_R8G8_UINT: case DDS_FORMAT_R8G8_SNORM: case DDS_FORMAT_R8G8_SINT:
		case DDS_FORMAT_R16_TYPELESS: case DDS_FORMAT_R16_FLOAT: case DDS_FORMAT_D16_UNORM: case DDS_FORMAT_R16_UNORM: case DDS_FORMAT_R16_UINT: case DDS_FORMAT_R16_SNORM: case DDS_FORMAT_R16_SINT:
		case DDS_FORMAT_B5G6R5_UNORM: case DDS_FORMAT_B5G5R5A1_UNORM:
		case DDS_FORMAT_A8P8:
		case DDS_FORMAT_B4G4R4A4_UNORM:
		case DDS_FORMAT_P010: case DDS_FORMAT_P016: case DDS_FORMAT_Y210: case DDS_FORMAT_Y216: case DDS_FORMAT_NV12:
			return 2;
		
		case DDS_FORMAT_R8_TYPELESS: case DDS_FORMAT_R8_UNORM: case DDS_FORMAT_R8_UINT: case DDS_FORMAT_R8_SNORM: case DDS_FORMAT_R8_SINT: case DDS_FORMAT_A8_UNORM:
		case DDS_FORMAT_P8:
		case DDS_FORMAT_AI44: case DDS_FORMAT_IA44: case DDS_FORMAT_420_OPAQUE:
		case DDS_FORMAT_NV11:
			return 1;
		case DDS_FORMAT_R1_UNORM:
		default: break;
	}
	return 0;
}

inline uint32_t dds_calc_pitch(DDS_FORMAT format, uint32_t pixel_width)
{
	// http://msdn.microsoft.com/en-us/library/windows/desktop/bb943991(v=vs.85).aspx#related_topics
	uint32_t pitch = 0;
	const uint32_t elsize = dds_element_size(format);
	const uint32_t elbits = format == DDS_FORMAT_R1_UNORM ? 1 : (elsize * 8);
	if (dds_is_block_compressed(format))
		pitch = max(1u, ((pixel_width + 3) / 4)) * elsize;
	else if (format == DDS_FORMAT_R8G8_B8G8_UNORM || format == DDS_FORMAT_G8R8_G8B8_UNORM)
		pitch = ((pixel_width + 1) >> 1) * 4;
	else 
		pitch = (pixel_width * elbits + 7 ) / 8;
	return pitch;
}
