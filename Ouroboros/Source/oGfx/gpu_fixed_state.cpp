// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#pragma once
#include <oCore/countof.h>
#include <oMesh/element.h>
#include "gpu_fixed_state.h"

using namespace ouro::gpu;
using namespace ouro::mesh;

namespace ouro { namespace gfx2 {

static const element_t s_lines[] = 
{
	{ element_semantic::position, 0, surface::format::r32g32b32_float,    0 },
	{ element_semantic::color,    0, surface::format::b8g8r8a8_unorm,     0 },
};

static const element_t s_geometry[] = 
{
	{ element_semantic::position, 0, surface::format::r32g32b32_float,    0 },
	{ element_semantic::normal,   0, surface::format::r32g32b32_float,    0 },
	{ element_semantic::tangent,  0, surface::format::r32g32b32a32_float, 0 },
	{ element_semantic::texcoord, 0, surface::format::r32g32_float,       0 },
	{ element_semantic::color,    0, surface::format::b8g8r8a8_unorm,     0 },
};

static const element_t s_fullscreen[] = 
{
	{ element_semantic::position, 0, surface::format::r32g32b32_float,    0 },
	{ element_semantic::texcoord, 0, surface::format::r32g32_float,       0 },
	{ element_semantic::misc,     0, surface::format::r32g32b32a32_float, 0 },
};

const element_t* s_input_layouts[] = 
{
	s_lines,
	s_geometry,
	s_fullscreen,
};

const uint8_t s_num_input_elements[] = 
{
	countof(s_lines),
	countof(s_geometry),
	countof(s_fullscreen),
};

static const rt_blend_desc s_rt_blend_descs[] =
{
	{ false, false, blend_type::one,            blend_type::zero,          blend_op::add,  blend_type::one,  blend_type::zero, blend_op::add,  logic_op::noop, color_write_mask::all },
	{ false, false, blend_type::one,            blend_type::zero,          blend_op::add,  blend_type::one,  blend_type::zero, blend_op::add,  logic_op::noop, color_write_mask::all },
	{ true,  false, blend_type::one,            blend_type::one,           blend_op::add,  blend_type::one,  blend_type::one,  blend_op::add,  logic_op::noop, color_write_mask::all },
	{ true,  false, blend_type::src_alpha,      blend_type::one,           blend_op::add,  blend_type::zero, blend_type::zero, blend_op::add,  logic_op::noop, color_write_mask::all },
	{ true,  false, blend_type::dest_color,     blend_type::zero,          blend_op::add,  blend_type::zero, blend_type::zero, blend_op::add,  logic_op::noop, color_write_mask::all },
	{ true,  false, blend_type::inv_dest_color, blend_type::one,           blend_op::add,  blend_type::zero, blend_type::zero, blend_op::add,  logic_op::noop, color_write_mask::all },
	{ true,  false, blend_type::src_alpha,      blend_type::inv_src_alpha, blend_op::add,  blend_type::zero, blend_type::zero, blend_op::add,  logic_op::noop, color_write_mask::all },
	{ true,  false, blend_type::one,            blend_type::one,           blend_op::min_, blend_type::one,  blend_type::one,  blend_op::min_, logic_op::noop, color_write_mask::all },
	{ true,  false, blend_type::one,            blend_type::one,           blend_op::max_, blend_type::one,  blend_type::one,  blend_op::max_, logic_op::noop, color_write_mask::all },
};

static const rasterizer_desc s_rasterizer_descs[] = 
{
	{ false, cull_mode::front, false, false, 0, 0.0f, 0.0f, true, false, false, false, 0 },
	{ false, cull_mode::back,  false, false, 0, 0.0f, 0.0f, true, false, false, false, 0 },
	{ false, cull_mode::none,  false, false, 0, 0.0f, 0.0f, true, false, false, false, 0 },
	{ true,  cull_mode::front, false, false, 0, 0.0f, 0.0f, true, false, false, false, 0 },
	{ true,  cull_mode::back,  false, false, 0, 0.0f, 0.0f, true, false, false, false, 0 },
	{ true,  cull_mode::none,  false, false, 0, 0.0f, 0.0f, true, false, false, false, 0 },
};

static const depth_stencil_desc s_depth_stencil_descs[] = 
{
	{ false, false, comparison::always,     false, 0xff, 0xff, stencil_op::keep, stencil_op::keep, stencil_op::keep, comparison::always, stencil_op::keep, stencil_op::keep, stencil_op::keep, comparison::always, },
	{ true,  true,  comparison::less_equal, false, 0xff, 0xff, stencil_op::keep, stencil_op::keep, stencil_op::keep, comparison::always, stencil_op::keep, stencil_op::keep, stencil_op::keep, comparison::always, },
	{ true,  false, comparison::less_equal, false, 0xff, 0xff, stencil_op::keep, stencil_op::keep, stencil_op::keep, comparison::always, stencil_op::keep, stencil_op::keep, stencil_op::keep, comparison::always, },

};

const gpu::rt_blend_desc& get_rt_blend_state(const rt_blend_state& state) { return s_rt_blend_descs[(int)state]; }
const gpu::rasterizer_desc& get_rasterizer_desc(const rasterizer_state& state) { return s_rasterizer_descs[(int)state]; }
const gpu::depth_stencil_desc& get_depth_stencil_desc(const depth_stencil_state& state) { return s_depth_stencil_descs[(int)state]; }
const mesh::element_t* get_input_layout_desc(const input_layout& layout, uint32_t* out_num_elements) { *out_num_elements = s_num_input_elements[(int)layout]; return s_input_layouts[(int)layout]; }

}}
