// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.
// This code contains code that cross-compiles in C++ and HLSL. This contains
// more primitive math manipulation functions.
#ifndef oHLSL
	#pragma once
#endif
#ifndef oComputeUtil_h
#define oComputeUtil_h

#include <oMath/hlslx.h>
#include <oMath/quat.h>
#include <oMath/hlsl_swizzles_on.h>

#ifndef oCONCAT
	#define oCONCAT(x, y) x##y
#endif

// _____________________________________________________________________________
// Normalmap and tangent basis utilities

// Returns an unnormalized normal in world space. If the return value is to be
// passed onto a Toksvig-scaled lighting model, then the input vertex vectors do 
// not need to be normalized.
inline float3 oDecodeTSNormal(oIN(float3, _WSVertexTangentVector)
	, oIN(float3, _WSVertexBitangentVector)
	, oIN(float3, _WSVertexNormalVector)
	, oIN(float3, _TSNormalMapColor)
	, float _BumpScale)
{
	const float3 TSNormal = _TSNormalMapColor*2.0f - 1.0f;
	return TSNormal.x*_BumpScale*_WSVertexTangentVector + TSNormal.y*_BumpScale*_WSVertexBitangentVector + TSNormal.z*_WSVertexNormalVector;
}

// Handedness is stored in w component of _InTangent. This should be called out 
// of the vertex shader and the out values passed through to the pixel shader.
// For world-space, pass the world matrix. For view space, pass the WorldView
// matrix. Read more: http://www.terathon.com/code/tangent.html.
inline void oTransformTangentBasisVectors(oIN(float4x4, _Matrix)
	, oIN(float3, _InNormalVector)
	, oIN(float4, _InTangentVectorAndFacing)
	, oOUT(float3, _OutNormalVector)
	, oOUT(float3, _OutTangentVector)
	, oOUT(float3, _OutBitangentVector))
{
	float3x3 R = (float3x3)_Matrix;
	_OutNormalVector = mul(R, _InNormalVector);
	_OutTangentVector = mul(R, _InTangentVectorAndFacing.xyz);
	_OutBitangentVector = cross(_OutNormalVector, _OutTangentVector) * _InTangentVectorAndFacing.w;
}

inline void oQRotateTangentBasisVectors(oIN(float4, _Quaternion)
	, oIN(float3, _InNormalVector)
	, oIN(float4, _InTangentVectorAndFacing)
	, oOUT(float3, _OutNormalVector), oOUT(float3, _OutTangentVector), oOUT(float3, _OutBitangentVector))
{
	_OutNormalVector = qmul(_Quaternion, _InNormalVector);
	_OutTangentVector = qmul(_Quaternion, _InTangentVectorAndFacing.xyz);
	_OutBitangentVector = cross(_OutNormalVector, _OutTangentVector) * _InTangentVectorAndFacing.w;
}

// _____________________________________________________________________________
// Misc

// Given an integer ID [0,255], return a color that ensures IDs near each other 
// (i.e. 13,14,15) have significantly different colors.
inline float3 idtocolor8(uint ID8Bit)
{
	uint R = rand_unmasked(ID8Bit);
	uint G = rand_unmasked(R);
	uint B = rand_unmasked(G);
	return float3(uint3(R,G,B) & 0xff) / 255.0f;
}

// Given an integer ID [0,65535], return a color that ensures IDs near each other 
// (i.e. 13,14,15) have significantly different colors.
inline float3 idtocolor16(uint ID16Bit)
{
	uint R = rand_unmasked(ID16Bit);
	uint G = rand_unmasked(R);
	uint B = rand_unmasked(G);
	return float3(uint3(R,G,B) & 0xffff) / 65535.0f;
}

// Creates a cube in a geometry shader (useful for voxel visualization. To use,
// generate indices in a for (uint i = 0; i < 14; i++) loop (14 iterations).
inline float3 oGSCubeCalcVertexPosition(uint _Index, oIN(float3, _Offset), oIN(float3, _Scale))
{
	static const float3 oGSCubeStripCW[] = 
	{
		float3(-0.5f,0.5f,-0.5f), // left top front
		float3(0.5f,0.5f,-0.5f), // right top front
		float3(-0.5f,-0.5f,-0.5f), // left bottom front
		float3(0.5f,-0.5f,-0.5f), // right bottom front
		float3(0.5f,-0.5f,0.5f), // right bottom back
		float3(0.5f,0.5f,-0.5f), // right top front
		float3(0.5f,0.5f,0.5f), // right top back
		float3(-0.5f,0.5f,-0.5f), // left top front
		float3(-0.5f,0.5f,0.5f), // left top back
		float3(-0.5f,-0.5f,-0.5f), // left bottom front 7
		float3(-0.5f,-0.5f,0.5f), // left bottom back
		float3(0.5f,-0.5f,0.5f), // right bottom back 5
		float3(-0.5f,0.5f,0.5f), // left top back
		float3(0.5f,0.5f,0.5f), // right top back
	};

	return _Offset + oGSCubeStripCW[_Index] * _Scale;
}

#include <oMath/hlsl_swizzles_off.h>
#endif
