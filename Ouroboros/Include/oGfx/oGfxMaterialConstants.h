// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.
// This header is compiled both by HLSL and C++. It contains all material 
// textures supplied by material definitions. These are re-purposed by various 
// BRDFs in the graphics layer. Use of oGPU_ does  not require that this be 
// used, but for the vast majority of use cases this provides a robust solution 
// and thus has been factored out as utility code to use, or be the basis of a 
// new, more fitted solution.
#ifndef oHLSL
	#pragma once
#endif
#ifndef oGfxMaterialConstants_h
#define oGfxMaterialConstants_h

#include <oGfx/oGfxHLSL.h>

#ifndef oGFX_MATERIAL_CONSTANTS_REGISTER
	#define oGFX_MATERIAL_CONSTANTS_REGISTER 4
#endif

// values from textures on [0,1] ([0,255]) will go from [1,oGFX_MAX_SPECULAR_EXPONENT]
#ifndef oGFX_MAX_SPECULAR_EXPONENT
	#define oGFX_MAX_SPECULAR_EXPONENT (2048.0f)
#endif

// Creates a material sampler that allows sampling from both standard textures
// and texturearrays.  This macro will create a function with two overloads:
// float4 _NameSample(float2 _Texcoord); // Standard texture
// float4 _NameSample(float2 _Texcoord, float _Slice); // Array texture
#define oDECLARE_MATERIAL_TEXTURE(_Name, _Register) \
	Texture2D oCONCAT(_Name, Texture) : register(oCONCAT(t, _Register)); \
	Texture2DArray oCONCAT(_Name, TextureArray) : register(oCONCAT(t, _Register)); \
	float4 oCONCAT(_Name, Sample) (float2 _Texcoord) { return oCONCAT(_Name, Texture).Sample(oGfxMaterialSampler, _Texcoord);} \
	float4 oCONCAT(_Name, Sample) (float2 _Texcoord, float _Slice) { return oCONCAT(_Name, TextureArray).Sample(oGfxMaterialSampler, float3(_Texcoord, _Slice));}

#ifdef oHLSL
SamplerState oGfxMaterialSampler : register(s0);

// oGfx implements the exact texture layout as documented in Valve's 
// "hero shader"
// http://media.steampowered.com/apps/dota2/workshop/Dota2ShaderMaskGuide.pdf
oDECLARE_MATERIAL_TEXTURE(oGfxColor, 0);
oDECLARE_MATERIAL_TEXTURE(oGfxNormal, 1);
oDECLARE_MATERIAL_TEXTURE(oGfxMask0, 2);
oDECLARE_MATERIAL_TEXTURE(oGfxMask1, 3);
oDECLARE_MATERIAL_TEXTURE(oGfxFalloff, 4);

#endif

struct oGfxMaterialConstants
{
	#ifndef oHLSL
		oGfxMaterialConstants()
			: Color(oWHITE3)
			, SampledTSNormal(0.5f, 0.5f, 1.0f)
			, Opacity(1.0f)
			, DetailMapIntensity(0.0f)
			, DiffuseFresnelIntensity(0.0f)
			, MetalnessIntensity(0.0f)
			, SelfIlluminationIntensity(0.0f)
			, SpecularIntensity(1.0f)
			, RimlightIntensity(0.0f)
			, SpecularTintIntensity(0.0f)
			, SpecularExponent(0.5f)
		{}
	#endif

	float3 Color;
	float3 SampledTSNormal; // directly sampled, not normalized
	float Opacity;
	float DetailMapIntensity;
	float DiffuseFresnelIntensity;
	float MetalnessIntensity;
	float SelfIlluminationIntensity;
	float SpecularIntensity;
	float RimlightIntensity;
	float SpecularTintIntensity;
	float SpecularExponent;
};

#ifdef oHLSL
cbuffer cbuffer_GfxMaterialConstants : register(oCONCAT(b, oGFX_MATERIAL_CONSTANTS_REGISTER)) { oGfxMaterialConstants GfxMaterialConstants; }

// The main path is where material parameters are sampled per-pixel, but for 
// legacy support and perhaps future support there is a cbuffer defined.
oGfxMaterialConstants oGfxGetMaterial(float2 _Texcoord)
{
	const float4 Color = oGfxColorSample(_Texcoord);
	const float4 Normal = oGfxNormalSample(_Texcoord);
	const float4 Mask0 = oGfxMask0Sample(_Texcoord);
	const float4 Mask1 = oGfxMask1Sample(_Texcoord);

	oGfxMaterialConstants Out;
	Out.Color = Color.rgb;
	Out.SampledTSNormal = Normal.xyz;
	Out.Opacity = Color.a;
	Out.DetailMapIntensity = Mask0.r;
	Out.DiffuseFresnelIntensity = Mask0.g;
	Out.MetalnessIntensity = Mask0.b;
	Out.SelfIlluminationIntensity = Mask0.a;
	Out.SpecularIntensity = Mask1.r;
	Out.RimlightIntensity = Mask1.g;
	Out.SpecularTintIntensity = Mask1.b;
	Out.SpecularExponent = pow(oGFX_MAX_SPECULAR_EXPONENT, Mask1.a);
	return Out;
}

#endif
#endif
