// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.
// NOTE: This header is compiled both by HLSL and C++, but is pretty much HLSL-
// only and in that, mostly only functions that invoke either a texture sampler
// or call upon interpolants APIs like ddx/ddy. There is not yet C++ API to 
// support such calls, so this code cannot be promoted to oCompute, though much
// of it should if there's ever a C++ texture sampler implemented well.
#ifndef oHLSL
#pragma once
#endif
#ifndef oGfxHLSL_h
#define oGfxHLSL_h

#include <oCompute/oComputeConstants.h>
#include <oCompute/oComputeFilter.h>
#include <oCompute/oComputeGBuffer.h>

#ifdef oHLSL

// _____________________________________________________________________________
// Utilities for working with textures

// http://http.developer.nvidia.com/GPUGems/gpugems_ch25.html
float oFilterwidth(float2 _Texcoord)
{
	float2 fw = max(abs(ddx(_Texcoord)), abs(ddy(_Texcoord)));
	return max(fw.x, fw.y);
}

// Returns 3 values; x contains the integer larger mip, y the next-smaller level 
// mip, and z contains the trilinear filter value
float3 oCalcMipSelection(float2 _Texcoord)
{
	const float2 TCddx = ddx(_Texcoord);
	const float2 TCddy = ddy(_Texcoord);
	const float DeltaMag2 = max(dot(TCddx, TCddx), dot(TCddy, TCddy));
	const float mip = 0.5f * log2(DeltaMag2);
	return float3(floor(mip), ceil(mip), frac(mip));
}

float2 oCalcTexelSize(Texture2D _Texture)
{
	uint Width; uint Height; uint NumberOfLevels;
	_Texture.GetDimensions(0, Width, Height, NumberOfLevels);
	return 1 / float2(Width, Height);
}

float2 oCalcTexelSize(Texture2D _Texture, float2 _Scale)
{
	uint Width; uint Height; uint NumberOfLevels;
	_Texture.GetDimensions(0, Width, Height, NumberOfLevels);
	return _Scale / float2(Width, Height);
}

// _____________________________________________________________________________
// Gradient Filtering (Cross, Sobel, Prewitt, etc.)

// Given a sample source's dimensions and the texcoord to sample, produce 
// several float4's each with a pair of neighbor texcoords to use in sampling
// elsewhere. This packs all nine texcoord values including the orignal one by
// going left-to-right, then top-to-bottom, so:
// 0xy 0zw 1xy
// 1zw 2xy 2zw (2xy is the original texcoord)
// 3xy 3zw 4xy (4zw not used)
// This assumes texcoord (0,0) is in the upper left and (1,1) is in the lower
// right. This function is intended to be called in a vertex shader that 
// prepares a fullscreen quad, so _Texcoord would be one of the corners and the 
// packed outputs would be vertex shader outputs.
void oSobelPackTexcoords(float2 _SampleWidthHeight, float2 _KernelScale, float2 _Texcoord, out float4 _PackedTexcoord0, out float4 _PackedTexcoord1, out float4 _PackedTexcoord2, out float4 _PackedTexcoord3, out float4 _PackedTexcoord4)
{
	const float2 t = _KernelScale / _SampleWidthHeight; // texel size

	// a 3x3 grid around the texcoord, specified from left to right, top to bottom
	// (where texcoord 0,0 is top-left and 1,1 is bottom-right)
	_PackedTexcoord0.xy = _Texcoord + -t;
	_PackedTexcoord0.zw = _Texcoord + float2(0, -t.y);
	_PackedTexcoord1.xy = _Texcoord + float2(t.x, -t.y);

	_PackedTexcoord1.zw = _Texcoord + float2(-t.x, 0);
	_PackedTexcoord2.xy = _Texcoord;
	_PackedTexcoord2.zw = _Texcoord + float2(t.x, 0);

	_PackedTexcoord3.xy = _Texcoord + float2(-t.x, t.y);
	_PackedTexcoord3.zw = _Texcoord + float2(0, t.y);
	_PackedTexcoord4.xy = _Texcoord + t;
	_PackedTexcoord4.zw = float2(0, 0);
}

