// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// C++/HLSL definition of the attributes exported from a vertex shader.

#ifndef oHLSL
#pragma once
#endif
#ifndef oGfx_gpu_signature_interpolants_h
#define oGfx_gpu_signature_interpolants_h

#include <oMath/hlsl.h>
#include <oGPU/gpu_semantics.h>

// _____________________________________________________________________________
// HLSL interpolants

struct INTp
{
	float4 SSposition oGPU_SVPOSITION;
	uint   instance   oGPU_SVINSTANCEID;
};

struct INTpbtnu
{
	float4 SSposition  oGPU_SVPOSITION;
	float3 WSposition  oGPU_POSITION1; // Until this can be rebuilt from GBuffer
	float3 WSbitangent oGPU_BITANGENT;
	float3 WStangent   oGPU_TANGENT;
	float3 WSnormal    oGPU_NORMAL;
	float2 texcoord0   oGPU_TEXCOORD;
	uint   instance    oGPU_SVINSTANCEID;
};

struct INTpu
{
	float4 SSposition oGPU_SVPOSITION;
	float2 texcoord0  oGPU_TEXCOORD;
	uint   instance   oGPU_SVINSTANCEID;
};

struct INTpc
{
	float4 SSposition oGPU_SVPOSITION;
	float4 color      oGPU_COLOR;
	uint   instance   oGPU_SVINSTANCEID;
};

#endif
