// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.
// This code contains code that cross-compiles in C++ and HLSL. This contains
// code relating to the Phong lighting model and OpenGL/shaders/HLSL utilities
// based on Phong.
#ifndef oHLSL
	#pragma once
#endif
#ifndef oComputePhong_h
#define oComputePhong_h

#include <oMath/hlsl.h>
#include <oCompute/oComputeUtil.h>
#include <oMath/hlsl_swizzles_on.h>
#ifdef oHLSL
	#define oRGBf float3
	#define oRGBAf float4
#endif

// _____________________________________________________________________________
// OpenGL lights math

// Classic OpenGL style attenuation
inline float oCalcAttenuation(float _ConstantFalloff, float _LinearFalloff, float _QuadraticFalloff, float _LightDistance)
{
	return saturate(1.0f / (_ConstantFalloff + _LinearFalloff*_LightDistance + _QuadraticFalloff*_LightDistance*_LightDistance));
}

// Input vectors are assumed to be normalized. Once NdotL is valid for lighting
// (i.e. this can be skipped if not) then this further culls against the cone
// of a spot light. The scalar returned should be multiplied against all 
// lighting values affected by the spotlight, (i.e. diffuse and specular, etc.)
inline float oCalcSpotlightAttenuation(oIN(float3, _SpotlightVector), oIN(float3, _LightVector), float _CosCutoffAngle, float _SpotExponent)
{
	float s = dot(_SpotlightVector, _LightVector);
	return (s > _CosCutoffAngle) ? pow(s, _SpotExponent) : 0.0f;
}

// Attenuates quadratically to a specific bounds of a light. This is useful for
// screen-space lighting where it is desirable to minimize the number of pixels
// touched, so the light's effect is 0 at _LightRadius.
inline float oCalcBoundQuadraticAttenuation(float _LightDistanceFromSurface, float _LightRadius, float _Cutoff)
{
	// Based on: http://imdoingitwrong.wordpress.com/2011/01/31/light-attenuation/
	float denom = (_LightDistanceFromSurface / _LightRadius) + 1.0f;
	float attenuation = 1.0f / (denom * denom); // simplified oCalcAttenuation
	return max((attenuation - _Cutoff) / (1.0f - _Cutoff), 0.0f);
}

// This is a companion function to oCalculateBoundQuadraticAttenuation and
// can be used to determine the radius of a screen-space circle inscribed in
// a quad that will optimally fit exactly the falloff of a point light.
inline float oCalcMaximumDistanceForPointLight(float _LightRadius, float _Cutoff)
{
	return _LightRadius * (sqrt(1.0f / _Cutoff) - 1.0f);
}

inline float oCalcLinearFogRatio(float _EyeDistance, float _FogStart, float _FogDistance)
{
	return saturate((_EyeDistance - _FogStart) / _FogDistance);
}

// _____________________________________________________________________________
// Permutations of lit to fix various common bugs and implement various trivial
// approximations of advanced effects (half-lambert, subsurface scatter)

// Returns NdotL in x and NdotH in y (not clamped in any way). All vectors are 
// assumed to be normalized.
inline float2 oLambert(oIN(float3, _SurfaceNormalVector), oIN(float3, _LightVector), oIN(float3, _EyeVector))
{
	return float2(dot(_SurfaceNormalVector, _LightVector), dot(_SurfaceNormalVector, normalize(_LightVector + _EyeVector)));
}

// This equation is from the NVIDIA shader library "lambskin". This is a super-
// trivial poor approximation for sub-surface scatter to achieve a skin-like 
// look. It's included here to be used as a placeholder during renderer bringup.
inline float oLambertSSS(float _NdotL, float _Rolloff)
{
	return max(0.0f, smoothstep(-_Rolloff, 1.0f, _NdotL) - smoothstep(0.0f, 1.0f, _NdotL));
}