// Same as the above, but overloaded to allow direct specification of a source
// texture.
void oSobelPackTexcoords(Texture2D _Source, float2 _KernelScale, float2 _Texcoord, out float4 _PackedTexcoord0, out float4 _PackedTexcoord1, out float4 _PackedTexcoord2, out float4 _PackedTexcoord3, out float4 _PackedTexcoord4)
{
	float2 Dim;
	_Source.GetDimensions(Dim.x, Dim.y);
	oSobelPackTexcoords(Dim, _KernelScale, _Texcoord, _PackedTexcoord0, _PackedTexcoord1, _PackedTexcoord2, _PackedTexcoord3, _PackedTexcoord4);
}

// Convert a 24-bit device depth buffer to a value [0,1] between the near and 
// far planes. NOTE: This is not 0 at the eye, this is 0 at the near plane so 
// this calculation is not fit for reconstructing world space. It is a better 
// fit for coloration/visualization of the depth buffer. _DeviceDepth is the
// value directly sampled/loaded from a D24_X8 or D24_S8 depth-stencil buffer.
float oDecodeDeviceDepth(float4 _DeviceDepth, float _Near, float _Far)
{
	float Depth = dot(_DeviceDepth.xyz, float3(255.0 / 256.0, 255.0 / 65536.0, 255.0 / 16777216.0));
	return (_Near * _Far) / (_Far - Depth * (_Far - _Near));
}

// This uses the values from oSobelPackTexcoords() to 8-neighbor Sobel filter
// an intensity texture (depth buffer) into the Sobel components.
float2 oSobelSampleIntensityPacked(Texture2D _Source, SamplerState _Sampler, float4 _PackedTexcoord0, float4 _PackedTexcoord1, float4 _PackedTexcoord2, float4 _PackedTexcoord3, float4 _PackedTexcoord4)
{
	float tl = _Source.Sample(_Sampler, _PackedTexcoord0.xy).x;
	float tc = _Source.Sample(_Sampler, _PackedTexcoord0.zw).x;
	float tr = _Source.Sample(_Sampler, _PackedTexcoord1.xy).x;
	float ml = _Source.Sample(_Sampler, _PackedTexcoord1.zw).x;
	// skip center texel
	float mr = _Source.Sample(_Sampler, _PackedTexcoord2.zw).x;
	float bl = _Source.Sample(_Sampler, _PackedTexcoord3.xy).x;
	float bc = _Source.Sample(_Sampler, _PackedTexcoord3.zw).x;
	float br = _Source.Sample(_Sampler, _PackedTexcoord4.xy).x;

	return oCalcSobelComponents(tl, tc, tr, ml, mr, bl, bc, br);
}

// This isn't quite a color, but since normals are often stored as RGB values
// in render targets, etc., place it here to draw contrast with the RGB version
// that uses a perceptual weighting. For normals, all axes are equal, so the 
// weighting is uniform.
inline float normal_to_luminance(oIN(float3, normal))
{
	return dot(normal, float3(0.33333f, 0.33333f, 0.33333f));
}

// This uses the values from oSobelPackTexcoords() to 8-neighbor Sobel filter
// a texture of encoded screen space normals.
float2 oSobelSampleHalfSphereIntensityPacked(Texture2D _Source, SamplerState _Sampler, float4 _PackedTexcoord0, float4 _PackedTexcoord1, float4 _PackedTexcoord2, float4 _PackedTexcoord3, float4 _PackedTexcoord4)
{
	float tl = normal_to_luminance(oHalfToFullSphere(_Source.Sample(_Sampler, _PackedTexcoord0.xy).xy));
	float tc = normal_to_luminance(oHalfToFullSphere(_Source.Sample(_Sampler, _PackedTexcoord0.zw).xy));
	float tr = normal_to_luminance(oHalfToFullSphere(_Source.Sample(_Sampler, _PackedTexcoord1.xy).xy));
	float ml = normal_to_luminance(oHalfToFullSphere(_Source.Sample(_Sampler, _PackedTexcoord1.zw).xy));
	// skip center texel
	float mr = normal_to_luminance(oHalfToFullSphere(_Source.Sample(_Sampler, _PackedTexcoord2.zw).xy));
	float bl = normal_to_luminance(oHalfToFullSphere(_Source.Sample(_Sampler, _PackedTexcoord3.xy).xy));
	float bc = normal_to_luminance(oHalfToFullSphere(_Source.Sample(_Sampler, _PackedTexcoord3.zw).xy));
	float br = normal_to_luminance(oHalfToFullSphere(_Source.Sample(_Sampler, _PackedTexcoord4.xy).xy));

	return oCalcSobelComponents(tl, tc, tr, ml, mr, bl, bc, br);
}

