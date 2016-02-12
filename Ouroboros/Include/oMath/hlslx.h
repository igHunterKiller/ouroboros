// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// extended hlsl utilities

#ifndef oHLSL
#pragma once
#endif
#ifndef oMath_hlslx_h
#define oMath_hlslx_h

#include <oMath/hlsl.h>
#include <oMath/floats.h>

#ifndef oHLSL
#include <oMath/hlsl_swizzles_on.h>
#endif

// _____________________________________________________________________________
// Constants

static const float2   kUnit2        = float2(1.0f, 1.0f);
static const float2   kZero2        = float2(0.0f, 0.0f);
static const float3   kXAxis        = float3(1.0f, 0.0f, 0.0f);
static const float3   kYAxis        = float3(0.0f, 1.0f, 0.0f);
static const float3   kZAxis        = float3(0.0f, 0.0f, 1.0f);
static const float3   kUnit3        = float3(1.0f, 1.0f, 1.0f);
static const float3   kZero3        = float3(0.0f, 0.0f, 0.0f);
static const float4   kXAxis4       = float4(1.0f, 0.0f, 0.0f, 0.0f);
static const float4   kYAxis4       = float4(0.0f, 1.0f, 0.0f, 0.0f);
static const float4   kZAxis4       = float4(0.0f, 0.0f, 1.0f, 0.0f);
static const float4   kWAxis4       = float4(0.0f, 0.0f, 0.0f, 1.0f);
static const float4   kUnit4        = float4(1.0f, 1.0f, 1.0f, 1.0f);
static const float4   kZero4        = float4(0.0f, 0.0f, 0.0f, 0.0f);
static const float4   kIdentityQuat = float4(0.0f, 0.0f, 0.0f, 1.0f);
static const float4x4 kIdentity4x4  = float4x4(kXAxis4, kYAxis4, kZAxis4, kWAxis4);
static const float3x3 kIdentity3x3  = float3x3(kXAxis, kYAxis, kZAxis);
static const float3   kUpAxis       = kYAxis;

// _____________________________________________________________________________

typedef int oAxis;
#define oAXIS_X 0
#define oAXIS_Y 1
#define oAXIS_Z 2

// _____________________________________________________________________________
// Component ordering operators

// Returns the smallest value in the specified vector
inline float min(oIN(float2, a)) { return min(a.x, a.y); }
inline float min(oIN(float3, a)) { return min(min(a.x, a.y), a.z); }
inline float min(oIN(float4, a)) { return min(min(min(a.x, a.y), a.z), a.w); }
inline int   min(oIN(int2, a))   { return min(a.x, a.y); }
inline int   min(oIN(int3, a))   { return min(min(a.x, a.y), a.z); }
inline int   min(oIN(int4, a))   { return min(min(min(a.x, a.y), a.z), a.w); }
inline uint  min(oIN(uint2, a))  { return min(a.x, a.y); }
inline uint  min(oIN(uint3, a))  { return min(min(a.x, a.y), a.z); }
inline uint  min(oIN(uint4, a))  { return min(min(min(a.x, a.y), a.z), a.w); }

// Returns the largest value in the specified vector
inline float max(oIN(float2, a)) { return max(a.x, a.y); }
inline float max(oIN(float3, a)) { return max(max(a.x, a.y), a.z); }
inline float max(oIN(float4, a)) { return max(max(max(a.x, a.y), a.z), a.w); }
inline int   max(oIN(int2, a))   { return max(a.x, a.y); }
inline int   max(oIN(int3, a))   { return max(max(a.x, a.y), a.z); }
inline int   max(oIN(int4, a))   { return max(max(max(a.x, a.y), a.z), a.w); }
inline uint  max(oIN(uint2, a))  { return max(a.x, a.y); }
inline uint  max(oIN(uint3, a))  { return max(max(a.x, a.y), a.z); }
inline uint  max(oIN(uint4, a))  { return max(max(max(a.x, a.y), a.z), a.w); }

// _____________________________________________________________________________
// swap

#define oMATH_DEFINE_OSWAP(type) inline void swap(oINOUT(type, A), oINOUT(type, B)) { type C = A; A = B; B = C; }

oMATH_DEFINE_OSWAP(float)	 oMATH_DEFINE_OSWAP(bool)	 oMATH_DEFINE_OSWAP(int)	oMATH_DEFINE_OSWAP(uint)
oMATH_DEFINE_OSWAP(float2) oMATH_DEFINE_OSWAP(bool2) oMATH_DEFINE_OSWAP(int2)	oMATH_DEFINE_OSWAP(uint2)
oMATH_DEFINE_OSWAP(float3) oMATH_DEFINE_OSWAP(bool3) oMATH_DEFINE_OSWAP(int3)	oMATH_DEFINE_OSWAP(uint3)
oMATH_DEFINE_OSWAP(float4) oMATH_DEFINE_OSWAP(bool4) oMATH_DEFINE_OSWAP(int4)	oMATH_DEFINE_OSWAP(uint4)