// Toksvig specular scales brightness based on the mipmapping of normal maps. As 
// normal maps mip, they approach a planar mirror, so specular can get 
// disproportionately brighter at distances. The _UnnormalizedNdotNormalizedH is 
// a decoded normal, unnormalized. This means normalization of vertex normal/
// tangent/bitangent can be skipped and used unnormalized to sample a tangent-
// space normal map, which too can remain unnormalized. Use that dotted against 
// a normalized half vector (Hn = dot(normalize(ViewDir), normalize(LightDir))). 
// The second parameter is the length of the unnormalized sampled and 
// transformed normal. Read more:
// http://www.nvidia.com/object/mipmapping_normal_maps.html
// ftp://download.nvidia.com/developer/presentations/GDC_2004/RenderingTechniquesNVIDIA.pdf
// http://developer.download.nvidia.com/shaderlibrary/packages/Toksvig_NormalMaps.zip
inline float oCalcToksvigSpecular(float _UnnormalizedNdotNormalizedH, float _UnnormalizedNormalLength, float _SpecularExponent)
{
	float toksvig = _UnnormalizedNormalLength/(_UnnormalizedNormalLength+_SpecularExponent*(1-_UnnormalizedNormalLength));
	float AdjustedSpecularExponent = toksvig * _SpecularExponent;
	// In the NVIDIA sample, everything is written to a texture sampled in CLAMP 
	// mode, so here reinstall the [0,1] clamping with saturate.
	return (1.0f + AdjustedSpecularExponent)/(1.0f + _SpecularExponent) * pow(saturate(_UnnormalizedNdotNormalizedH/_UnnormalizedNormalLength), AdjustedSpecularExponent);
}

// Returns the results of HLSL's lit() with the specified parameters. All 
// vectors are assumed to be normalized.
inline float4 oLit(oIN(float3, _SurfaceNormalVector), oIN(float3, _LightVector), oIN(float3, _EyeVector), float _SpecularExponent)
{
	float2 L = oLambert(_SurfaceNormalVector, _LightVector, _EyeVector);
	return lit(L.x, L.y, _SpecularExponent);
}

// A variation on oLit that uses a softening of the NdotL ratio to give what 
// some might consider a more appealing falloff.
inline float4 oLitHalfLambert(oIN(float3, _SurfaceNormalVector), oIN(float3, _LightVector), oIN(float3, _EyeVector), float _SpecularExponent)
{
	const float kWrap = 0.5f;
	float2 L = oLambert(_SurfaceNormalVector, _LightVector, _EyeVector);
	float Kd = max(L.x*kWrap + (1.0f - kWrap), 0.0f);
	float Ks = pow(saturate(L.y), _SpecularExponent);
	return float4(1.0f, Kd, Ks, 1.0f);
}

// Calculate only the specular component using a Toksvig scaled specular value.
// The surface normal should be unnormalized, but light and eye vectors should
// be normalized. _NdotL should be the result of the normalized surface normal
// dotted against the light vector.
inline float oPhongSpecularToksvig(float _NdotL, oIN(float3, _UnnormalizedSurfaceNormalVector), float _UnnormalizedSurfaceNormalVectorLength, oIN(float3, _LightVector), oIN(float3, _EyeVector), float _SpecularExponent)
{
	float3 HalfVector = normalize(_LightVector + _EyeVector);
	float NdotH = dot(_UnnormalizedSurfaceNormalVector, HalfVector);
	return (_NdotL < 0.0f) || (NdotH < 0.0f) ? 0.0f : oCalcToksvigSpecular(NdotH, _UnnormalizedSurfaceNormalVectorLength, _SpecularExponent);
}

// Returns results similar to HLSL's lit(), but uses Toksvig's math to calculate
// a more appealing specular value based on mip'ing normal maps that approach
// perfect mirrors for higher mip levels. Use this instead of oLit when using
// normal maps with a mip chain.
inline float4 oLitToksvig(oIN(float3, _UnnormalizedSurfaceNormalVector), oIN(float3, _LightVector), oIN(float3, _EyeVector), float _SpecularExponent)
{
	float NLen = length(_UnnormalizedSurfaceNormalVector);
	float NDotL = dot(_UnnormalizedSurfaceNormalVector / NLen, _LightVector);
	float Kd = max(0.0f, NDotL);
	float Ks = oPhongSpecularToksvig(NDotL, _UnnormalizedSurfaceNormalVector, NLen, _LightVector, _EyeVector, _SpecularExponent);
	return float4(1.0f, Kd, Ks, 1.0f);
}