// This is expected to be called during a full-screen-quad pass.
float2 oSobelSampleIntensity(Texture2D _Source, SamplerState _Sampler, float2 _FSQTexcoord, float2 _SampleScale)
{
	const float2 t = oCalcTexelSize(_Source, _SampleScale);

#define S(_Out, _Offset) float _Out = _Source.Sample(_Sampler, _FSQTexcoord + _Offset).x;
	S(tl, -t)                S(tc, float2(0, -t.y)) S(tr, float2(t.x, -t.y))
		S(ml, float2(-t.x, 0)) /*S(mc, float2(0,0))*/   S(mr, float2(t.x, 0))
		S(bl, float2(-t.x, t.y)) S(bc, float2(0, t.y))  S(br, t)
#undef S

		return oCalcSobelComponents(tl, tc, tr, ml, mr, bl, bc, br);
}

// This is expected to be called during a full-screen-quad pass.
float2 oSobelSampleHalfSphereIntensity(Texture2D _Source, SamplerState _Sampler, float2 _FSQTexcoord, float2 _SampleScale)
{
	const float2 t = oCalcTexelSize(_Source, _SampleScale);

#define S(_Out, _Offset) float _Out = normal_to_luminance(oHalfToFullSphere(_Source.Sample(_Sampler, _FSQTexcoord + _Offset).xy)).x;
	S(tl, -t)                S(tc, float2(0, -t.y)) S(tr, float2(t.x, -t.y))
		S(ml, float2(-t.x, 0)) /*S(mc, float2(0,0))*/   S(mr, float2(t.x, 0))
		S(bl, float2(-t.x, t.y)) S(bc, float2(0, t.y))  S(br, t)
#undef S

		return oCalcSobelComponents(tl, tc, tr, ml, mr, bl, bc, br);
}

// Samples a displacement map's texels with 8-neighbor Sobel to generate a 
// normalized normal across the surface
float3 oSobelSampleNormal(Texture2D _Texture, SamplerState _Sampler, float2 _Texcoord, float _NormalScale)
{
	float w, h;
	_Texture.GetDimensions(w, h);
	float2 texelSize = 1.0 / float2(w, h);

	float tl = _Texture.Sample(_Sampler, _Texcoord + float2(-1, 1) * texelSize).x;
	float T = _Texture.Sample(_Sampler, _Texcoord + float2(0, 1) * texelSize).x;
	float tr = _Texture.Sample(_Sampler, _Texcoord + float2(1, 1) * texelSize).x;
	float L = _Texture.Sample(_Sampler, _Texcoord + float2(-1, 0) * texelSize).x;
	float R = _Texture.Sample(_Sampler, _Texcoord + float2(1, 0) * texelSize).x;
	float bl = _Texture.Sample(_Sampler, _Texcoord + float2(-1, -1) * texelSize).x;
	float B = _Texture.Sample(_Sampler, _Texcoord + float2(0, -1) * texelSize).x;
	float br = _Texture.Sample(_Sampler, _Texcoord + float2(1, -1) * texelSize).x;

	float2 SobelComponents = oCalcSobelComponents(tl, T, tr, L, R, bl, B, br);
	return normalize(float3(SobelComponents.x, 1 / _NormalScale, SobelComponents.y));
}

// Samples a displacement maps texel's 4 neighbors to generate a normalized 
// normal across the surface
float3 oCrossSampleNormal(Texture2D _Texture, SamplerState _Sampler, float2 _Texcoord, float _NormalScale)
{
	float w, h;
	_Texture.GetDimensions(w, h);
	float2 texelSize = 1.0 / float2(w, h);

	float T = _Texture.Sample(_Sampler, _Texcoord + float2(0, 1) * texelSize).x;
	float L = _Texture.Sample(_Sampler, _Texcoord + float2(-1, 0) * texelSize).x;
	float R = _Texture.Sample(_Sampler, _Texcoord + float2(1, 0) * texelSize).x;
	float B = _Texture.Sample(_Sampler, _Texcoord + float2(0, -1) * texelSize).x;
	return normalize(float3(R-L, 2 * _NormalScale, B-T));
}

// _____________________________________________________________________________
// Texture Sampling

