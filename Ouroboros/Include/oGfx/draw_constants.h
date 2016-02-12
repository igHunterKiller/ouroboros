// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Constant buffer contents for drawing oGfx objects

#ifndef oHLSL
	#pragma once
#endif
#ifndef oGfx_draw_constants_h
#define oGfx_draw_constants_h

#include <oCompute/oComputeUtil.h>

#ifndef oHLSL
#include <oCore/color.h>
#include <oMath/matrix.h>
namespace ouro { namespace gfx {
#else
#ifndef oGFX_DECLARE_DRAW_CONSTANTS
#error oGFX_DECLARE_DRAW_CONSTANTS(name) must be defined, i.e. define oGFX_DECLARE_DRAW_CONSTANTS(name) cbuffer cbuffer_draw : register(b0) { draw_constants name; }
#endif
#endif

#define DRAW_SKINNED 1            // set if geometry has valid dynamic vertices representing a skinned pose

struct draw_constants
{
	float4x4 world;                 // for anything that requires world-space
	float4x4 world_view_projection; // for SV_Position
	float3x4 world_rotation;        // padded float3x3 rotation-only matrix for normals
	float4   color;                 // one color that represents the object
	
	float    vertex_scale;          // positions are stored a SNORM across this scale
	uint     vertex_offset;         // ~0u is an invalid offset and rendering should occur normally. If valid, index into the dynamic vertices buffer.
	uint     object_id;             // unique per instance of an object
	uint     draw_id;               // unique per draw call

	float    time;                  // shader time [0,1] looping
	uint     flags;                 // bitmask of oDRAW_FLAGS*
	uint     slice;                 // index into a texture array
	uint     pada;

	// Note: slice will be like a 4-bit or 8-bit number, so maybe combine with draw_id, flags, or vertex offset?

#ifndef oHLSL
	draw_constants() { set_default(); }
	draw_constants(const float4x4& _world, const float4x4& view, const float4x4& projection) { set_transform(_world, view, projection); set_default_attributes(); }

	inline void set_default_attributes()
	{
		color         = float4(1.0f, 1.0f, 1.0f, 0.0f);
		vertex_scale  = 1.0f;
		vertex_offset = ~0u;
		object_id     = 0;
		draw_id       = 0;
		time          = 0.0f;
		flags         = 0;
		slice         = 0;
		pada          = 0;
	}

	inline void set_default()
	{
		set_transform(kIdentity4x4, kIdentity4x4, kIdentity4x4);
		set_default_attributes();
	}

	inline void set_transform(const float4x4& _world, const float4x4& view, const float4x4& projection)
	{
		world = _world;
		world_view_projection = world * view * projection;
		world_rotation = remove_scale((float3x4)world);
	}
#endif
};

#ifndef oHLSL
static_assert((sizeof(draw_constants) & 0xf) == 0, "sizeof(draw_constants) must be 16-byte aligned");
}}
#else
	// HLSL accessors to data
	oGFX_DECLARE_DRAW_CONSTANTS(gfx_draw_constants[2]);

	float4 gfx_ls2ss(uint instance_id, float3 LSposition)
	{
		return mul(gfx_draw_constants[instance_id].world_view_projection, float4(LSposition, 1));
	}

	float3 gfx_ls2ws(uint instance_id, float3 LSposition)
	{
		return mul(gfx_draw_constants[instance_id].world, float4(LSposition, 1)).xyz;
	}

	float3 oGfxRotateLStoWS(uint instance_id, float3 LSvector)
	{
		return mul((float3x3)gfx_draw_constants[instance_id].world, LSvector);
	}
	
	void oGfxRotateBasisLStoWS(uint instance_id, float3 LSnormal, float4 LStangent, out float3 out_WSnormal, out float3 out_WStangent, out float3 out_WSbitangent)
	{
		oTransformTangentBasisVectors(gfx_draw_constants[instance_id].world, LSnormal, LStangent, out_WSnormal, out_WStangent, out_WSbitangent);
	}

	uint oGfxGetObjectID(uint instance_id)
	{
		return gfx_draw_constants[instance_id].object_id;
	}

	uint oGfxGetDrawID(uint instance_id)
	{
		return gfx_draw_constants[instance_id].draw_id;
	}

	uint oGfxGetSlice(uint instance_id)
	{
		return gfx_draw_constants[instance_id].slice;
	}

	float4 oGfxGetColor(uint instance_id)
	{
		return gfx_draw_constants[instance_id].color;
	}

#endif
#endif
