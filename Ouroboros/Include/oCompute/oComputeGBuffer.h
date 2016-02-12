// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.
// This code contains code that cross-compiles in C++ and HLSL. This contains
// encode/decode functions for manipulating data into a format that can be 
// stored in a geometry buffer (GBuffer) for deferred/inferred rendering.
#ifndef oHLSL
	#pragma once
#endif
#ifndef oComputeGBuffer_h
#define oComputeGBuffer_h

#include <oMath/hlsl.h>
#include <oMath/hlsl_swizzles_on.h>
#include <oCompute/oComputeUtil.h>// fix me

// Generates a clipspace-ish quad based off the SVVertexID semantic in a vertex
// shader.
// VertexID Texcoord Position
// 0        0,0      -1,1,0
// 1        1,0      1,1,0
// 2        0,1      -1,-1,0
// 3        1,1      1,-1,0
inline void oExtractQuadInfoFromVertexID(uint _SVVertexID, oOUT(float4, _Position), oOUT(float2, _Texcoord))
{
	_Texcoord = float2(float(_SVVertexID & 1), float(_SVVertexID >> 1));
	_Position = float4(_Texcoord * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
}

// Returns texcoords of screen: [0,0] upper-left, [1,1] lower-right
// _SVPosition is a local-space 3D position multiplied by a WVP matrix, so the
// final projected position that would normally be passed from a vertex shader
// to a pixel shader.
// NOTE: There are 2 flavors because the _SVPosition behaves slightly 
// differently between the result calculated in a vertex shader and what happens
// to it by the time it gets to the pixel shader.
inline float2 oCalcSSTexcoordVS(oIN(float4, _SVPosition))
{
	float2 Texcoord = _SVPosition.xy / _SVPosition.w;
	return Texcoord * float2(0.5f, -0.5f) + 0.5f;
}

inline float2 oCalcSSTexcoordPS(oIN(float4, _SVPosition), oIN(float2, _RenderTargetDimensions))
{
	return _SVPosition.xy / _RenderTargetDimensions;
}

// Transform depth values to [0,1] to maximize precision. Near/Far are the 
// parameters for the projection matrix.
inline float oNormalizeDepth(float _Depth, float _Near, float _Far)
{
	return (_Depth - _Near) / (_Far - _Near);
}

// Undo the normalization done by oNormalizeDepth().
inline float oUnnormalizeDepth(float _NormalizedDepth, float _Near, float _Far)
{
	// (ViewspaceZ / FarPlane) == (NormalizedDepth * ( FarPlane - NearPlane) + NearPlane ) / FarPlane
	// This refactors to:
	return _NormalizedDepth + (_Near * (1 - _NormalizedDepth)) / _Far;
}

// When writing a normal to a screen buffer, it's not useful to have normals 
// that point away from the screen and thus won't be evaluated, so get that 
// precision back by mapping a normal that could point anywhere on a unit sphere 
// into a half-sphere.
inline float2 oFullToHalfSphere(oIN(float3, _NormalVector))
{
	// From Inferred Lighting, Kicher, Lawrance @ Volition
	return normalize(_NormalVector + float3(0.0f, 0.0f, 1.0f)).xy;
}

// Given the XY of a normal, recreate Z and remap from a half-sphere to a full-
// sphere normal.
inline float3 oHalfToFullSphere(oIN(float2, Nxy))
{
	// From Inferred Lighting, Kicher, Lawrance @ Volition
	// Restores Z value from a normal's XY on a half sphere
	float z = sqrt(1.0f - dot(Nxy, Nxy));
	return float3(2.0f * z * Nxy.x, 2.0f * z * Nxy.y, (2.0f * z * z) - 1.0f);
}

// Given the rotation of a normal from _UpVector, create a half-sphere encoded 
// version fit for use in deferred rendering.
inline float2 oEncodeQuaternionNormal(oIN(float4, _NormalRotationQuat), oIN(float3, _UpVector), bool _IsFrontFace)
{
	const float3 up = _IsFrontFace ? _UpVector : -_UpVector;
	return oFullToHalfSphere(qmul(normalize(_NormalRotationQuat), up));
}

// Returns a normal as encoded by oEncodeQuaternionNormal()
inline float3 oDecodeQuaternionNormal(oIN(float2, _EncodedNormal))
{
	return oHalfToFullSphere(_EncodedNormal);
}

#include <oMath/hlsl_swizzles_off.h>
#endif
