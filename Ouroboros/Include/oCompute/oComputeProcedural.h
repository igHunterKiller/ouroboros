// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.
// This code contains code that cross-compiles in C++ and HLSL. This contains
// some procedural textures mostly based on simplex noise.
#ifndef oHLSL
	#pragma once
#endif
#ifndef oComputeProcedural_h
#define oComputeProcedural_h

#include <oMath/hlsl.h>
#include <oCompute/oComputeSimplex.h>
#include <oCompute/oComputeUtil.h>
#include <oMath/hlsl_swizzles_on.h>

// Calculate a marble-like pattern (from NVIDIA, ATI, RenderMan and anyone else
// whose had a procedural marble texture). Use the returned value to interpolate
// between two colors. (Also use it to interpolate between two specular values!)
// Start tweaking from values: _Scale = 5, _Lacunarity = 2, _Gain = 0.5
inline float oCalcMarbleIntensity(oIN(float3, _Coord), float _Scale, float _Lacunarity, float _Gain)
{
	return 0.2f + _Scale * abs(oTurbulence(_Coord, 4, _Lacunarity, _Gain) - 0.5f);
}

// Calculate a granite-like pattern (from NVIDIA, ATI, RenderMan and anyone else
// whose had a procedural granite texture). Use the returned value to 
// interpolate between two colors. (Also use it to interpolate between two 
// specular values!) 
// Start tweaking from values: _Scale = 5, _Lacunarity = 2, _Gain = 0.5
inline float oCalcGraniteIntensity(oIN(float3, _Coord), float _Scale, float _Lacunarity, float _Gain)
{
	return abs(0.8f - oTurbulence(_Coord, 4, _Lacunarity, _Gain));
}

// This might've been described as a marble formula, but it appears with 
// oTurbulence to be more of a rust/stain pattern. Interpolate between diffuse
// and rust colors/textures. (Also use it to interpolate between two specular 
// values!)
// Start tweaking from values: _Amplification = 1, _Lacunarity = 2, _Gain = 0.5
inline float oCalcStainIntensity(oIN(float3, _Coord), float _Amplification, float _Lacunarity, float _Gain)
{
	// based on:
	// http://www.sci.utah.edu/~leenak/IndStudy_reportfall/MarbleCode.txt
	return saturate(cos(_Coord.z * 0.1f + _Amplification * oTurbulence(_Coord, 4, _Lacunarity, _Gain)));
}

// Generate a lerp value between two colors in a procedural wood pattern.
// (note it is recommended to have two specular colors to go with each diffuse
// color and lerp them by the intensity value
// Good start values: _RingScale = 16 _IrregularityScale = 0.2
inline float oCalcWoodIntensity(float3 _Coord, float _RingScale, float _IrregularityScale, float _Streakiness)
{
	// Based on NVIDIA Shader Library's wood shader

	float noisy0 = oTurbulence(_Coord, 4, 2.2f, 0.5f);
	float noisy1 = oTurbulence(_Coord + 1.0f, 4, 2.1f, 0.5f);
	float noisy2 = oTurbulence(_Coord + 2.0f, 4, 2.345f, 0.5f) * (1.0f-_Streakiness);

	float3 noiseval = float3(noisy0, noisy1, noisy2);
	float3 Pwood = _Coord + (_IrregularityScale * noiseval);
	float r = _RingScale * length(Pwood.xy);
	r = r + oTurbulence(float3(r,r,r)/32.0f, 4, 2.0f, 0.5f);
	r = frac(r);
	return smoothstep(0.0f, 0.8f, r) - smoothstep(0.83f, 1.0f, r);
}

// _____________________________________________________________________________
// Procedural textures not based on noise

// Returns a multiplier on where a uniform grid exists. The second parameter 
// should be: fwidth(_Coord * _GridRepeat). It has been separated from this
// function to separate raw math from data derived from interpolants.
// Good starting values for parameters are: _GridRepeat = 3, _Thickness = 1.2
inline float oCalcGridShadeIntensity2D(oIN(float2, _Coord), oIN(float2, _fwidthOfCoordTimesGridRepeat), float _GridRepeat, float _Thickness)
{
	// thickness-derived values that can be constants
	const float mi = max(0.0f, _Thickness - 1.0f);
	const float miInv = max(0.0f, 1.0f - _Thickness);
	const float ma = max(1.0f, _Thickness);

	const float2 f = abs(frac(_Coord * _GridRepeat) - 0.5f);
	const float2 df = _fwidthOfCoordTimesGridRepeat;
	const float2 g = clamp((f - df * mi) / (0.0001f + df * (ma - mi)), miInv, 1.0f);
	return 1.0f - (g.x * g.y);
}

