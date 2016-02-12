// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oMath/quantize.h>

// adapted from http://ptgmedia.pearsoncmg.com/images/9780321552624/downloads/0321552628_AppJ.pdf

#define F32_INFINITY 0x7f800000

#define F16_EXPONENT_BITS 0x1F
#define F16_EXPONENT_SHIFT 10
#define F16_EXPONENT_BIAS 15
#define F16_MANTISSA_BITS 0x3ff
#define F16_MANTISSA_SHIFT (23 - F16_EXPONENT_SHIFT)
#define F16_MAX_EXPONENT (F16_EXPONENT_BITS << F16_EXPONENT_SHIFT)

#define UF11_EXPONENT_BIAS 15
#define UF11_EXPONENT_BITS 0x1F
#define UF11_EXPONENT_SHIFT 6
#define UF11_MANTISSA_BITS 0x3F
#define UF11_MANTISSA_SHIFT (23 - UF11_EXPONENT_SHIFT)
#define UF11_MAX_EXPONENT (UF11_EXPONENT_BITS << UF11_EXPONENT_SHIFT)

#define UF10_EXPONENT_BIAS 15
#define UF10_EXPONENT_BITS 0x1F
#define UF10_EXPONENT_SHIFT 5
#define UF10_MANTISSA_BITS 0x3F
#define UF10_MANTISSA_SHIFT (23 - UF10_EXPONENT_SHIFT)
#define UF10_MAX_EXPONENT (UF10_EXPONENT_BITS << UF10_EXPONENT_SHIFT)

namespace ouro {

static inline float common_decode(int sign, int exponent, int mantissa, float scale_denominator, float mantissa_denominator)
{
	float f32 = 0.0f;

	if (exponent == 0)
	{
		if (mantissa != 0)
		{
			const float scale = 1.0f / scale_denominator;
			f32 = scale * mantissa;
		}
	}

	else if (exponent == 31)
		*(int*)&f32 = sign | F32_INFINITY | mantissa;

	else
	{
		exponent -= 15;
		
		const float scale = exponent < 0 ? (1.0f / (1 << -exponent)) : float(1 << exponent);
		const float decimal = 1.0f + (float)mantissa / mantissa_denominator;
		f32 = scale * decimal;
	}

	if (sign)
		f32 = -f32;

	return f32;
}

uint16_t f32tof16(float x)
{
	uint32_t f32 = *(uint32_t*)&x;
	int f16 = 0;
	
	// Decode IEEE 754 little-endian 32-bit floating-point value
	int sign = (f32 >> 16) & 0x8000;
	int exponent = ((f32 >> 23) & 0xff) - 127; // Map exponent to the range [-127,128]
	int mantissa = f32 & 0x007fffff;
	
	if (exponent == 128) // NaN/inf
	{
		f16 = sign | F16_MAX_EXPONENT;
		if (mantissa)
			f16 |= (mantissa & F16_MANTISSA_BITS);
	}
	else if (exponent > 15) // overflow, mark as inf
		f16 = sign | F16_MAX_EXPONENT;
	
	else if (exponent > -15)
	{
		exponent += F16_EXPONENT_BIAS;
		mantissa >>= F16_MANTISSA_SHIFT;

		f16 = sign | exponent << F16_EXPONENT_SHIFT | mantissa;
	}

	else
		f16 = sign;
	
	return static_cast<uint16_t>(f16);
}

float f16tof32(uint16_t x)
{
	const int sign = (x & 0x8000) << 15;
	const int exponent = (x & 0x7c00) >> 10;
	const int mantissa = (x & 0x03ff);

	return common_decode(sign, exponent, mantissa, float(1<<24), 1024.0f);
}

uint16_t f32touf11(float x)
{
  // decode IEEE 32-bit float
  uint32_t f32 = (*(uint32_t*)&x);
  if ((f32 >> 16) & 0x8000) // only support unsigned values
    return 0;

  int exponent = ((f32 >> 23) & 0xff) - 127; // rerange exp to [-127,128]
  int mantissa = f32 & 0x007fffff;

	uint16_t uf11 = 0;

  if (exponent == 128) // NaN/inf
  {
    uf11 = UF11_MAX_EXPONENT;
    if (mantissa)
      uf11 |= mantissa & UF11_MANTISSA_BITS;
  }

  else if (exponent > 15) // overflow, mark as inf
    uf11 = UF11_MAX_EXPONENT;
  
  else if (exponent > -15)
  {
    exponent += UF11_EXPONENT_BIAS;
    mantissa >>= UF11_MANTISSA_SHIFT;
    
    uf11 = static_cast<uint16_t>(exponent << UF11_EXPONENT_SHIFT | mantissa);
  }

	return uf11;
}

float uf11tof32(uint16_t x)
{
	const uint32_t exponent = (x & 0x07c0) >> UF11_EXPONENT_SHIFT;
	const uint32_t mantissa = (x & 0x003f);
	
	return common_decode(0, exponent, mantissa, float(1<<20), 64.0f);
}

uint16_t f32touf10(float x)
{
  // decode IEEE 32-bit float
  uint32_t f32 = (*(uint32_t*)&x);
  if ((f32 >> 16) & 0x8000) // only support unsigned values
    return 0;

  int exponent = ((f32 >> 23) & 0xff) - 127; // rerange exp to [-127,128]
  int mantissa = f32 & 0x007fffff;

	uint16_t uf10 = 0;

  if (exponent == 128) // NaN/inf
  {
    uf10 = UF10_MAX_EXPONENT;
    if (mantissa)
      uf10 = mantissa & UF10_MANTISSA_BITS;
  }

  else if (exponent > 15) // overflow, mark as inf
    uf10 = UF10_MAX_EXPONENT;
  
  else if (exponent > -15)
  {
    exponent += UF10_EXPONENT_BIAS;
    mantissa >>= UF10_MANTISSA_SHIFT;
    
    uf10 = static_cast<uint16_t>(exponent << UF10_EXPONENT_SHIFT | mantissa);
  }

	return uf10;
}

float uf10tof32(uint16_t x)
{
	const int exponent = (x & 0x07c0) >> UF10_EXPONENT_SHIFT;
	const int mantissa = (x & 0x003f);

	return common_decode(0, exponent, mantissa, float(1<<20), 64.0f);
}

}
