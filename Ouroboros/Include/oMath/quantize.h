// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Conversion of 32-bit floating point to/from smaller, less precise types.
// This compiles for both C++ and HLSL.

#ifndef oHLSL
#pragma once
#endif
#ifndef oMath_quantize_h
#define oMath_quantize_h

#ifndef oHLSL
#include <oMath/hlsl.h>
#endif

#define HALF_DIG         2               /* # of decimal digits of precision */
#define HALF_EPSILON     0.00097656F     /* smallest such that 1.0+FLT_EPSILON != 1.0 */
#define HALF_MANT_DIG    11              /* # of bits in mantissa */
#define HALF_MAX         65504.0f        /* max value */
#define HALF_MAX_10_EXP  4               /* max decimal exponent */
#define HALF_MAX_EXP     16              /* max binary exponent */
#define HALF_MIN         5.96046448e-08F /* min positive value */
#define HALF_MIN_10_EXP  (-4)            /* min decimal exponent */
#define HALF_MIN_EXP     (-13)           /* min binary exponent */
#define HALF_RADIX       2               /* exponent radix */
#define HALF_MINh        0x400           /* bit pattern for HALF_MIN as a half */
#define HALF_MAXh        0x7bff          /* bit pattern for HALF_MAX as a half */
#define HALF_INFh        0x7c00          /* bit pattern for +infinity as a half */
#define HALF_NINFh       0xfc00          /* bit pattern for -infinity as a half */

#define UF11_MINf 0.0f
#define UF11_MAXf 65024.0f
#define UF11_INF  0x0c40

#define UF10_MINf 0.0f
#define UF10_MAXf 64512.0f
#define UF10_INF  0x0620

// _____________________________________________________________________________
// convert a normalized float [0,1] to various lower bit representations
// i.e. n8tof32 s8tof32

#define oQUANTIZE_CONCAT2(a,b)      a##b
#define oQUANTIZE_CONCAT(a,b)       oQUANTIZE_CONCAT2(a,b)
#define oQUANTIZE_MINUS1(a)         ((1<<(a))-1)
#define oQUANTIZE_TO_NX(bits)       oQUANTIZE_CONCAT(f32ton,bits)
#define oQUANTIZE_TO_SX(bits)       oQUANTIZE_CONCAT(f32tos,bits)
#define oQUANTIZE_TO_F32N(bits)     oQUANTIZE_CONCAT(oQUANTIZE_CONCAT(n,bits),tof32)
#define oQUANTIZE_TO_F32S(bits)     oQUANTIZE_CONCAT(oQUANTIZE_CONCAT(s,bits),tof32)
#define oQUANTIZE_OVER_MINUS1(n,bits) ((n)/oQUANTIZE_MINUS1(bits))

#define oQUANTIZE_F32_TO_NX(bits) \
	inline uint   oQUANTIZE_TO_NX  (bits)    (float   a)  { return uint(a * float(oQUANTIZE_MINUS1(bits)) + 0.5f); } \
	inline uint2  oQUANTIZE_TO_NX  (bits)(oIN(float2, a)) { return uint2(oQUANTIZE_TO_NX(bits)(a.x), oQUANTIZE_TO_NX(bits)(a.y)); } \
	inline uint3  oQUANTIZE_TO_NX  (bits)(oIN(float3, a)) { return uint3(oQUANTIZE_TO_NX(bits)(a.x), oQUANTIZE_TO_NX(bits)(a.y), oQUANTIZE_TO_NX(bits)(a.z)); } \
	inline uint4  oQUANTIZE_TO_NX  (bits)(oIN(float4, a)) { return uint4(oQUANTIZE_TO_NX(bits)(a.x), oQUANTIZE_TO_NX(bits)(a.y), oQUANTIZE_TO_NX(bits)(a.z), oQUANTIZE_TO_NX(bits)(a.w)); } \
	inline float  oQUANTIZE_TO_F32N(bits)    (uint    a)  { return (float)((a & oQUANTIZE_MINUS1(bits)) / float(oQUANTIZE_MINUS1(bits))); } \
	inline float2 oQUANTIZE_TO_F32N(bits)(oIN(uint2,  a)) { return float2(oQUANTIZE_TO_F32N(bits)(a.x), oQUANTIZE_TO_F32N(bits)(a.y)); } \
	inline float3 oQUANTIZE_TO_F32N(bits)(oIN(uint3,  a)) { return float3(oQUANTIZE_TO_F32N(bits)(a.x), oQUANTIZE_TO_F32N(bits)(a.y), oQUANTIZE_TO_F32N(bits)(a.z)); } \
	inline float4 oQUANTIZE_TO_F32N(bits)(oIN(uint4,  a)) { return float4(oQUANTIZE_TO_F32N(bits)(a.x), oQUANTIZE_TO_F32N(bits)(a.y), oQUANTIZE_TO_F32N(bits)(a.z), oQUANTIZE_TO_F32N(bits)(a.w)); }

