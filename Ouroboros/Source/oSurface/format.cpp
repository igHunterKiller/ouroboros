// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oSurface/format.h>
#include <oCore/countof.h>
#include <oString/string.h>

namespace ouro { namespace surface {

struct subformats { format format[4]; };

namespace traits
{	enum flag : int16_t {

	none = 0,

	is_bc             = 1 << 0,
	is_unorm          = 1 << 1,
	is_srgb           = 1 << 2,
	has_alpha         = 1 << 3,
	is_depth          = 1 << 4,
	is_typeless       = 1 << 5,
	is_planar         = 1 << 6,
	is_yuv            = 1 << 7,
	paletted          = 1 << 8,
	subsurface1_bias1 = 1 << 9,
	subsurface2_bias1 = 1 << 10,
	subsurface3_bias1 = 1 << 11,

};}

struct format_info
{
	const char* string;
	fourcc_t fourcc;
	struct bit_size bit_size;
	struct subformats subformats;
	uint16_t element_size : 5; // [0,16] bytes per pixel or block
	uint16_t num_channels : 3; // [0,4]
	uint16_t num_subformats : 3; // [1,4]
	int16_t traits;
};

static const uint16_t kMinMip = 1;
static const uint16_t kMinMipBC = 4;
static const uint16_t kMinMipYUV = 2;

static const bit_size kUnknownBits = {0,0,0,0};
static const bit_size kBS_4_32     = {32,32,32,32};
static const bit_size kBS_3_32     = {32,32,32,0};
static const bit_size kBS_2_32     = {32,32,0,0};
static const bit_size kBS_1_32     = {32,0,0,0};
static const bit_size kBS_4_16     = {16,16,16,16};
static const bit_size kBS_3_16     = {16,16,16,0};
static const bit_size kBS_2_16     = {16,16,0,0};
static const bit_size kBS_1_16     = {16,0,0,0};
static const bit_size kBS_4_8      = {8,8,8,8};
static const bit_size kBS_3_8      = {8,8,8,0};
static const bit_size kBS_2_8      = {8,8,0,0};
static const bit_size kBS_1_8      = {8,0,0,0};
static const bit_size kBS_565      = {5,6,5,0};
static const bit_size kBS_DEC3N    = {10,10,10,2};
static const bit_size kBS_DS       = {24,8,0,0};
static const bit_size kBS_4_4      = {4,4,4,4};
static const bit_size kBS_3_4      = {4,4,4,0};
static const bit_size kBS_2_4      = {4,4,0,0};
static const bit_size kBS_1_4      = {4,0,0,0};

static const fourcc_t kFCC_Unknown = oFCC('????');

static const subformats kNoSubformats = {format::unknown,    format::unknown,      format::unknown,   format::unknown};
static const subformats kSFD_R8_R8    = {format::r8_unorm,   format::r8_unorm,     format::unknown,   format::unknown};
static const subformats kSFD_R8_RG8   = {format::r8_unorm,   format::r8g8_unorm,   format::unknown,   format::unknown};
static const subformats kSFD_RG8_RG8  = {format::r8g8_unorm, format::r8g8_unorm,   format::unknown,   format::unknown};
static const subformats kSFD_R16_RG16 = {format::r16_unorm,  format::r16g16_unorm, format::unknown,   format::unknown};
static const subformats kSFD_BC4_BC4  = {format::bc4_unorm,  format::bc4_unorm,    format::unknown,   format::unknown};
static const subformats kSFD_BC4_BC5  = {format::bc4_unorm,  format::bc5_unorm,    format::unknown,   format::unknown};
static const subformats kSFD_BC5_BC5  = {format::bc5_unorm,  format::bc5_unorm,    format::unknown,   format::unknown};
static const subformats kSFD_R8_4     = {format::r8_unorm,   format::r8_unorm,     format::r8_unorm,  format::r8_unorm};
static const subformats kSFD_R8_3     = {format::r8_unorm,   format::r8_unorm,     format::r8_unorm,  format::unknown};
static const subformats kSFD_BC4_4    = {format::bc4_unorm,  format::bc4_unorm,    format::bc4_unorm, format::bc4_unorm};
static const subformats kSFD_BC4_3    = {format::bc4_unorm,  format::bc4_unorm,    format::bc4_unorm, format::unknown};

#define FPERM(srgb,depth,typeless,unorm,x,a) { format::srgb, format::depth, format::typeless, format::a8_unorm, format::x, format::a }

static const format_info sFormatInfo[] = 
{
  { "unknown",                    kFCC_Unknown, kUnknownBits, kNoSubformats,    0, 0, 0, traits::none },
  { "r32g32b32a32_typeless",      oFCC('?i4 '), kBS_4_32,     kNoSubformats,   16, 4, 1, traits::has_alpha|traits::is_typeless },
  { "r32g32b32a32_float",         oFCC('f4  '), kBS_4_32,     kNoSubformats,   16, 4, 1, traits::has_alpha },
  { "r32g32b32a32_uint",          oFCC('ui4 '), kBS_4_32,     kNoSubformats,   16, 4, 1, traits::has_alpha },
  { "r32g32b32a32_sint",          oFCC('si4 '), kBS_4_32,     kNoSubformats,   16, 4, 1, traits::has_alpha },
  { "r32g32b32_typeless",         oFCC('?i3 '), kBS_3_32,     kNoSubformats,   12, 3, 1, traits::has_alpha|traits::is_typeless },
  { "r32g32b32_float",            oFCC('f3  '), kBS_3_32,     kNoSubformats,   12, 3, 1, traits::none },
  { "r32g32b32_uint",             oFCC('ui3 '), kBS_3_32,     kNoSubformats,   12, 3, 1, traits::none },
  { "r32g32b32_sint",             oFCC('si3 '), kBS_3_32,     kNoSubformats,   12, 3, 1, traits::none },
  { "r16g16b16a16_typeless",      oFCC('?s4 '), kBS_4_16,     kNoSubformats,    8, 4, 1, traits::has_alpha|traits::is_typeless },
  { "r16g16b16a16_float",         oFCC('h4  '), kBS_4_16,     kNoSubformats,    8, 4, 1, traits::has_alpha },
  { "r16g16b16a16_unorm",         oFCC('h4u '), kBS_4_16,     kNoSubformats,    8, 4, 1, traits::is_unorm|traits::has_alpha },
  { "r16g16b16a16_uint",          oFCC('us4 '), kBS_4_16,     kNoSubformats,    8, 4, 1, traits::has_alpha },
  { "r16g16b16a16_snorm",         oFCC('h4s '), kBS_4_16,     kNoSubformats,    8, 4, 1, traits::has_alpha },
  { "r16g16b16a16_sint",          oFCC('ss4 '), kBS_4_16,     kNoSubformats,    8, 4, 1, traits::has_alpha },
  { "r32g32_typeless",            oFCC('?i2 '), kBS_2_32,     kNoSubformats,    8, 2, 1, traits::is_typeless },
  { "r32g32_float",               oFCC('f2  '), kBS_2_32,     kNoSubformats,    8, 2, 1, traits::none },
  { "r32g32_uint",                oFCC('ui2 '), kBS_2_32,     kNoSubformats,    8, 2, 1, traits::none },
  { "r32g32_sint",                oFCC('si2 '), kBS_2_32,     kNoSubformats,    8, 2, 1, traits::none },
  { "r32g8x24_typeless",          kFCC_Unknown, {32,8,0,24},  kNoSubformats,    8, 3, 1, traits::is_typeless },
  { "d32_float_s8x24_uint",       kFCC_Unknown, {32,8,0,24},  kNoSubformats,    8, 3, 1, traits::is_depth },
  { "r32_float_x8x24_typeless",   kFCC_Unknown, {32,8,0,24},  kNoSubformats,    8, 3, 1, traits::is_depth|traits::is_typeless },
  { "x32_typeless_g8x24_uint",    kFCC_Unknown, {32,8,0,24},  kNoSubformats,    8, 3, 1, traits::none },
  { "r10g10b10a2_typeless",       kFCC_Unknown, kBS_DEC3N,    kNoSubformats,    4, 4, 1, traits::has_alpha|traits::is_typeless },
  { "r10g10b10a2_unorm",          kFCC_Unknown, kBS_DEC3N,    kNoSubformats,    4, 4, 1, traits::is_unorm|traits::has_alpha },
  { "r10g10b10a2_uint",           kFCC_Unknown, kBS_DEC3N,    kNoSubformats,    4, 4, 1, traits::has_alpha },
  { "r11g11b10_float",            kFCC_Unknown, {11,11,10,0}, kNoSubformats,    4, 3, 1, traits::is_typeless },
  { "r8g8b8a8_typeless",          oFCC('?c4 '), kBS_4_8,      kNoSubformats,    4, 4, 1, traits::has_alpha },
  { "r8g8b8a8_unorm",             oFCC('c4u '), kBS_4_8,      kNoSubformats,    4, 4, 1, traits::is_unorm|traits::has_alpha },
  { "r8g8b8a8_unorm_srgb",        oFCC('c4us'), kBS_4_8,      kNoSubformats,    4, 4, 1, traits::is_unorm|traits::is_srgb|traits::has_alpha },
  { "r8g8b8a8_uint",              oFCC('uc4 '), kBS_4_8,      kNoSubformats,    4, 4, 1, traits::has_alpha },
  { "r8g8b8a8_snorm",             oFCC('c4s '), kBS_4_8,      kNoSubformats,    4, 4, 1, traits::has_alpha },
  { "r8g8b8a8_sint",              oFCC('sc4 '), kBS_4_8,      kNoSubformats,    4, 4, 1, traits::has_alpha },
  { "r16g16_typeless",            oFCC('?s2 '), kBS_2_16,     kNoSubformats,    4, 2, 1, traits::is_typeless },
  { "r16g16_float",               oFCC('h2  '), kBS_2_16,     kNoSubformats,    4, 2, 1, traits::none },
  { "r16g16_unorm",               oFCC('h2u '), kBS_2_16,     kNoSubformats,    4, 2, 1, traits::is_unorm },
  { "r16g16_uint",                oFCC('us2 '), kBS_2_16,     kNoSubformats,    4, 2, 1, traits::none },
  { "r16g16_snorm",               oFCC('h2s '), kBS_2_16,     kNoSubformats,    4, 2, 1, traits::none },
  { "r16g16_sint",                oFCC('ss2 '), kBS_2_16,     kNoSubformats,    4, 2, 1, traits::none },
  { "r32_typeless",               oFCC('?i1 '), kBS_1_32,     kNoSubformats,    4, 1, 1, traits::is_depth|traits::is_typeless },
  { "d32_float",                  oFCC('f1d '), kBS_1_32,     kNoSubformats,    4, 1, 1, traits::is_depth },
  { "r32_float",                  oFCC('f1  '), kBS_1_32,     kNoSubformats,    4, 1, 1, traits::none },
  { "r32_uint",                   oFCC('ui1 '), kBS_1_32,     kNoSubformats,    4, 1, 1, traits::none },
  { "r32_sint",                   oFCC('si1 '), kBS_1_32,     kNoSubformats,    4, 1, 1, traits::none },
  { "r24g8_typeless",             kFCC_Unknown, kBS_DS,       kNoSubformats,    4, 2, 1, traits::is_depth|traits::is_typeless },
  { "d24_unorm_s8_uint",          kFCC_Unknown, kBS_DS,       kNoSubformats,    4, 2, 1, traits::is_unorm|traits::is_depth },
  { "r24_unorm_x8_typeless",      kFCC_Unknown, kBS_DS,       kNoSubformats,    4, 2, 1, traits::is_unorm|traits::is_depth },
  { "x24_typeless_g8_uint",       kFCC_Unknown, kBS_DS,       kNoSubformats,    4, 2, 1, traits::is_depth },
  { "r8g8_typeless",              oFCC('?c2 '), kBS_2_8,      kNoSubformats,    2, 2, 1, traits::is_typeless },
  { "r8g8_unorm",                 oFCC('uc2u'), kBS_2_8,      kNoSubformats,    2, 2, 1, traits::is_unorm },
  { "r8g8_uint",                  oFCC('ui2 '), kBS_2_8,      kNoSubformats,    2, 2, 1, traits::none },
  { "r8g8_snorm",                 oFCC('uc2s'), kBS_2_8,      kNoSubformats,    2, 2, 1, traits::none },
  { "r8g8_sint",                  oFCC('si2 '), kBS_2_8,      kNoSubformats,    2, 2, 1, traits::none },
  { "r16_typeless",               oFCC('?s1 '), kBS_1_16,     kNoSubformats,    2, 1, 1, traits::is_depth|traits::is_typeless },
  { "r16_float",                  oFCC('h1  '), kBS_1_16,     kNoSubformats,    2, 1, 1, traits::none },
  { "d16_unorm",                  oFCC('h1ud'), kBS_1_16,     kNoSubformats,    2, 1, 1, traits::is_unorm|traits::is_depth },
  { "r16_unorm",                  oFCC('h1u '), kBS_1_16,     kNoSubformats,    2, 1, 1, traits::is_unorm },
  { "r16_uint",                   oFCC('us1 '), kBS_1_16,     kNoSubformats,    2, 1, 1, traits::none },
  { "r16_snorm",                  oFCC('h1s '), kBS_1_16,     kNoSubformats,    2, 1, 1, traits::none },
  { "r16_sint",                   oFCC('ss1 '), kBS_1_16,     kNoSubformats,    2, 1, 1, traits::none },
  { "r8_typeless",                oFCC('?c1 '), kBS_1_8,      kNoSubformats,    1, 1, 1, traits::is_typeless },
  { "r8_unorm",                   oFCC('uc1u'), kBS_1_8,      kNoSubformats,    1, 1, 1, traits::is_unorm },
  { "r8_uint",                    oFCC('uc1 '), kBS_1_8,      kNoSubformats,    1, 1, 1, traits::none },
  { "r8_snorm",                   oFCC('uc1s'), kBS_1_8,      kNoSubformats,    1, 1, 1, traits::none },
  { "r8_sint",                    oFCC('sc1 '), kBS_1_8,      kNoSubformats,    1, 1, 1, traits::none },
  { "a8_unorm",                   oFCC('ac1u'), kBS_1_8,      kNoSubformats,    1, 1, 1, traits::is_unorm|traits::has_alpha },
  { "r1_unorm",                   oFCC('bitu'), {1,0,0,0},    kNoSubformats,    1, 1, 1, traits::is_unorm },
  { "r9g9b9e5_sharedexp",         kFCC_Unknown, {9,9,9,5},    kNoSubformats,    4, 4, 1, traits::none },
  { "r8g8_b8g8_unorm",            kFCC_Unknown, kBS_4_8,      kNoSubformats,    4, 4, 1, traits::is_unorm },
  { "g8r8_g8b8_unorm",            kFCC_Unknown, kBS_4_8,      kNoSubformats,    4, 4, 1, traits::is_unorm },
  { "bc1_typeless",               oFCC('BC1?'), kBS_565,      kNoSubformats,    8, 3, 1, traits::is_bc|traits::is_typeless },
  { "bc1_unorm",                  oFCC('BC1u'), kBS_565,      kNoSubformats,    8, 3, 1, traits::is_bc|traits::is_unorm },
  { "bc1_unorm_srgb",             oFCC('BC1s'), kBS_565,      kNoSubformats,    8, 3, 1, traits::is_bc|traits::is_unorm|traits::is_srgb },
  { "bc2_typeless",               oFCC('BC2?'), {5,6,5,4},    kNoSubformats,   16, 4, 1, traits::is_bc|traits::has_alpha|traits::is_typeless },
  { "bc2_unorm",                  oFCC('BC2u'), {5,6,5,4},    kNoSubformats,   16, 4, 1, traits::is_bc|traits::is_unorm|traits::has_alpha },
  { "bc2_unorm_srgb",             oFCC('BC2s'), {5,6,5,4},    kNoSubformats,   16, 4, 1, traits::is_bc|traits::is_unorm|traits::is_srgb|traits::has_alpha },
  { "bc3_typeless",               oFCC('BC3?'), {5,6,5,8},    kNoSubformats,   16, 4, 1, traits::is_bc|traits::has_alpha|traits::is_typeless },
  { "bc3_unorm",                  oFCC('BC3u'), {5,6,5,8},    kNoSubformats,   16, 4, 1, traits::is_bc|traits::is_unorm|traits::has_alpha },
  { "bc3_unorm_srgb",             oFCC('BC3s'), {5,6,5,8},    kNoSubformats,   16, 4, 1, traits::is_bc|traits::is_unorm|traits::is_srgb|traits::has_alpha },
  { "bc4_typeless",               oFCC('BC4?'), kBS_1_8,      kNoSubformats,    8, 1, 1, traits::is_bc|traits::is_typeless },
  { "bc4_unorm",                  oFCC('BC4u'), kBS_1_8,      kNoSubformats,    8, 1, 1, traits::is_bc|traits::is_unorm },
  { "bc4_snorm",                  oFCC('BC4s'), kBS_1_8,      kNoSubformats,    8, 1, 1, traits::is_bc },
  { "bc5_typeless",               oFCC('BC5?'), kBS_2_8,      kNoSubformats,   16, 2, 1, traits::is_bc },
  { "bc5_unorm",                  oFCC('BC5u'), kBS_2_8,      kNoSubformats,   16, 2, 1, traits::is_bc|traits::is_unorm },
  { "bc5_snorm",                  oFCC('BC5s'), kBS_2_8,      kNoSubformats,   16, 2, 1, traits::is_bc },
  { "b5g6r5_unorm",               kFCC_Unknown, kBS_565,      kNoSubformats,    2, 3, 1, traits::is_unorm },
  { "b5g5r5a1_unorm",             kFCC_Unknown, {5,6,5,1},    kNoSubformats,    2, 4, 1, traits::is_unorm|traits::has_alpha },
  { "b8g8r8a8_unorm",             oFCC('c4u '), kBS_4_8,      kNoSubformats,    4, 4, 1, traits::is_unorm|traits::has_alpha },
  { "b8g8r8x8_unorm",             oFCC('c4u '), kBS_4_8,      kNoSubformats,    4, 4, 1, traits::is_unorm },
  { "r10g10b10_xr_bias_a2_unorm", kFCC_Unknown, kBS_DEC3N,    kNoSubformats,    4, 4, 1, traits::is_unorm },
  { "b8g8r8a8_typeless",          kFCC_Unknown, kBS_4_8,      kNoSubformats,    4, 4, 1, traits::has_alpha|traits::is_typeless },
  { "b8g8r8a8_unorm_srgb",        kFCC_Unknown, kBS_4_8,      kNoSubformats,    4, 4, 1, traits::is_unorm|traits::is_srgb },
  { "b8g8r8x8_typeless",          kFCC_Unknown, kBS_4_8,      kNoSubformats,    4, 4, 1, traits::is_typeless },
  { "b8g8r8x8_unorm_srgb",        kFCC_Unknown, kBS_4_8,      kNoSubformats,    4, 4, 1, traits::is_unorm|traits::is_srgb },
  { "bc6h_typeless",              oFCC('BC6?'), kUnknownBits, kNoSubformats,   16, 3, 1, traits::is_bc|traits::is_typeless },
  { "bc6h_uf16",                  oFCC('BC6u'), kUnknownBits, kNoSubformats,   16, 3, 1, traits::is_bc },
  { "bc6h_sf16",                  oFCC('BC6s'), kUnknownBits, kNoSubformats,   16, 3, 1, traits::is_bc },
  { "bc7_typeless",               oFCC('BC7?'), kUnknownBits, kNoSubformats,   16, 4, 1, traits::is_bc|traits::has_alpha|traits::is_typeless },
  { "bc7_unorm",                  oFCC('BC7u'), kUnknownBits, kNoSubformats,   16, 4, 1, traits::is_bc|traits::is_unorm|traits::has_alpha },
  { "bc7_unorm_srgb",             oFCC('BC7s'), kUnknownBits, kNoSubformats,   16, 4, 1, traits::is_bc|traits::is_unorm|traits::is_srgb|traits::has_alpha },
  { "ayuv",                       oFCC('AYUV'), kBS_4_4,      kNoSubformats,   16, 4, 1, traits::is_unorm|traits::has_alpha|traits::is_yuv },
  { "y410",                       oFCC('Y410'), kBS_DEC3N,    {format::r10g10b10a2_unorm,format::unknown}, 4, 4, 1, traits::is_unorm|traits::has_alpha|traits::is_yuv },
  { "y416",                       oFCC('Y416'), kBS_4_16,     {format::b8g8r8a8_unorm,format::unknown}, 4, 4, 1, traits::is_unorm|traits::has_alpha|traits::is_yuv },
  { "nv12",                       oFCC('NV12'), kBS_3_8,      kSFD_R8_RG8,      1, 3, 2, traits::is_unorm|traits::is_yuv },
  { "p010",                       oFCC('P010'), {10,10,10,0}, kSFD_R16_RG16,    2, 3, 2, traits::is_unorm|traits::is_planar|traits::is_yuv },
  { "p016",                       oFCC('P016'), kBS_3_16,     kSFD_R16_RG16,    2, 3, 2, traits::is_unorm|traits::is_planar|traits::is_yuv },
  { "420_opaque",                 oFCC('420O'), kBS_3_8,      kSFD_R8_RG8,      1, 3, 2, traits::is_unorm|traits::is_planar|traits::is_yuv },
  { "yuy2",                       oFCC('YUY2'), kBS_4_8,      kSFD_R8_RG8,      4, 4, 1, traits::is_unorm|traits::has_alpha|traits::is_yuv },
  { "y210",                       oFCC('Y210'), kBS_4_16,     kNoSubformats,    2, 3, 1, traits::is_unorm|traits::is_yuv },
  { "y216",                       oFCC('Y216'), kBS_4_16,     kNoSubformats,    2, 3, 1, traits::is_unorm|traits::is_yuv },
  { "nv11",                       kFCC_Unknown, kBS_3_8,      kSFD_R8_RG8,      1, 3, 2, traits::is_unorm|traits::is_planar|traits::is_yuv },
  { "ia44",                       oFCC('IA44'), {4,0,0,4},    kNoSubformats,    1, 2, 1, traits::has_alpha|traits::paletted }, // index-alpha
  { "ai44",                       oFCC('AI44'), {4,0,0,4},    kNoSubformats,    1, 2, 1, traits::has_alpha|traits::paletted }, // alpha-index
  { "p8",                         oFCC('P8  '), kUnknownBits, kNoSubformats,    1, 1, 1, traits::paletted },
  { "a8p8",                       oFCC('A8P8'), kUnknownBits, kNoSubformats,    2, 2, 1, traits::has_alpha|traits::paletted },
  { "b4g4r4a4_unorm",             oFCC('n4u '), kBS_4_4,      kNoSubformats,    2, 4, 1, traits::is_unorm|traits::has_alpha },
  { "r8g8b8_unorm",               oFCC('c3u '), kBS_3_8,      kNoSubformats,    3, 3, 1, traits::is_unorm },
  { "r8g8b8_unorm_srgb",          kFCC_Unknown,	kBS_3_8,      kNoSubformats,    3, 3, 1, traits::is_unorm|traits::is_srgb },
	{ "r8g8b8x8_unorm",             kFCC_Unknown,	kBS_4_8,      kNoSubformats,		4, 4, 1, traits::is_unorm },
  { "r8g8b8x8_unorm_srgb",        kFCC_Unknown,	kBS_4_8,      kNoSubformats,    4, 4, 1, traits::is_unorm|traits::is_srgb },
	{ "b8g8r8_unorm",               kFCC_Unknown,	kBS_3_8,      kNoSubformats,    3, 3, 1, traits::is_unorm },
  { "b8g8r8_unorm_srgb",          kFCC_Unknown,	kBS_3_8,      kNoSubformats,    3, 3, 1, traits::is_unorm|traits::is_srgb },
  { "a8b8g8r8_unorm",             oFCC('abgr'), kBS_4_8,      kNoSubformats,    4, 4, 1, traits::is_unorm|traits::has_alpha },
  { "a8b8g8r8_unorm_srgb",        kFCC_Unknown,	kBS_4_8,      kNoSubformats,    4, 4, 1, traits::is_unorm|traits::is_srgb|traits::has_alpha },
  { "x8b8g8r8_unorm",             oFCC('xbgr'), kBS_4_8,      kNoSubformats,    4, 4, 1, traits::is_unorm },
  { "x8b8g8r8_unorm_srgb",        kFCC_Unknown,	kBS_4_8,      kNoSubformats,    4, 4, 1, traits::is_unorm|traits::is_srgb },
  { "y8_u8_v8_unorm",             oFCC('yuv8'), kBS_3_8,      kSFD_R8_3,        1, 3, 3, traits::is_unorm|traits::is_planar|traits::is_yuv|traits::subsurface1_bias1|traits::subsurface2_bias1 },
  { "y8_a8_u8_v8_unorm",          oFCC('auv8'), kBS_3_8,      kSFD_R8_4,        1, 3, 4, traits::is_unorm|traits::has_alpha|traits::is_planar|traits::is_yuv|traits::subsurface2_bias1|traits::subsurface3_bias1 },
  { "ybc4_ubc4_vbc4_unorm",       oFCC('yuvb'), kBS_4_8,      kSFD_BC4_3,       8, 3, 3, traits::is_bc|traits::is_unorm|traits::is_planar|traits::is_yuv|traits::subsurface1_bias1|traits::subsurface2_bias1 },
  { "ybc4_abc4_ubc4_vbc4_unorm",  oFCC('auvb'), kBS_4_8,      kSFD_BC4_4,       8, 3, 4, traits::is_bc|traits::is_unorm|traits::has_alpha|traits::is_planar|traits::is_yuv|traits::subsurface2_bias1|traits::subsurface3_bias1 },
  { "y8_u8v8_unorm",              oFCC('yv8u'), kBS_3_8,      kSFD_R8_RG8,      1, 3, 2, traits::is_unorm|traits::is_planar|traits::is_yuv|traits::subsurface1_bias1 },
  { "y8a8_u8v8_unorm",            oFCC('av8u'), kBS_4_8,      kSFD_RG8_RG8,     2, 4, 2, traits::is_unorm|traits::has_alpha|traits::is_planar|traits::is_yuv|traits::subsurface1_bias1 },
  { "ybc4_uvbc5_unorm",           oFCC('yvbu'), kBS_3_8,      kSFD_BC4_BC5,     8, 3, 2, traits::is_bc|traits::is_unorm|traits::is_planar|traits::is_yuv|traits::subsurface1_bias1 },
  { "yabc5_uvbc5_unorm",          oFCC('avbu'), kBS_4_8,      kSFD_BC5_BC5,    16, 4, 2, traits::is_bc|traits::is_unorm|traits::has_alpha|traits::is_planar|traits::is_yuv|traits::subsurface1_bias1 },
};
match_array_e(sFormatInfo, format);

static const format_info& finf(const format& f)
{
	return sFormatInfo[(int) (f < format::count ? f : format::unknown)];
}

format as_srgb(const format& f)
{
	switch (f)
	{
		case format::r8g8b8a8_typeless:
		case format::r8g8b8a8_unorm:
		case format::r8g8b8a8_unorm_srgb:
		case format::r8g8b8a8_uint:
		case format::r8g8b8a8_snorm:
		case format::r8g8b8a8_sint: return format::r8g8b8a8_unorm_srgb;
		case format::bc1_typeless:
		case format::bc1_unorm:
		case format::bc1_unorm_srgb: return format::bc1_unorm_srgb;
		case format::bc2_typeless:
		case format::bc2_unorm:
		case format::bc2_unorm_srgb: return format::bc2_unorm_srgb;
		case format::bc3_typeless:
		case format::bc3_unorm:
		case format::bc3_unorm_srgb: return format::bc3_unorm_srgb;
		case format::b8g8r8a8_typeless:
		case format::b8g8r8a8_unorm_srgb: return format::b8g8r8a8_unorm_srgb;
		case format::b8g8r8x8_typeless:
		case format::b8g8r8x8_unorm_srgb: return format::b8g8r8x8_unorm_srgb;
		case format::bc7_typeless:
		case format::bc7_unorm:
		case format::bc7_unorm_srgb: return format::bc7_unorm_srgb;
		case format::r8g8b8_unorm:
		case format::r8g8b8_unorm_srgb: return format::r8g8b8_unorm_srgb;
		case format::r8g8b8x8_unorm:
		case format::r8g8b8x8_unorm_srgb: return format::r8g8b8x8_unorm_srgb;
		case format::b8g8r8_unorm:
		case format::b8g8r8_unorm_srgb: return format:: b8g8r8_unorm_srgb;
		case format::a8b8g8r8_unorm:
		case format::a8b8g8r8_unorm_srgb: return format::a8b8g8r8_unorm_srgb;
		case format::x8b8g8r8_unorm:
		case format::x8b8g8r8_unorm_srgb: return format::x8b8g8r8_unorm_srgb;
		default: break;
	}
	return format::unknown;
}

format as_depth(const format& f)
{
	switch (f)
	{
		case format::d32_float_s8x24_uint:
		case format::r32_float_x8x24_typeless:
		case format::r32g8x24_typeless:
		case format::x32_typeless_g8x24_uint: return format::d32_float_s8x24_uint;
		case format::r32_typeless:
		case format::d32_float:
		case format::r32_float: return format::d32_float;
		case format::r24g8_typeless:
		case format::d24_unorm_s8_uint:
		case format::r24_unorm_x8_typeless:
		case format::x24_typeless_g8_uint: return format::d24_unorm_s8_uint;
		case format::r16_typeless:
		case format::r16_float:
		case format::d16_unorm:
		case format::r16_unorm:
		case format::r16_uint:
		case format::r16_snorm:
		case format::r16_sint: return format::d16_unorm;
		default: break;
	}
	return format::unknown;
}

format as_typeless(const format& f)
{
	switch (f)
	{
		case format::r32g32b32a32_typeless:
		case format::r32g32b32a32_float:
		case format::r32g32b32a32_uint:
		case format::r32g32b32a32_sint: return format::r32g32b32a32_typeless;
		case format::r32g32b32_typeless:
		case format::r32g32b32_float:
		case format::r32g32b32_uint:
		case format::r32g32b32_sint: return format::r32g32b32_typeless;
		case format::r16g16b16a16_typeless:
		case format::r16g16b16a16_float:
		case format::r16g16b16a16_unorm:
		case format::r16g16b16a16_uint:
		case format::r16g16b16a16_snorm:
		case format::r16g16b16a16_sint: return format::r16g16b16a16_typeless;
		case format::r32g32_typeless:
		case format::r32g32_float:
		case format::r32g32_uint:
		case format::r32g32_sint: return format::r32g32_typeless;
		case format::r32g8x24_typeless: 
		case format::d32_float_s8x24_uint: return format::r32g8x24_typeless;
		case format::r32_float_x8x24_typeless:
		case format::x32_typeless_g8x24_uint: return format::r32_float_x8x24_typeless;
		case format::r10g10b10a2_typeless:
		case format::r10g10b10a2_unorm:
		case format::r10g10b10a2_uint: return format::r10g10b10a2_typeless;
		case format::r8g8b8a8_typeless:
		case format::r8g8b8a8_unorm:
		case format::r8g8b8a8_unorm_srgb:
		case format::r8g8b8a8_uint:
		case format::r8g8b8a8_snorm:
		case format::r8g8b8a8_sint: return format::r8g8b8a8_typeless;
		case format::r16g16_typeless:
		case format::r16g16_float:
		case format::r16g16_unorm:
		case format::r16g16_uint:
		case format::r16g16_snorm:
		case format::r16g16_sint: return format::r16g16_typeless;
		case format::r32_typeless:
		case format::d32_float:
		case format::r32_float:
		case format::r32_uint:
		case format::r32_sint: return format::r32_typeless;
		case format::r24g8_typeless:
		case format::d24_unorm_s8_uint:
		case format::r24_unorm_x8_typeless:
		case format::x24_typeless_g8_uint: return format::r24g8_typeless;
		case format::r8g8_typeless:
		case format::r8g8_unorm:
		case format::r8g8_uint:
		case format::r8g8_snorm:
		case format::r8g8_sint: return format::r8g8_typeless;
		case format::r16_typeless:
		case format::r16_float:
		case format::d16_unorm:
		case format::r16_unorm:
		case format::r16_uint:
		case format::r16_snorm:
		case format::r16_sint: return format::r16_typeless;
		case format::r8_typeless:
		case format::r8_unorm:
		case format::r8_uint:
		case format::r8_snorm:
		case format::r8_sint: return format::r8_unorm;
		case format::bc1_typeless:
		case format::bc1_unorm:
		case format::bc1_unorm_srgb: return format::bc1_typeless;
		case format::bc2_typeless:
		case format::bc2_unorm:
		case format::bc2_unorm_srgb: return format::bc2_typeless;
		case format::bc3_typeless:
		case format::bc3_unorm:
		case format::bc3_unorm_srgb: return format::bc3_typeless;
		case format::bc4_typeless:
		case format::bc4_unorm:
		case format::bc4_snorm: return format::bc4_typeless;
		case format::bc5_typeless:
		case format::bc5_unorm:
		case format::bc5_snorm: return format::bc5_typeless;
		case format::b8g8r8a8_typeless:
		case format::b8g8r8a8_unorm_srgb: return format::b8g8r8a8_typeless;
		case format::b8g8r8x8_typeless:
		case format::b8g8r8x8_unorm_srgb: return format::b8g8r8x8_typeless;
		case format::bc6h_typeless:
		case format::bc6h_uf16:
		case format::bc6h_sf16: return format::bc6h_typeless;
		case format::bc7_typeless:
		case format::bc7_unorm:
		case format::bc7_unorm_srgb: return format::bc7_typeless;
		default: break;
	}
	return format::unknown;
}

format as_unorm(const format& f)
{
	switch (f)
	{
		case format::r16g16b16a16_typeless:
		case format::r16g16b16a16_float:
		case format::r16g16b16a16_unorm:
		case format::r16g16b16a16_uint:
		case format::r16g16b16a16_snorm:
		case format::r16g16b16a16_sint: return format::r16g16b16a16_unorm;
		case format::r10g10b10a2_typeless:
		case format::r10g10b10a2_unorm:
		case format::r10g10b10a2_uint: return format::r10g10b10a2_unorm;
		case format::r8g8b8a8_typeless:
		case format::r8g8b8a8_unorm:
		case format::r8g8b8a8_unorm_srgb:
		case format::r8g8b8a8_uint:
		case format::r8g8b8a8_snorm:
		case format::r8g8b8a8_sint: return format::r8g8b8a8_unorm;
		case format::r16g16_typeless:
		case format::r16g16_float:
		case format::r16g16_unorm:
		case format::r16g16_uint:
		case format::r16g16_snorm:
		case format::r16g16_sint: return format::r16g16_unorm;
		case format::r24g8_typeless:
		case format::d24_unorm_s8_uint:
		case format::r24_unorm_x8_typeless:
		case format::x24_typeless_g8_uint: return format::r24_unorm_x8_typeless;
		case format::r8g8_typeless:
		case format::r8g8_unorm:
		case format::r8g8_uint:
		case format::r8g8_snorm:
		case format::r8g8_sint: return format::r8g8_unorm;
		case format::r16_typeless:
		case format::r16_float:
		case format::d16_unorm:
		case format::r16_unorm:
		case format::r16_uint:
		case format::r16_snorm:
		case format::r16_sint: return format::r16_unorm;
		case format::r8_typeless:
		case format::r8_unorm:
		case format::r8_uint:
		case format::r8_snorm:
		case format::r8_sint: return format::r8_unorm;
		case format::bc1_typeless:
		case format::bc1_unorm:
		case format::bc1_unorm_srgb: return format::bc1_unorm;
		case format::bc2_typeless:
		case format::bc2_unorm:
		case format::bc2_unorm_srgb: return format::bc2_unorm;
		case format::bc3_typeless:
		case format::bc3_unorm:
		case format::bc3_unorm_srgb: return format::bc3_unorm;
		case format::bc4_typeless:
		case format::bc4_unorm:
		case format::bc4_snorm: return format::bc4_unorm;
		case format::bc5_typeless:
		case format::bc5_unorm:
		case format::bc5_snorm: return format::bc5_unorm;
		case format::b8g8r8a8_typeless:
		case format::b8g8r8a8_unorm:
		case format::b8g8r8a8_unorm_srgb: return format::b8g8r8a8_unorm;
		case format::b8g8r8x8_typeless:
		case format::b8g8r8x8_unorm_srgb: return format::b8g8r8x8_unorm;
		case format::bc7_typeless:
		case format::bc7_unorm:
		case format::bc7_unorm_srgb: return format::bc7_unorm;
		case format::b4g4r4a4_unorm: return format::b4g4r4a4_unorm;
		case format::r8g8b8_unorm:
		case format::r8g8b8_unorm_srgb: return format::r8g8b8_unorm;
		case format::r8g8b8x8_unorm:
		case format::r8g8b8x8_unorm_srgb: return format::r8g8b8x8_unorm;
		case format::b8g8r8_unorm:
		case format::b8g8r8_unorm_srgb: return format::b8g8r8_unorm;
		case format::a8b8g8r8_unorm:
		case format::a8b8g8r8_unorm_srgb: return format::a8b8g8r8_unorm;
		case format::x8b8g8r8_unorm:
		case format::x8b8g8r8_unorm_srgb: return format::x8b8g8r8_unorm;
		case format::y8_u8_v8_unorm:
		case format::y8_a8_u8_v8_unorm:
		case format::ybc4_ubc4_vbc4_unorm:
		case format::ybc4_abc4_ubc4_vbc4_unorm:
		case format::y8_u8v8_unorm:
		case format::y8a8_u8v8_unorm:
		case format::ybc4_uvbc5_unorm:
		case format::yabc5_uvbc5_unorm: return f;
		default: break;
	}
	return format::unknown;
}

format as_4chanx(const format& f)
{
	switch (f)
	{
		case format::r8g8b8_unorm:
		case format::r8g8b8a8_unorm:
		case format::r8g8b8x8_unorm: return format::r8g8b8x8_unorm;
		case format::r8g8b8_unorm_srgb:
		case format::r8g8b8a8_unorm_srgb:
		case format::r8g8b8x8_unorm_srgb: return format::r8g8b8x8_unorm_srgb;
		case format::b8g8r8_unorm:
		case format::b8g8r8a8_unorm:
		case format::b8g8r8x8_unorm: return format::b8g8r8x8_unorm;
		case format::b8g8r8a8_typeless:
		case format::b8g8r8x8_typeless: return format::b8g8r8x8_typeless;
		case format::b8g8r8_unorm_srgb:
		case format::b8g8r8a8_unorm_srgb:
		case format::b8g8r8x8_unorm_srgb: return format::b8g8r8x8_unorm_srgb;
		case format::a8b8g8r8_unorm:
		case format::x8b8g8r8_unorm: return format::x8b8g8r8_unorm;
		case format::a8b8g8r8_unorm_srgb:
		case format::x8b8g8r8_unorm_srgb: return format::x8b8g8r8_unorm_srgb;
		default: break;
	}
	return format::unknown;
}

format as_4chana(const format& f)
{
	switch (f)
	{
		case format::r8g8b8_unorm:
		case format::r8g8b8a8_unorm:
		case format::r8g8b8x8_unorm: return format::r8g8b8a8_unorm;
		case format::r8g8b8_unorm_srgb:
		case format::r8g8b8a8_unorm_srgb:
		case format::r8g8b8x8_unorm_srgb: return format::r8g8b8a8_unorm_srgb;
		case format::b8g8r8_unorm:
		case format::b8g8r8a8_unorm:
		case format::b8g8r8x8_unorm: return format::b8g8r8a8_unorm;
		case format::b8g8r8a8_typeless:
		case format::b8g8r8x8_typeless: return format::b8g8r8a8_typeless;
		case format::b8g8r8_unorm_srgb:
		case format::b8g8r8a8_unorm_srgb:
		case format::b8g8r8x8_unorm_srgb: return format::b8g8r8a8_unorm_srgb;
		case format::a8b8g8r8_unorm:
		case format::x8b8g8r8_unorm: return format::a8b8g8r8_unorm;
		case format::a8b8g8r8_unorm_srgb:
		case format::x8b8g8r8_unorm_srgb: return format::a8b8g8r8_unorm_srgb;
		default: break;
	}
	return format::unknown;
}

format as_3chan(const format& f)
{
	switch (f)
	{
		case format::r8g8b8_unorm:
		case format::r8g8b8a8_unorm:
		case format::r8g8b8x8_unorm: return format::r8g8b8_unorm;
		case format::r8g8b8_unorm_srgb:
		case format::r8g8b8a8_unorm_srgb:
		case format::r8g8b8x8_unorm_srgb: return format::r8g8b8_unorm_srgb;
		case format::a8b8g8r8_unorm:
		case format::x8b8g8r8_unorm:
		case format::b8g8r8_unorm:
		case format::b8g8r8a8_unorm:
		case format::b8g8r8x8_unorm: return format::b8g8r8_unorm;
		case format::b8g8r8_unorm_srgb:
		case format::b8g8r8a8_unorm_srgb:
		case format::b8g8r8x8_unorm_srgb:;
		case format::a8b8g8r8_unorm_srgb:
		case format::x8b8g8r8_unorm_srgb: return format::b8g8r8_unorm_srgb;
		default: break;
	}
	return format::unknown;
}

format as_texture(const format& f)
{
	switch (f)
	{
		case surface::format::b8g8r8_unorm_srgb:
		case surface::format::r8g8b8_unorm_srgb:
		case surface::format::r8g8b8x8_unorm_srgb:
		case surface::format::a8b8g8r8_unorm_srgb:
		case surface::format::x8b8g8r8_unorm_srgb: return surface::format::r8g8b8a8_unorm_srgb;

		case surface::format::b8g8r8_unorm:
		case surface::format::r8g8b8_unorm:
		case surface::format::r8g8b8x8_unorm:
		case surface::format::a8b8g8r8_unorm:
		case surface::format::x8b8g8r8_unorm: return surface::format::r8g8b8a8_unorm;

		default: break;
	}

	return f;
}

format as_nv12(const format& f)
{
	if (num_subformats(f) == 2) // already nv12
		return f;
	if (num_subformats(f) == 3) // no alpha
		return (is_block_compressed(f)) ? format::ybc4_uvbc5_unorm : format::y8_u8v8_unorm;
	if (is_block_compressed(f)) // alpha
		return format::yabc5_uvbc5_unorm;
	return format::y8a8_u8v8_unorm;
}

bit_size channel_bits(const format& f)
{
	return finf(f).bit_size;
}

static bool has_trait(const format& f, uint32_t traits)
{
	return !!(finf(f).traits & traits);
}

bool is_block_compressed(const format& f)
{
	return has_trait(f, traits::is_bc);
}

bool is_typeless(const format& f)
{
	return has_trait(f, traits::is_typeless);
}

bool is_depth(const format& f)
{
	return has_trait(f, traits::is_depth);
}

bool has_alpha(const format& f)
{
	return has_trait(f, traits::has_alpha);
}

bool is_unorm(const format& f)
{
	return has_trait(f, traits::is_unorm);
}

bool is_srgb(const format& f)
{
	return has_trait(f, traits::is_srgb);
}

bool is_planar(const format& f)
{
	return has_trait(f, traits::is_planar);
}

bool is_yuv(const format& f)
{
	return has_trait(f, traits::is_yuv);
}

uint32_t num_channels(const format& f)
{
	return finf(f).num_channels;
}

uint32_t num_subformats(const format& f)
{
	return finf(f).num_subformats;
}

uint32_t subsample_bias(const format& f, uint32_t subsurface)
{
	const auto& i = finf(f);
	if (subsurface == 1 && i.traits & traits::subsurface1_bias1) return 1;
	if (subsurface == 2 && i.traits & traits::subsurface2_bias1) return 1;
	if (subsurface == 3 && i.traits & traits::subsurface3_bias1) return 1;
	return 0;
}

uint32_t element_size(const format& f, uint32_t subsurface)
{
	return subsurface ? element_size(finf(f).subformats.format[subsurface]) : finf(f).element_size;
}

uint32_t bits(const format& f)
{
	if (f == format::r1_unorm) return 1;
	return 8 * element_size(f);
}

uint2 min_dimensions(const format& f)
{
	if (is_block_compressed(f)) return kMinMipBC;
	else if (is_yuv(f)) return kMinMipYUV;
	return kMinMip;
}

format subformat(const format& f, uint32_t subsurface)
{
	const auto& i = finf(f);
	if (!!(i.traits & traits::is_yuv))
	{
		if (i.num_subformats < subsurface)
			return format::unknown;
		return i.subformats.format[subsurface];
	}
	return f;
}

format from_fourcc(const fourcc_t& fcc)
{
	for (int i = 0; i < countof(surface::sFormatInfo); i++)
		if (fcc == sFormatInfo[i].fourcc)
			return format(i);
	return format::unknown;
}

fourcc_t to_fourcc(const format& f)
{
	return finf(f).fourcc;
}

	}

const char* as_string(const surface::format& f)
{
	return (f < surface::format::count) ? surface::sFormatInfo[int(f)].string : "unknown";
}

char* to_string(char* dst, size_t dst_size, const surface::format& f)
{
	return strlcpy(dst, as_string(f), dst_size) < dst_size ? dst : nullptr;
}

bool from_string(surface::format* out_format, const char* src)
{
	*out_format = surface::format::unknown;
	for (int i = 0; i < countof(surface::sFormatInfo); i++)
	{
		if (!_stricmp(src, surface::sFormatInfo[i].string))
		{
			*out_format = surface::format(i);
			return true;
		}
	}
	return false;
}

}
