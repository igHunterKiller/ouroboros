// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#ifndef oHLSL
	#pragma once
#endif
#ifndef oGfx_gpu_signature_h
#define oGfx_gpu_signature_h

#include <oMath/hlsl.h>
#include <oGfx/gpu_signature_interpolants.h>
#include <oGfx/gpu_signature_slots.h>
#include <oGfx/gpu_signature_vertices.h>

#ifndef oHLSL
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
	
	// mesh::basic::meshf
	mesh_u0_as_color,         //
	mesh_v0_as_color,         //
	mesh_uv0_as_color,        //
	mesh_bitangentx_as_color, //
	mesh_bitangenty_as_color, //
	mesh_bitangentz_as_color, //
	mesh_bitangent_as_color,  //
	mesh_tangentx_as_color,   //
	mesh_tangenty_as_color,   //
	mesh_tangentz_as_color,   //
	mesh_tangent_as_color,    //
	mesh_normalx_as_color,    //
	mesh_normaly_as_color,    //
	mesh_normalz_as_color,    //
	mesh_normal_as_color,     //
	mesh_simple_texture,      // sample texture in slot0
	mesh_wire,                // wireframe with UVs as color
	count,
};

// Creates root signature objects and pipeline state objects that can
// then be set by enum.
void sign_device(gpu::device* dev);

}}
#endif
#endif