// _____________________________________________________________________________
// Denormalized float detection

inline bool isdenorm(float x)
{
	int ix = asint(x);
	int mantissa = ix & 0x007fffff;
	int exponent = ix & 0x7f800000;
	return mantissa && !exponent;
}

inline float zerodenorm(float x)
{
	// http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.20.1348&rep=rep1&type=pdf
	const float anti_denorm = 1e-18f;
	float tmp = x + anti_denorm;
	tmp -= anti_denorm;
	return tmp;
}

// _____________________________________________________________________________
// Simple random number generators

// A simple LCG rand() function, unshifted/masked
// Note: this isn't worth it... use wang_hash instead
inline uint rand_unmasked(uint seed)
{
	return 1103515245 * seed + 12345;
}

// Another simple shader-ready RNG based on a 2D coord.
// Note: this isn't worth it... use wang_hash instead
inline float gelfond_rand(oIN(float2, coord))
{
	static const float gelfonds_constant = 23.1406926327792690f; // e ^ pi
	static const float gelfond_schneider_constant = 2.6651441426902251f; // 2 ^ sqrt(2)
	return frac(cos(fmod(123456789.0f, 1e-7f + 256.0f * dot(coord, float2(gelfonds_constant, gelfond_schneider_constant)))));
}

// _____________________________________________________________________________
// Integer pow

inline int  powi(int a, uint e)  { int v = 1; for (uint i = 0; i < e; i++) v *= a; return v; }
inline uint powi(uint a, uint e) { uint v = 1; oHLSL_UNROLL for (uint i = 0; i < e; i++) v *= a; return v; }
inline int  powi(int a, int e)   { return powi(a, (uint)e); }
inline uint powi(uint a, int e)  { return powi(a, (uint)e); }

// _____________________________________________________________________________
// Plane (Ax + By + Cz + Dw = 0 format (ABC = normalized normal, D = offset))

inline float4 normalize_plane(oIN(float4, plane)) { float len_inv = rsqrt(dot(plane.xyz, plane.xyz)); return plane * len_inv; }

// >0: same side as normal; <0: opposite side as normal; 0 means on plane
inline float sdistance(oIN(float4, plane), oIN(float3, p)) { return dot(plane.xyz, p) + plane.w; }
inline bool in_front_of(oIN(float4, plane), oIN(float3, p)) { return sdistance(plane, p) > 0.0f; }

// magnitude of distance of point from plane
inline float distance(oIN(float4, plane), oIN(float3, p)) { return abs(sdistance(plane, p));  }

// create a plane from a unit-length normal and a point
inline float4 plane(oIN(float3, normalized_normal), oIN(float3, p)) { return float4(normalized_normal, -dot(normalized_normal, p)); }

// returns the point on the plane that is closed to point p
inline float3 project(oIN(float4, plane), oIN(float3, p))
{
	float3 v = plane.xyz * plane.w;
	float d = dot(plane.xyz, p - v);
	return p - plane.xyz * d;
}

// _____________________________________________________________________________
// Color Conversion

inline float srgbtolin    (float x) { return (x <= 0.04045f) ? (x / 12.42f) : pow((x + 0.055f) / 1.055f, 2.4f); }
inline float lintosrgb    (float x) { return (x <= 0.0031308f) ? (x * 12.92f) : (1.055f * pow(x, 1.0f / 2.4f) - 0.055f); }
inline float lintosrgbfast(float x) { return x < 0.0031308f ? 12.92f * x : 1.13005f * sqrt(abs(x - 0.00228f)) - 0.13448f * x + 0.005719f; }

inline float3 srgbtolin(oIN(float3, srgb))
{
	float3 lin_rgb;
	lin_rgb.x = srgbtolin(srgb.x); lin_rgb.y = srgbtolin(srgb.y); lin_rgb.z = srgbtolin(srgb.z);
	return lin_rgb;
}

inline float3 lintosrgb(oIN(float3, lin_rgb))
{
	float3 srgb;
	srgb.x = lintosrgb(lin_rgb.x); srgb.y = lintosrgb(lin_rgb.y); srgb.z = lintosrgb(lin_rgb.z);
	return srgb;
}

inline float srgbtolum(oIN(float3, srgb))
{
	// from http://en.wikipedia.org/wiki/Luminance_(relative)
	// "For RGB color spaces that use the ITU-R BT.709 primaries 
	// (or sRGB, which defines the same primaries), relative 
	// luminance can be calculated from linear RGB components:"
	return dot(srgb, float3(0.2126f, 0.7152f, 0.0722f));
}