float3 oSobelSampleNormalATI(Texture2D _Texture, SamplerState _Sampler, float2 _Texcoord, float4 _Lightness, float2 _TextureDimensions)
{
	// From RenderMonkey code, which is the GPU version of ATI's

	// The Sobel filter extracts the first order derivates of the image,
	// that is, the slope. The slope in X and Y direction allows us to
	// given a heightmap evaluate the normal for each pixel. This is
	// the same this as ATI's NormalMapGenerator application does,
	// except this is in hardware.
	//
	// These are the filter kernels:
	//
	//  SobelX       SobelY
	//  1  0 -1      1  2  1
	//  2  0 -2      0  0  0
	//  1  0 -1     -1 -2 -1

	float2 off = 1.0 / _TextureDimensions;

	// Take all neighbor samples
	float4 s00 = _Texture.Sample(_Sampler, _Texcoord + -off);
	float4 s01 = _Texture.Sample(_Sampler, _Texcoord + float2(0, -off.y));
	float4 s02 = _Texture.Sample(_Sampler, _Texcoord + float2(off.x, -off.y));

	float4 s10 = _Texture.Sample(_Sampler, _Texcoord + float2(-off.x, 0));
	float4 s12 = _Texture.Sample(_Sampler, _Texcoord + float2(off.x, 0));

	float4 s20 = _Texture.Sample(_Sampler, _Texcoord + float2(-off.x, off.y));
	float4 s21 = _Texture.Sample(_Sampler, _Texcoord + float2(0, off.y));
	float4 s22 = _Texture.Sample(_Sampler, _Texcoord + off);

	// Slope in X direction
	float4 sobelX = s00 + 2 * s10 + s20 - s02 - 2 * s12 - s22;
	// Slope in Y direction
	float4 sobelY = s00 + 2 * s01 + s02 - s20 - 2 * s21 - s22;

	// Weight the slope in all channels, we use grayscale as height
	float sx = dot(sobelX, _Lightness);
	float sy = dot(sobelY, _Lightness);

	// Compose the normal
	float3 normal = normalize(float3(sx, sy, 1));

	// Pack [-1, 1] into [0, 1]
	return normal * 0.5 + 0.5;
}

// Returns RGB as sampled from YUV sources in NV12 format (1 1-channel and 1 2-
// channel textures)
float3 oYUVSampleNV12(Texture2D _Y, Texture2D _UV, SamplerState _Sampler, float2 _Texcoord)
{
	float y = _Y.Sample(_Sampler, _Texcoord).x;
	float2 uv = _UV.Sample(_Sampler, _Texcoord).xy;
	return yuvtorgb(float3(y, uv.x, uv.y));
}

float4 oYUVSampleNV12A(Texture2D _YA, Texture2D _UV, SamplerState _Sampler, float2 _Texcoord)
{
	float2 ya = _YA.Sample(_Sampler, _Texcoord).xy;
	float2 uv = _UV.Sample(_Sampler, _Texcoord).xy;
	return float4(yuvtorgb(float3(ya.x, uv.x, uv.y)), ya.y);
}

// _____________________________________________________________________________
// Shading

// Implements the Hatch NPR lighting model. Basically this uses the Lambert 
// diffuse component (NdotL) to interpolate between several textures, here 
// stored as a Texture2DArray. http://gfx.cs.princeton.edu/proj/hatching/
float4 oHatchShade(Texture2DArray _HatchTextureArray, SamplerState _HatchSampler, float _Kd, float2 _Texcoord)
{
	uint Width; uint Height; uint Elements; uint NumberOfLevels;
	_HatchTextureArray.GetDimensions(0, Width, Height, Elements, NumberOfLevels);
	const float MaxElementIndex = Elements - 1;
	float ScaledToHatchCount = _Kd * MaxElementIndex;
	uint HatchLowIndex = floor(ScaledToHatchCount);
	uint HatchHighIndex = min(MaxElementIndex, HatchLowIndex + 1);
	float HatchLerp = frac(ScaledToHatchCount);
	float4 Low = _HatchTextureArray.Sample(_HatchSampler, float3(_Texcoord, HatchLowIndex));
	float4 High = _HatchTextureArray.Sample(_HatchSampler, float3(_Texcoord, HatchHighIndex));
	return lerp(Low, High, HatchLerp);
}

#endif
#endif