#define oQUANTIZE_F32_TO_SX(bits) \
	inline uint   oQUANTIZE_TO_SX  (bits)    (float   a)  { return uint((((a) * oQUANTIZE_OVER_MINUS1(0.5f,bits)) + oQUANTIZE_OVER_MINUS1(0.5f,bits)) + 0.5f); } \
	inline uint2  oQUANTIZE_TO_SX  (bits)(oIN(float2, a)) { return uint2(oQUANTIZE_TO_SX(bits)(a.x), oQUANTIZE_TO_SX(bits)(a.y)); } \
	inline uint3  oQUANTIZE_TO_SX  (bits)(oIN(float3, a)) { return uint3(oQUANTIZE_TO_SX(bits)(a.x), oQUANTIZE_TO_SX(bits)(a.y), oQUANTIZE_TO_SX(bits)(a.z)); } \
	inline uint4  oQUANTIZE_TO_SX  (bits)(oIN(float4, a)) { return uint4(oQUANTIZE_TO_SX(bits)(a.x), oQUANTIZE_TO_SX(bits)(a.y), oQUANTIZE_TO_SX(bits)(a.z), oQUANTIZE_TO_SX(bits)(a.w)); } \
	inline float  oQUANTIZE_TO_F32S(bits)    (uint    a)  { return (float)((a & oQUANTIZE_MINUS1(bits)) * oQUANTIZE_OVER_MINUS1(2.0f,bits)) - 1.0f; } \
	inline float2 oQUANTIZE_TO_F32S(bits)(oIN(uint2,  a)) { return float2(oQUANTIZE_TO_F32S(bits)(a.x), oQUANTIZE_TO_F32S(bits)(a.y)); } \
	inline float3 oQUANTIZE_TO_F32S(bits)(oIN(uint3,  a)) { return float3(oQUANTIZE_TO_F32S(bits)(a.x), oQUANTIZE_TO_F32S(bits)(a.y), oQUANTIZE_TO_F32S(bits)(a.z)); } \
	inline float4 oQUANTIZE_TO_F32S(bits)(oIN(uint4,  a)) { return float4(oQUANTIZE_TO_F32S(bits)(a.x), oQUANTIZE_TO_F32S(bits)(a.y), oQUANTIZE_TO_F32S(bits)(a.z), oQUANTIZE_TO_F32S(bits)(a.w)); }

oQUANTIZE_F32_TO_NX(1) oQUANTIZE_F32_TO_NX(2)  oQUANTIZE_F32_TO_NX(3)  oQUANTIZE_F32_TO_NX(4)  oQUANTIZE_F32_TO_NX(5)  oQUANTIZE_F32_TO_NX(6)  oQUANTIZE_F32_TO_NX(7)  oQUANTIZE_F32_TO_NX(8) 
oQUANTIZE_F32_TO_NX(9) oQUANTIZE_F32_TO_NX(10) oQUANTIZE_F32_TO_NX(11) oQUANTIZE_F32_TO_NX(12) oQUANTIZE_F32_TO_NX(13) oQUANTIZE_F32_TO_NX(14) oQUANTIZE_F32_TO_NX(15) oQUANTIZE_F32_TO_NX(16)

oQUANTIZE_F32_TO_SX(1) oQUANTIZE_F32_TO_SX(2)  oQUANTIZE_F32_TO_SX(3)  oQUANTIZE_F32_TO_SX(4)  oQUANTIZE_F32_TO_SX(5)  oQUANTIZE_F32_TO_SX(6)  oQUANTIZE_F32_TO_SX(7)  oQUANTIZE_F32_TO_SX(8) 
oQUANTIZE_F32_TO_SX(9) oQUANTIZE_F32_TO_SX(10) oQUANTIZE_F32_TO_SX(11) oQUANTIZE_F32_TO_SX(12) oQUANTIZE_F32_TO_SX(13) oQUANTIZE_F32_TO_SX(14) oQUANTIZE_F32_TO_SX(15) oQUANTIZE_F32_TO_SX(16)

// _____________________________________________________________________________
// convert a normalized float [0,1] to a signed normalized float [-1,1]

inline float  nf32tosf32    (float   a)  { return a * 2.0f - 1.0f; }
inline float2 nf32tosf32(oIN(float2, a)) { return a * 2.0f - 1.0f; }
inline float3 nf32tosf32(oIN(float3, a)) { return a * 2.0f - 1.0f; }
inline float4 nf32tosf32(oIN(float4, a)) { return a * 2.0f - 1.0f; }
inline float  sf32tonf32    (float   a)  { return a * 0.5f + 0.5f; }
inline float2 sf32tonf32(oIN(float2, a)) { return a * 0.5f + 0.5f; }
inline float3 sf32tonf32(oIN(float3, a)) { return a * 0.5f + 0.5f; }
inline float4 sf32tonf32(oIN(float4, a)) { return a * 0.5f + 0.5f; }

