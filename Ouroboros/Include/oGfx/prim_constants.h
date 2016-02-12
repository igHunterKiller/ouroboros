// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Constant buffer contents for drawing oGfx primitive objects

#ifndef oGfx_prim_constants_h
#define oGfx_prim_constants_h

#ifndef oHLSL
#include <oMath/hlsl.h>
#else
	#ifndef oGFX_DECLARE_PRIM_CONSTANTS
		#error oGFX_DECLARE_PRIM_CONSTANTS(name) must be defined, i.e. define oGFX_DECLARE_PRIM_CONSTANTS(name) cbuffer cbuffer_prim : register(b0) { prim_constants name; }
	#endif
#endif

struct prim_constants
{
	float4x4 world_view_projection;
	float4x4 world;

	// a simple lighting model
	float4 ambient_color;
	float4 diffuse_color;
	float3 light_dir;

	// shader time [0,1] looping
	float time;

	// used with ushort4 positions that store [0,65535] -> [-1,1], finish 
	// uncompressing by multiplying by this number.
	float vertex_scale;

	// index into a texture array
	uint slice;

	// Unused
	uint padA;
	uint padB;
};

#ifdef oHLSL
// _____________________________________________________________________________
// CB and interpolants

oGFX_DECLARE_PRIM_CONSTANTS(gfx_prim_constants[2]);

// _____________________________________________________________________________
// Accessors (either VS or PS)

float gfx_prim_time(uint instance_id)
{
	return gfx_prim_constants[instance_id].time;
}

uint gfx_prim_slice(uint instance_id)
{
	return gfx_prim_constants[instance_id].slice;
}

// _____________________________________________________________________________
// Vertex Shader Functions

float4 gfx_prim_ls2ss(uint instance_id, float3 LSposition)
{
	return mul(gfx_prim_constants[instance_id].world_view_projection, float4(LSposition, 1));
}

float3 gfx_prim_ls2ws(uint instance_id, float3 LSposition)
{
	return mul(gfx_prim_constants[instance_id].world, float4(LSposition, 1)).xyz;
}

float3 gfx_prim_rotate_ls2ws(uint instance_id, float3 LSvector)
{
	return mul((float3x3)gfx_prim_constants[instance_id].world, LSvector);
}

float3 gfx_prim_decode_position(uint instance_id, int3 encoded)
{
	return encoded * gfx_prim_constants[instance_id].vertex_scale;
}

// _____________________________________________________________________________
// Pixel Shader Functions

float4 gfx_prim_lit(uint instance_id, float3 WSnormal)
{
	float4 A = gfx_prim_constants[instance_id].ambient_color;
	float4 D = gfx_prim_constants[instance_id].diffuse_color;
	float3 L = gfx_prim_constants[instance_id].light_dir;
	return max(0.0f, dot(WSnormal, L)) * D + A;
}
	
#endif
#endif
