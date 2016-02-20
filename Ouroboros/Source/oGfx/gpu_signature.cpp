// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oCore/countof.h>
#include <oGfx/gpu_signature.h>
#include <oString/stringize.h>

#include <VSpos.h>
#include <VSpos_col.h>
#include <VSpos_uv0.h>
#include <VSpos_nrm_tan_uv0.h>
#include <PSmouse_depth.h>
#include <PSconstant_color.h>
#include <PSvertex_color.h>
#include <PStexcoordu.h>
#include <PStexcoordv.h>
#include <PStexcoord.h>
#include <PStexture2d.h>
#include <PSwhite.h>

using namespace ouro::gpu;

namespace ouro {
	
template<> const char* as_string(const gfx::signature& sig)
{
	const char* s_names[] = 
	{
		"graphics",
	};
	return detail::enum_as(sig, s_names);
}

template<> const char* as_string(const gfx::pipeline_state& state)
{
	const char* s_names[] = 
	{
		"pos_only",
		"pos_only_points",
		"pos_only_wire",
		"pos_color",
		"pos_color_wire",
		"pos_vertex_color",
		"pos_vertex_color_wire",
		"lines_color",
		"lines_vertex_color",
		"mouse_depth",
		"mesh_u0_as_color",
		"mesh_v0_as_color",
		"mesh_uv0_as_color",
		"mesh_simple_texture",
		"mesh_wire",
	};
	
	return detail::enum_as(state, s_names);
}

namespace gfx {

static const sampler_desc s_gfx_signature_samples[] = { basic::point_clamp, basic::point_wrap, basic::linear_clamp, basic::linear_wrap };
static const uint32_t s_gfx_signature_cb_strides[] = { sizeof(frame_constants), sizeof(view_constants), sizeof(draw_constants), sizeof(shadow_constants), sizeof(misc_constants) };
static const uint32_t s_gfx_signature_cb_max[] = { 1, 1, 64, 64, 2 };

const root_signature_desc root_signatures[] = 
{
	root_signature_desc(s_gfx_signature_samples, s_gfx_signature_cb_strides, s_gfx_signature_cb_max),
};
match_array_e(root_signatures, signature);

const pipeline_state_desc pipeline_states[] = 
{
	pipeline_state_desc(VSpos,                   basic::PSwhite,          mesh::basic::pos_col, basic::opaque,      basic::front_face, basic::depth_test_and_write, primitive_type::triangle),
	pipeline_state_desc(VSpos,                   basic::PSwhite,          mesh::basic::pos_col, basic::opaque,      basic::front_face, basic::depth_test_and_write, primitive_type::point   ),
	pipeline_state_desc(VSpos,                   basic::PSwhite,          mesh::basic::pos,     basic::opaque,      basic::front_wire, basic::depth_test_and_write, primitive_type::triangle),
	pipeline_state_desc(VSpos,                   PSconstant_color,        mesh::basic::pos,     basic::translucent, basic::front_face, basic::depth_test,           primitive_type::triangle),
	pipeline_state_desc(VSpos,                   PSconstant_color,        mesh::basic::pos,     basic::translucent, basic::front_wire, basic::depth_test,           primitive_type::triangle),
	pipeline_state_desc(VSpos_col,               PSvertex_color,          mesh::basic::pos_col, basic::opaque,      basic::front_face, basic::depth_test_and_write, primitive_type::triangle),
	pipeline_state_desc(VSpos_col,               PSvertex_color,          mesh::basic::pos_col, basic::opaque,      basic::front_wire, basic::depth_test_and_write, primitive_type::triangle),
	pipeline_state_desc(VSpos_col,               PSconstant_color,        mesh::basic::pos_col, basic::opaque,      basic::front_face, basic::depth_test_and_write, primitive_type::line    ),
	pipeline_state_desc(VSpos_col,               PSvertex_color,          mesh::basic::pos_col, basic::opaque,      basic::front_face, basic::depth_test_and_write, primitive_type::line    ),
	pipeline_state_desc(basic::VSfullscreen_tri, PSmouse_depth,           mesh::basic::pos_col, basic::opaque,      basic::front_face, basic::no_depth_stencil,     primitive_type::triangle),
	pipeline_state_desc(VSpos_nrm_tan_uv0,       PStexcoordu,             mesh::basic::meshf,   basic::opaque,      basic::front_face, basic::depth_test_and_write, primitive_type::triangle),
	pipeline_state_desc(VSpos_nrm_tan_uv0,       PStexcoordv,             mesh::basic::meshf,   basic::opaque,      basic::front_face, basic::depth_test_and_write, primitive_type::triangle),
	pipeline_state_desc(VSpos_nrm_tan_uv0,       PStexcoord,              mesh::basic::meshf,   basic::opaque,      basic::front_face, basic::depth_test_and_write, primitive_type::triangle),
	pipeline_state_desc(VSpos_nrm_tan_uv0,       PStexture2d,             mesh::basic::meshf,   basic::opaque,      basic::front_face, basic::depth_test_and_write, primitive_type::triangle),
	pipeline_state_desc(VSpos_nrm_tan_uv0,       PStexcoord,              mesh::basic::meshf,   basic::opaque,      basic::front_wire, basic::depth_test_and_write, primitive_type::triangle),
};
match_array_e(pipeline_states, pipeline_state);

void sign_device(gpu::device* dev)
{
	dev->new_rsos<signature>(root_signatures);
	dev->new_psos<pipeline_state>(pipeline_states);
}

}}