inline float3 hsvtorgb(oIN(float3, hsv))
{
	// http://chilliant.blogspot.com/2010/11/rgbhsv-in-hlsl.html
	float R = abs(hsv.x * 6.0f - 3.0f) - 1.0f;
	float G = 2.0f - abs(hsv.x * 6.0f - 2.0f);
	float B = 2.0f - abs(hsv.x * 6.0f - 4.0f);
	return ((saturate(float3(R,G,B)) - 1.0f) * hsv.y + 1.0f) * hsv.z;
}

inline float3 rgbtohsv(oIN(float3, rgb))
{
	// http://stackoverflow.com/questions/4728581/hsl-image-adjustements-on-gpu
	float H = 0.0f;
	float S = 0.0f;
	float V = max(rgb.x, max(rgb.y, rgb.z));
	float m = min(rgb.x, min(rgb.y, rgb.z));
	float chroma = V - m;
	S = chroma / V;
	float3 delta = (V - rgb) / chroma;
	delta -= delta.zxy;
	delta += float3(2.0f, 4.0f, 0.0f);
	float3 choose = float3(
		(rgb.y == V && rgb.z != V) ? 1.0f : 0.0f,
		(rgb.z == V && rgb.x != V) ? 1.0f : 0.0f,
		(rgb.x == V && rgb.y != V) ? 1.0f : 0.0f);
	H = dot(delta, choose);
	H = frac(H / 6.0f);
	return float3(chroma == 0.0f ? 0.0f : H, chroma == 0.0f ? 0.0f : S, V);
}

inline float3 yuvtorgb(oIN(float3, yuv))
{
	// Using the float version of ITU-R BT.601 that jpeg uses. This is similar to 
	// the integer version, except this uses the full 0 - 255 range.
	static const float3 oITU_R_BT_601_Offset  = float3(0.0f, -128.0f, -128.0f) / 255.0f;
	static const float3 oITU_R_BT_601_RFactor = float3(1.0f, 0.0f, 1.402f);
	static const float3 oITU_R_BT_601_GFactor = float3(1.0f, -0.34414f, -0.71414f);
	static const float3 oITU_R_BT_601_BFactor = float3(1.0f, 1.772f, 0.0f);
	float3 x = yuv + oITU_R_BT_601_Offset;
	return saturate(float3(dot(x, oITU_R_BT_601_RFactor), dot(x, oITU_R_BT_601_GFactor), dot(x, oITU_R_BT_601_BFactor)));
}

inline float3 rgbtoyuv(oIN(float3, rgb))
{
	// Using the float version of ITU-R BT.601 that jpeg uses. This is similar to 
	// the integer version, except this uses the full 0 - 255 range.
	static const float3 oITU_R_BT_601_OffsetYUV = float3(0.0f, 128.0f, 128.0f) / 255.0f;
	static const float3 oITU_R_BT_601_YFactor   = float3(0.299f, 0.587f, 0.114f);
	static const float3 oITU_R_BT_601_UFactor   = float3(-0.1687f, -0.3313f, 0.5f);
	static const float3 oITU_R_BT_601_VFactor   = float3(0.5f, -0.4187f, -0.0813f);
	return saturate(float3(dot(rgb, oITU_R_BT_601_YFactor), dot(rgb, oITU_R_BT_601_UFactor), dot(rgb, oITU_R_BT_601_VFactor)) + oITU_R_BT_601_OffsetYUV);
}

// _____________________________________________________________________________
// Misc

// Normalize, but returns the length as well
inline float2 normalize(oIN(float2, x), oOUT(float, out_length)) { out_length = length(x); return x / out_length; }
inline float3 normalize(oIN(float3, x), oOUT(float, out_length)) { out_length = length(x); return x / out_length; }
inline float4 normalize(oIN(float4, x), oOUT(float, out_length)) { out_length = length(x); return x / out_length; }

// returns the angle between the two specified vectors (vectors are not assumed
// to be normalized).
inline float angle(oIN(float3, a), oIN(float3, b)) { return acos(dot(a, b) / (length(a) * length(b))); }

// returns the index of the axis with the largest magnitude
inline oAxis dominant_axis(oIN(float3, x))
{
	float3 mag = abs(x);
	float max_ = max(mag);
	return (max_ <= mag.x) ? oAXIS_X : ((max_ <= mag.y) ? oAXIS_Y : oAXIS_Z);
}

// _____________________________________________________________________________

#ifndef oHLSL
#include <oMath/hlsl_swizzles_off.h>

typedef unsigned short ushort;
typedef oHLSL2<short> short2; typedef oHLSL2<ushort> ushort2;
typedef oHLSL4<short> short4; typedef oHLSL4<ushort> ushort4;

#endif

#endif

