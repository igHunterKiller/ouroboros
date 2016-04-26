// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#ifndef oHLSL
	#pragma once
#endif
#ifndef oGfx_gpu_signature_h
#define oGfx_gpu_signature_h

#include <oGfx/gpu_signature_slots.h>
#include <oGfx/vertex_layouts.h>

#ifdef oHLSL

#else
#include <oCore/color.h>
#include <oMath/matrix.h>
#include <oGPU/gpu.h>

namespace ouro { namespace gfx {

enum class signature : uint8_t
{
	graphics,
	count,
};

enum class pipeline_state : uint8_t
{
	pos_only,
	pos_only_points,
	pos_only_wire,
	pos_color,
	pos_color_wire,
	pos_vertex_color,
	pos_vertex_color_stipple,
	pos_vertex_color_wire,
	lines_color,
	lines_vertex_color,
	lines_vertex_color_stipple,
	mouse_depth,
	linearize_depth,
	
	// pos_nrm_tan_uv0
	mesh_u0_as_color,		 //
	mesh_v0_as_color,    //
	mesh_uv0_as_color,   //
	mesh_simple_texture, // sample texture in slot0
	mesh_wire,           // wireframe with UVs as color
	count,
};

// placeholders
struct frame_constants  { float rand; };
struct shadow_constants { float4x4 matrix; };

// Creates root signature objects and pipeline state objects that can
// then be set by enum.
void sign_device(gpu::device* dev);

}}
#endif
#endif
