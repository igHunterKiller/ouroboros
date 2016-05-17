// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// This header is compiled both by HLSL and C++. It describes a oGfx-level 
// policy encapsulation of per-view values for rasterization of 3D scenes.

#ifndef oHLSL
	#pragma once
#endif
#ifndef oGfx_view_constants_h
#define oGfx_view_constants_h

#include <oGfx/shader_registers.h>
#include <oMath/cube_topology.h>

#ifndef oHLSL
#include <oMath/hlslx.h>
#include <oMath/matrix.h>
#include <oMath/projection.h>
#include <oMath/pov.h>
namespace ouro { namespace gfx {
#else
#ifndef oGFX_DECLARE_VIEW_CONSTANTS
#error oGFX_DECLARE_VIEW_CONSTANTS(name) must be defined, i.e. define oGFX_DECLARE_VIEW_CONSTANTS(name) cbuffer cbuffer_view : register(b0) { view_constants name; }
#endif
#define alignas(x)
#endif

struct alignas(16) view_constants
{
#ifndef oHLSL
	view_constants() { set(kIdentity4x4, kIdentity4x4, 0, 0, 0); }
	view_constants(const float4x4& view, const float4x4& projection, uint32_t target_width, uint32_t target_height, uint32_t array_index) { set(view, projection, target_width, target_height, array_index); }
	void set(const float4x4& view, const float4x4& projection, uint32_t target_width, uint32_t target_height, uint array_index);
	void set(const pov_t& pov, uint32_t array_index);
protected:
#endif

	float4x4 view;
	float4x4 view_inverse;
	float4x4 projection;
	float4x4 projection_inverse;
	float4x4 view_projection;
	float4x4 view_projection_inverse;

	// top left/right bottom left/right ordered in such a way that indexing by
	// vertex id or texcoord is easy.
	float4 vs_far_planes[4]; // 0.w is near_dist, 1.w is far_dist, 2.w is far_dist_inverse, 3.w is unused

	float2 target_dimensions;
	uint array_index;
	float padA;
};

#ifndef oHLSL

static_assert((sizeof(view_constants) & 0xf) == 0, "sizeof(view_constants) must be 16-byte aligned");

inline void view_constants::set(const float4x4& view_, const float4x4& projection_, uint32_t target_width, uint32_t target_height, uint32_t array_index_)
{
	view = view_;
	view_inverse = invert(view);
	projection = projection_;
	projection_inverse = invert(projection);
	view_projection = view * projection;
	view_projection_inverse = invert(view_projection);

	float3 corners[8];
	proj_corners(projection, corners);

	const float near_dist = proj_near(projection);
	const float far_dist = proj_far(projection);
	vs_far_planes[0] = float4(corners[oCUBE_LEFT_TOP_FAR], near_dist);
	vs_far_planes[1] = float4(corners[oCUBE_RIGHT_TOP_FAR], far_dist);
	vs_far_planes[2] = float4(corners[oCUBE_LEFT_BOTTOM_FAR], 1.0f / far_dist);
	vs_far_planes[3] = float4(corners[oCUBE_RIGHT_BOTTOM_FAR], 0.0f);

	target_dimensions = float2(uint2(target_width, target_height));

	array_index = array_index_;
	padA = 0.0f;
}

inline void view_constants::set(const pov_t& pov, uint32_t array_index_)
{
	view                    = pov.view();
	view_inverse            = pov.view_inverse();
	projection              = pov.projection();
	projection_inverse      = pov.projection_inverse();
	view_projection         = pov.view_projection();
	view_projection_inverse = pov.view_projection_inverse();

	float3 corners[8];
	proj_corners(projection, corners);

	const float near_dist = pov.near_dist();
	const float far_dist = pov.far_dist();
	vs_far_planes[0] = float4(corners[oCUBE_LEFT_TOP_FAR], near_dist);
	vs_far_planes[1] = float4(corners[oCUBE_RIGHT_TOP_FAR], far_dist);
	vs_far_planes[2] = float4(corners[oCUBE_LEFT_BOTTOM_FAR], 1.0f / far_dist);
	vs_far_planes[3] = float4(corners[oCUBE_RIGHT_BOTTOM_FAR], 0.0f);

	target_dimensions = pov.viewport().dimensions;
	array_index = array_index_;
	padA = 0.0f;
}

}}
#else
// HLSL accessors to data
oGFX_DECLARE_VIEW_CONSTANTS(oGfx_view_constants);

// NOTE: All screen_texcoords assume 0,0 in upper left to 1,1 in lower right.

float3 gfx_screenuvs2ws_pos(float2 screen_texcoord, float linear_depth)
{
	float3 far_plane_pos = oGfx_view_constants.vs_far_planes[2 * screen_texcoord.y + screen_texcoord.x].xyz;
	return far_plane_pos * linear_depth + oGfx_view_constants.view_inverse[3].xyz;
}

float4 gfx_vs2ss(float3 vs_position)
{
	return mul(oGfx_view_constants.projection, float4(vs_position, 1));
}

float4 gfx_ws2ss(float3 ws_position)
{
	return mul(oGfx_view_constants.view_projection, float4(ws_position, 1));
}

float4 gfx_ws2vs(float3 ws_position)
{
	return mul(oGfx_view_constants.view, float4(ws_position, 1));
}

float gfx_near_clip()
{
	return oGfx_view_constants.vs_far_planes[0].w;
}

float gfx_far_clip()
{
	return oGfx_view_constants.vs_far_planes[1].w;
}

float gfx_far_clip_rcp()
{
	return oGfx_view_constants.vs_far_planes[2].w;
}

float2 gfx_target_dimensions()
{
	return oGfx_view_constants.target_dimensions;
}

float2 gfx_target_array_index()
{
	return oGfx_view_constants.array_index;
}

// returns linear depth in view-space (not normalized) computed from a 
// depth-buffer-sampled hyperbolic value.
float gfx_linearize_depth(float sampled_hyperbolic_depth)
{
	float2 depth_constants = float2(oGfx_view_constants.projection[2][2], oGfx_view_constants.projection[2][3]);
	return depth_constants.y / (sampled_hyperbolic_depth - depth_constants.x);
}

#endif
#endif
