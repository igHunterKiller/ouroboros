// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// C++/HLSL definition of the various vertices used in line, model and 
// dynamic geometry definitions.

#ifndef oHLSL
	#pragma once
#endif
#ifndef oGfx_gpu_signature_vertices_h
#define oGfx_gpu_signature_vertices_h

#include <oMath/hlsl.h>
#include <oGPU/gpu_semantics.h>

#ifndef oHLSL
namespace ouro { namespace gfx {
#endif

struct VTXp
{
	float3 position oGPU_POSITION;
};

struct VTXpnt
{
	float3 position oGPU_POSITION;
	float3 normal   oGPU_NORMAL;
	float4 tangent  oGPU_TANGENT;
};

struct VTXpc
{
	float3      position oGPU_POSITION;
	oHLSL_COLOR color    oGPU_COLOR;
};

struct VTXpntu
{
	float3 position  oGPU_POSITION;
	float3 normal    oGPU_NORMAL;
	float4 tangent   oGPU_TANGENT;
	float2 texcoord0 oGPU_TEXCOORD0;
};

struct VTXu
{
	float2 texcoord0 oGPU_TEXCOORD0;
};

struct VTXc
{
	oHLSL_COLOR color oGPU_COLOR;
};

struct VTXmesh
{
	uint2 position   oGPU_POSITION; // ushort3 position in xyz, curvature in w (pos is a unorm scaled to [-1,1] like a normal map, then scaled by a constant)
	uint  quaternion oGPU_NORMAL;   // udec3 quaternion representing the tangent basis
	uint  texcoord0  oGPU_TEXCOORD; // half2 uvs
};

#ifndef oHLSL
}}
#endif
#endif