// _____________________________________________________________________________
// tuple formats: shared

// Name    Layout       Element
// high15  a1r5g6b5     unorm
// high16  r5g6b5			  unorm
// true    a8r8g8b8     unorm aka bgra, D3DCOLOR
// udec3   r10g10b10a2	unorm
// pk3     r11g11b10    ufloat

inline uint   float4tohigh15(oIN(float4, a))    { return uint16_t((f32ton1(a.w)<<15) | (f32ton5(a.x)<<10) | (f32ton5(a.y)<<5) | f32ton5(a.z)); }
inline float4 high15tofloat4(oIN(uint, high15)) { return float4(n5tof32((high15>>10) & 0x1f), n5tof32((high15>>5) & 0x1f), n5tof32(high15 & 0x1f), n1tof32(high15>>15)); }
inline uint   float3tohigh16(oIN(float3, a))    { return uint16_t((f32ton5(a.x)<<11) | (f32ton6(a.y)<<5) | f32ton5(a.z)); }
inline float3 high16tofloat3(oIN(uint, high16)) { return float3(n5tof32((high16>>11) & 0x1f), n6tof32((high16>>5) & 0x3f), n5tof32(high16 & 0x1f)); }
inline uint   float4totrue  (oIN(float4, a))    { return (f32ton8(a.w)<<24) | (f32ton8(a.x)<<16) | (f32ton8(a.y)<<8) | f32ton8(a.z); }
inline float4 truetofloat4  (oIN(uint, argb))   { return float4(n8tof32((argb>>16) & 0xff), n8tof32((argb>>8) & 0xff), n8tof32(argb & 0xff), n8tof32(argb>>24)); }
inline uint   float4toudec3 (oIN(float4, a))    { return uint(f32tos10(a.x)<<22) | (f32tos10(a.y)<<12) | (f32tos10(a.z)<<2) | f32tos2(a.w); }
inline float4 udec3tofloat4 (oIN(uint, udec3))  { return float4(s10tof32(udec3>>22), s10tof32((udec3>>12) & 0x3ff), s10tof32((udec3>>2) & 0x3ff), s10tof32(udec3 & 0x3)); }


// _____________________________________________________________________________
#ifndef oHLSL
#include <cstdint>

namespace ouro {

// _____________________________________________________________________________
// convert a signed normalized float [-1,1] to various lower bits representations

inline int16_t f32tos16(float x)   { return static_cast<int16_t>(x >= 0.0f ? (x * 32767.0f + 0.5f) : (x * 32768.0f - 0.5f)); }
inline float   s16tof32(int16_t x) { return x * (x >= 0 ? (1.0f/32767.0f) : (1.0f/32768.0f)); }

// _____________________________________________________________________________
// converts to/from a 16-bit float (half)

uint16_t f32tof16(float x);
float    f16tof32(uint16_t x);

// _____________________________________________________________________________
// converts to/from an 11/10 bit unsigned (non-negative) float

uint16_t        f32touf11(float x);
float           uf11tof32(uint16_t x);
inline uint16_t uf11exponent(uint16_t x) { return (x & 0x07c0) >> 6; }
inline uint16_t uf11mantissa(uint16_t x) { return x & 0x003f; }
inline uint16_t uf11(uint32_t exponent, uint32_t mantissa) { return static_cast<uint16_t>(exponent << 6 | mantissa); }

uint16_t        f32touf10(float x);
float           uf10tof32(uint16_t x);
inline uint16_t uf10exponent(uint16_t x) { return (x & 0x07c0) >> 5; }
inline uint16_t uf10mantissa(uint16_t x) { return x & 0x003f; }
inline uint16_t uf10(uint32_t exponent, uint32_t mantissa) { return static_cast<uint16_t>(exponent << 5 | mantissa); }

// _____________________________________________________________________________
// Tuple format: cpu-only

// Name    Layout       Element
// pk3     r11g11b10    ufloat

inline bool   pk3compatible (oIN(float3, a))  { return a.x >= 0.0f && a.x <= UF11_MAXf && a.y >= 0.0f && a.y <= UF11_MAXf && a.z >= 0.0f && a.z <= UF10_MAXf; }
inline uint   float3topk3   (oIN(float3, a))  { return uint((f32touf10(a.z)<<22) | (f32touf11(a.y)<<11) | f32touf11(a.x)); }
inline float3 pk3tofloat3   (uint pk3)        { return float3(uf11tof32(pk3 & 0x7ff), uf11tof32((pk3>>11) & 0x7ff), uf10tof32((pk3>>22) & 0x3ff)); }

}

#endif
#endif