// _fwidthOfCoordTimesGridRepeat should be fwidth(_Coord * _GridRepeat)
// Good starting values for parameters are: _GridRepeat = 3, _Thickness = 1.2
inline float oCalcGridShadeIntensity3D(oIN(float3, _Coord), oIN(float3, _fwidthOfCoordTimesGridRepeat), float _GridRepeat, float _Thickness)
{
	// thickness-derived values that can be constants
	const float mi = max(0.0f, _Thickness - 1.0f);
	const float miInv = max(0.0f, 1.0f - _Thickness);
	const float ma = max(1.0f, _Thickness);

	const float3 f  = abs(frac(_Coord * _GridRepeat) - 0.5f);
	const float3 df = _fwidthOfCoordTimesGridRepeat;
	const float3 g = clamp((f - df * mi) / (0.0001f + df * (ma - mi)), miInv, 1.0f);
	return g.x * g.y * g.z;
}

// Returns a multiplier on where a uniform grid exists. This tries to maintain
// a uniform density of grid sizes by calculating mip levels for a surface and
// using that to create another level of gridding (quad-tree style). A trilinear
// calculation is used to fade LODs of the grid in and out. Good starting values
// for parameters are: _GridRepeat = 0.2, _Intensity = 10, _Thickness = 0.05
// _MipSelection should be the result of oCalcMipSelection(_Texcoord).

// Based on article at:
// http://www.gamedev.net/topic/618788-efficient-drawing-of-2d-gridlines-with-shaders/
#define oCalcFadingGridShadeIntensityImpl(_Type) \
	const _Type TexcoordThisMip = _Texcoord * pow(2.0f, _MipSelection.x); \
	const _Type TexcoordNextMip = _Texcoord * pow(2.0f, _MipSelection.y); \
	const _Type HighlightThisMip = saturate(cos(TexcoordThisMip * _GridRepeat) - 1.0f + _Thickness) * _Intensity; \
	const _Type HighlightNextMip = saturate(cos(TexcoordNextMip * _GridRepeat) - 1.0f + _Thickness) * _Intensity; \
	return saturate(lerp(max(HighlightThisMip), max(HighlightNextMip), _MipSelection.z));

inline float oCalcFadingGridShadeIntensity2D(oIN(float2, _Texcoord), oIN(float3, _MipSelection), float _GridRepeat, float _Intensity, float _Thickness) { oCalcFadingGridShadeIntensityImpl(float2); }
inline float oCalcFadingGridShadeIntensity3D(oIN(float3, _Texcoord), oIN(float3, _MipSelection), float _GridRepeat, float _Intensity, float _Thickness) { oCalcFadingGridShadeIntensityImpl(float3); }

// Returns a multiplier between 0 and 1 in a checkerboard pattern. _fwidthCoord
// should be specified as the result of fwidth(_Coord). Good starting values: 
// _Scale = 0.5, _Balance = 0.5, _AAFilterWidth = 1
inline float oCalcCheckerIntensity1D(float _Coord, float _fwidthCoord, float _Scale, float _Balance, float _AAFilterWidth)
{
	// Based on NVIDIA's checker3d_math.fx shader
	// http://developer.download.nvidia.com/shaderlibrary/webpages/hlsl_shaders.html
	const float edge = _Scale * _Balance;
	const float op = _AAFilterWidth / _Scale;
	const float w = _fwidthCoord*op;
	const float x0 = _Coord / _Scale - (w/2.0f);
	const float x1 = x0 + w;
	const float nedge = edge/_Scale;
	const float i0 = (1.0f-nedge)*floor(x0) + max(0.0f, frac(x0)-nedge);
	const float i1 = (1.0f-nedge)*floor(x1) + max(0.0f, frac(x1)-nedge);
	return saturate((i1 - i0)/ (0.00001f + w));
}

// _fwidthCoord should be specified as the result of fwidth(_Coord).
inline float oCalcCheckerIntensity2D(oIN(float2, _Coord), oIN(float2, _fwidthCoord), float _Scale, float _Balance, float _AAFilterWidth)
{
	float edge = _Scale * _Balance;
	float op = _AAFilterWidth / _Scale;
	float2 check = float2(
		oCalcCheckerIntensity1D(_Coord.x, _fwidthCoord.x, _Scale, _Balance, _AAFilterWidth),
		oCalcCheckerIntensity1D(_Coord.y, _fwidthCoord.y, _Scale, _Balance, _AAFilterWidth));

	return abs(check.y - check.x);
}

// _fwidthCoord should be specified as the result of fwidth(_Coord).
inline float oCalcCheckerIntensity3D(oIN(float3, _Coord), oIN(float3, _fwidthCoord), float _Scale, float _Balance, float _AAFilterWidth)
{
	const float edge = _Scale * _Balance;
	const float op = _AAFilterWidth / _Scale;
	float3 check = float3(
		oCalcCheckerIntensity1D(_Coord.x, _fwidthCoord.x, _Scale, _Balance, _AAFilterWidth),
		oCalcCheckerIntensity1D(_Coord.y, _fwidthCoord.y, _Scale, _Balance, _AAFilterWidth),
		oCalcCheckerIntensity1D(_Coord.z, _fwidthCoord.z, _Scale, _Balance, _AAFilterWidth));

	return abs(check.z - abs(check.y - check.x));
}

#include <oMath/hlsl_swizzles_off.h>
#endif