// A variation on oLit that uses a softening of the NdotL ratio to give what 
// some might consider a more appealing falloff.
inline float4 oLitHalfLambertToksvig(oIN(float3, _UnnormalizedSurfaceNormalVector), oIN(float3, _LightVector), oIN(float3, _EyeVector), float _SpecularExponent, oOUT(float3, _OutNormalizedSurfaceNormalVector))
{
	float NLen = length(_UnnormalizedSurfaceNormalVector);
	_OutNormalizedSurfaceNormalVector = _UnnormalizedSurfaceNormalVector / NLen;
	float NDotL = dot(_OutNormalizedSurfaceNormalVector, _LightVector);
	float Kd = max(0.0f, NDotL*0.5f+0.5f);
	float Ks = oPhongSpecularToksvig(NDotL, _UnnormalizedSurfaceNormalVector, NLen, _LightVector, _EyeVector, _SpecularExponent);
	return float4(1.0f, Kd, Ks, 1.0f);
}

// Replicated throughout permutations of Phong functions

#define oPHONG_BASE_PARAMETERS \
	float _Attenuation  \
	, oIN(oRGBf, _Ka) /* ambient color */ \
	, oIN(oRGBf, _Ke) /* emissive color */ \
	, oIN(oRGBf, _Kd) /* diffuse color */ \
	, oIN(oRGBf, _Ks) /* specular color */ \
	, oIN(oRGBf, _Kt) /* transmissive color */ \
	, oIN(oRGBf, _Kr) /* reflective color */ \
	, oIN(oRGBf, _Kl) /* light color */ \
	, float _Ksh /* shadow term (1 = unshadowed, 0 = shadowed) */

#define oPHONG_PARAMETERS oPHONG_BASE_PARAMETERS, float _Kh /* specular exponent */ \

#define oPHONG_BASE_VALUES _Attenuation, _Ka, _Ke, _Kd, _Ks, _Kt, _Kr, _Kl, _Ksh

// The main put-it-all-together Phong lighting model written in a way that keeps
// the math separate from parameter acquisition. _Lit is the result of lit() or
// an oLit() permutation.
inline oRGBf oPhongShadeBase(oIN(float4, _Lit), oPHONG_BASE_PARAMETERS)
{
	const float Cd = _Lit.y;
	const float Ca = _Lit.x * (1.0f - Cd); // so that in-shadow bump mapping isn't completely lost
	const float Cs = _Lit.z;
	const float Catt = _Ksh * _Attenuation;
	oRGBf rgb = Ca*_Ka + _Ke + Catt * _Kl * (Cd*_Kd + Cs*_Ks);
	// @tony: TODO: add transmissive and reflective
	return rgb;
}

// Mix in variations of lit into the main Phong entry API NOTE VECTOR 
// NORMALIZATION REQUIREMENTS
inline oRGBf oPhongShade(oIN(float3, _SurfaceNormalVector) // assumed to be normalized, pointing out from the surface
	, oIN(float3, _LightVector) // assumed to be normalized, pointing from surface to light
	, oIN(float3, _EyeVector) // assumed to be normalized, pointing from surface to eye
	, oPHONG_PARAMETERS)
{
	float4 Lit = oLit(_SurfaceNormalVector, _LightVector, _EyeVector, _Kh);
	return oPhongShadeBase(Lit, oPHONG_BASE_VALUES);
}

inline oRGBf oPhongShadeToksvig(oIN(float3, _UnnormalizedSurfaceNormalVector) // NOT NORMALIZED, pointing out from the surface
	, oIN(float3, _LightVector) // assumed to be normalized, pointing from surface to light
	, oIN(float3, _EyeVector) // assumed to be normalized, pointing from surface to eye
	, oPHONG_PARAMETERS)
{
	float4 Lit = oLitToksvig(_UnnormalizedSurfaceNormalVector, _LightVector, _EyeVector, _Kh);
	return oPhongShadeBase(Lit, oPHONG_BASE_VALUES);
}

