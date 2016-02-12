// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// @tony: disabled after move to gpu.h
#if 0

#pragma once

#include <oBase/plan.h>
#include <oGfx/renderer.h>
#include <oGfx/vertex_layouts.h>

namespace ouro { namespace gpu { 
	
class command_list;
class basic_color_target;

}}

namespace ouro { namespace gfx {

class camera;

struct ctx_technique
{
	gpu::command_list* cl;
	core* core;
	const camera* camera;
	const gpu::primary_target* primary_target;
};

struct ctx_clear_color_target
{
	const gpu::basic_color_target* target;
	uint32_t clear_color;
	uint32_t array_index;
};

struct ctx_clear_depth_target
{
	const gpu::depth_target* target;
	uint32_t array_index;
	float depth;
	uint8_t stencil;
	gpu::depth_target::clear_type clear_type; 
	uint8_t padA;
	uint8_t padB;
};

struct ctx_draw
{
	float4x4 transform;
	model_t model;
	texture2d_t texture;
	uint32_t color;
};

struct ctx_prim
{
	float4x4 transform;
	model_t model;
	bool use_texture;
	bool use_vertex_colors;
	bool use_color;
	uint32_t color;
	texture2d_t texture;
};

struct ctx_lines
{
	// allocate this using the render plan thread's allocate() 
	// function so it persists for the frame but then is recycled
	// there are two vertices per line
	
	VTXpos_col* line_vertices;
	uint32_t num_vertices;
};

enum class technique : uint8_t
{
	init_technique_context, // ctx_technique, only one allowed
	init_view_constants,    // nullptr, only one allowed
	clear_color_target,     // ctx_clear_color_target
	clear_depth_target,     // ctx_clear_depth_target
	draw_model_forward,     // ctx_draw
	draw_lines,             // ctx_lines
	count,
};

extern const technique_t gRenderTechniques[];

}}

#endif