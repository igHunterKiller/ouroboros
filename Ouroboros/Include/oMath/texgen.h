// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// texture coordinate generation

#ifndef oHLSL
#pragma once
#endif
#ifndef oMath_texgen_h
#define oMath_texgen_h

#include <oMath/hlsl.h>
#include <oMath/floats.h>
#ifndef oHLSL
	#include <oMath/hlsl_swizzles_on.h>
#endif

// returns texcoords for a position based on cylindrical projection where 
// the cylinder's bases are on the Z-axis at min_z and max_z.
inline float2 cylindrical_texgen(oIN(float3, position), float min_z, float max_z)
{
	return float2(1.0f - (atan2(position.x, position.y) + oPIf) / (2.0f * oPIf), (position.z - min_z) / (max_z - min_z));
}

// returns texcoords for a position based on spherical projection. (Z-Up)
inline float2 spherical_texgen(oIN(float3, position))
{
	return float2(1.0f - (atan2(position.x, position.y) + oPIf) / (2.0f * oPIf), (atan2(position.z, length(position.xy)) + oPIf) / (2.0f * oPIf));
}

// returns uv in xy and face index in z
inline float3 cubemap_texgen(oIN(float3, vec))
{
	// initialize facei to 0 if x, 2 if y, 4 if z is largest axis
	float3 abs_vec = abs(vec);
	uint facei = abs_vec.x > abs_vec.y ? 0 : 2;
	facei = abs_vec[facei] > abs_vec.z ? facei : 4;

	// rotate coords into uv, z is the divisor
	float2 uv = vec.xy;
	if (facei == 2) uv = vec.xz;
	else if (facei == 0) uv = vec.zy;
	
	// divide (project) largest magnitude onto others, then [-1,1] -> [0,1]
	uv = (uv / abs_vec[facei]) * 0.5f + 0.5f;

	// finish out facei as to whether the face is + or -
	return float3(uv.x, uv.y, float(facei + (vec[facei] < 0.0f ? 1 : 0)));
}

//X-Y-Z Planar
//Softimage wraps a texture on an object depending on how large the object is. To replicate the effect in a CAVE program:
//•Keep track of the largest and smallest X-Y-Z values when loading the model verticies.
//•Use (Vy - MinY)/(MaxY - MinY) to assign the U/V values.
//Alternatively, you can use the TexGen function. Place the planes in the smallest 
// coodinates and futz around with the normal length until you get the coverage 
// you want. For example, X-Z planar means you would need to assign the U values 
// to a plane with (A,B,C,D) values of the form (?,0,0,MinX) where ? is a 
// positive value. V would be of the form (0,0,?,MinZ).

#ifndef oHLSL
	#include <oMath/hlsl_swizzles_off.h>
#endif
#endif