inline oRGBf oPhongShadeHalfLambert(oIN(float3, _SurfaceNormalVector) // assumed to be normalized, pointing out from the surface
	, oIN(float3, _LightVector) // assumed to be normalized, pointing from surface to light
	, oIN(float3, _EyeVector) // assumed to be normalized, pointing from surface to eye
	, oPHONG_PARAMETERS)
{
	float4 Lit = oLitHalfLambert(_SurfaceNormalVector, _LightVector, _EyeVector, _Kh);
	return oPhongShadeBase(Lit, oPHONG_BASE_VALUES);
}

inline oRGBf oPhongShadeHalfLambertToksvig(oIN(float3, _UnnormalizedSurfaceNormalVector) // NOT NORMALIZED, pointing out from the surface
	, oIN(float3, _LightVector) // assumed to be normalized, pointing from surface to light
	, oIN(float3, _EyeVector) // assumed to be normalized, pointing from surface to eye
	, oPHONG_PARAMETERS)
{
	float3 NormalizedSurfaceNormal;
	float4 Lit = oLitHalfLambertToksvig(_UnnormalizedSurfaceNormalVector, _LightVector, _EyeVector, _Kh, NormalizedSurfaceNormal);
	return oPhongShadeBase(Lit, oPHONG_BASE_VALUES);
}

// A naive global illumination approximation
inline oRGBf oHemisphericShade(oIN(float3, _SurfaceNormalVector) // assumed to be normalized, pointing out from the surface
	, oIN(float3, _SkyVector) // assumed to be normalized, vector pointing at the sky color (and away from the ground color)
	, oIN(oRGBf, _HemisphericSkyColor) // color of the hemisphere in the direction pointed to by _SkyVector
	, oIN(oRGBf, _HemisphericGroundColor) // color of the hemisphere in the direction pointed away from by _SkyVector
	)
{
	return lerp(_HemisphericGroundColor, _HemisphericSkyColor, dot(_SurfaceNormalVector, _SkyVector) * 0.5f + 0.5f);
}

// http://vrayc4d.com/manuals/glossary/ior
static const float IoRVacuum = 1.0f;
static const float IoRAir = 1.0002926f; // at STP
static const float IoRIce = 1.31f;
static const float IoRSkin = 1.44f;
static const float IoRWater0 = 1.33346f;
static const float IoRWater100 = 1.31766f;
static const float IoRWater20 = 1.33283f;
static const float IoRWaterGas = 1.000261f;
static const float oFresnelDefaultPower = 5.0f;

// _EyeVector and _SurfaceNormal are expected to be normalized. _IoR1 and _IoR2
// are the indices of refraction. The ShaderX2 documented value for 
// _FalloffPower should be 5, but setting this to 1 will result in a "linear" 
// falloff. Setting _IoR1 == _IoR2 with a _FalloffPower == 1 exposes the 
// dot(_EyeVector, _SurfaceNormal) directly. This function is perfect for 
// prototyping, but if there's a ubiquitous shader (like an ocean shader) that
// has consistent values, it might be better to hard-code/pre-calculate large
// parts of this equation, or confirm the compiler is doing that if static const
// values are 
inline float oFresnel(oIN(float3, _EyeVector), oIN(float3, _SurfaceNormalVector), float _IoR1, float _IoR2, float _FalloffPower)
{
	// http://habibs.wordpress.com/alternative-solutions/ referring to ShaderX2.
	const float IoRDiff = _IoR1 - _IoR2;
	const float IoRSum = _IoR1 + _IoR2;
	const float R0 = (IoRDiff * IoRDiff) / (IoRSum * IoRSum);
	const float alpha = dot(_EyeVector, _SurfaceNormalVector);
	return R0 + (1.0f - R0) * pow(max(0.001f, 1.0f - alpha), _FalloffPower);
}

// Assume default falloff
inline float oFresnel(oIN(float3, _EyeVector), oIN(float3, _SurfaceNormalVector), float _IoR1, float _IoR2)
{
	return oFresnel(_EyeVector, _SurfaceNormalVector, _IoR1, _IoR2, oFresnelDefaultPower);
}

// Assume default falloff and an IoR1 of Air at STP
inline float oFresnel(oIN(float3, _EyeVector), oIN(float3, _SurfaceNormalVector), float _IoR)
{
	return oFresnel(_EyeVector, _SurfaceNormalVector, IoRAir, _IoR, oFresnelDefaultPower);
}

#include <oMath/hlsl_swizzles_off.h>
#endif
