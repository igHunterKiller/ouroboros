// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oCore/countof.h>
#include <oGfx/gpu_signature.h>
#include <oCore/stringize.h>
#include <oGfxShaders.h>

using namespace ouro::gpu;

namespace ouro {
	
template<> const char* as_string(const gfx::signature& sig)
{
	const char* s_names[] = 
	{
		"graphics",
	};
	return as_string(sig, s_names);
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
		"pos_vertex_color_stipple",
		"pos_vertex_color_wire",
		"lines_color",
		"lines_vertex_color",
		"lines_vertex_color_stipple",
		"mouse_depth",
		"linearize_depth",
		"mesh_u0_as_color",
		"mesh_v0_as_color",
		"mesh_uv0_as_color",
		"mesh_normalx_as_color",
		"mesh_normaly_as_color",
		"mesh_normalz_as_color",
		"mesh_normal_as_color",
		"mesh_tangentx_as_color",
		"mesh_tangenty_as_color",
		"mesh_tangentz_as_color",
		"mesh_tangent_as_color",
		"mesh_bitangentx_as_color",
		"mesh_bitangenty_as_color",
		"mesh_bitangentz_as_color",
		"mesh_bitangent_as_color",
		"mesh_simple_texture",
		"mesh_wire",
	};
	
	return as_string(state, s_names);
}

namespace gfx {

static const sampler_desc s_gfx_signature_samples   [] = { basic::point_clamp, basic::point_wrap, basic::linear_clamp, basic::linear_wrap };
static const uint32_t     s_gfx_signature_cb_strides[] = { sizeof(frame_constants), sizeof(view_constants), sizeof(draw_constants), sizeof(shadow_constants), sizeof(misc_constants) };
static const uint32_t     s_gfx_signature_cb_max    [] = { 1, 1, 64, 64, 2 };

const root_signature_desc root_signatures[] = 
{
	root_signature_desc(s_gfx_signature_samples, s_gfx_signature_cb_strides, s_gfx_signature_cb_max),
};
match_array_e(root_signatures, signature);

const pipeline_state_desc pipeline_states[] = 
{
	/* pos_only                   */ pipeline_state_desc(VSp,                     basic::PSwhite,          mesh::basic::pos_col, basic::opaque,      basic::front_face, basic::depth_test_and_write, primitive_type::triangle),
	/* pos_only_points            */ pipeline_state_desc(VSp,                     basic::PSwhite,          mesh::basic::pos_col, basic::opaque,      basic::front_face, basic::depth_test_and_write, primitive_type::point   ),
	/* pos_only_wire              */ pipeline_state_desc(VSp,                     basic::PSwhite,          mesh::basic::pos,     basic::opaque,      basic::front_wire, basic::depth_test_and_write, primitive_type::triangle),
	/* pos_color                  */ pipeline_state_desc(VSp,                     PSconstant_color,        mesh::basic::pos,     basic::translucent, basic::front_face, basic::depth_test,           primitive_type::triangle),
	/* pos_color_wire             */ pipeline_state_desc(VSp,                     PSconstant_color,        mesh::basic::pos,     basic::translucent, basic::front_wire, basic::depth_test,           primitive_type::triangle),
	/* pos_vertex_color           */ pipeline_state_desc(VSpc,                    PSpc_color,              mesh::basic::pos_col, basic::opaque,      basic::front_face, basic::depth_test_and_write, primitive_type::triangle),
	/* pos_vertex_color_stipple   */ pipeline_state_desc(VSpc,                    PSdepth_stippled,        mesh::basic::pos_col, basic::opaque,      basic::front_face, basic::no_depth_stencil,     primitive_type::triangle),
	/* pos_vertex_color_wire      */ pipeline_state_desc(VSpc,                    PSpc_color,              mesh::basic::pos_col, basic::opaque,      basic::front_wire, basic::depth_test_and_write, primitive_type::triangle),
	/* lines_color                */ pipeline_state_desc(VSpc,                    PSconstant_color,        mesh::basic::pos_col, basic::opaque,      basic::front_face, basic::depth_test_and_write, primitive_type::line    ),
	/* lines_vertex_color         */ pipeline_state_desc(VSpc,                    PSpc_color,              mesh::basic::pos_col, basic::opaque,      basic::front_face, basic::depth_test_and_write, primitive_type::line    ),
	/* lines_vertex_color_stipple */ pipeline_state_desc(VSpc,                    PSdepth_stippled,        mesh::basic::pos_col, basic::opaque,      basic::front_face, basic::no_depth_stencil,     primitive_type::line    ),
	/* mouse_depth                */ pipeline_state_desc(basic::VSfullscreen_tri, PSmouse_depth,           mesh::basic::pos_col, basic::opaque,      basic::front_face, basic::no_depth_stencil,     primitive_type::triangle),
	/* linearize_depth            */ pipeline_state_desc(basic::VSfullscreen_tri, PSlinearize_depth,       mesh::basic::pos,     basic::opaque,      basic::front_face, basic::no_depth_stencil,     primitive_type::triangle),
	/* mesh_bitangentx_as_color   */ pipeline_state_desc(VSpntu,                  PSpbtnu_bitangentx,      mesh::basic::meshf,   basic::opaque,      basic::front_face, basic::depth_test_and_write, primitive_type::triangle),
	/* mesh_bitangenty_as_color   */ pipeline_state_desc(VSpntu,                  PSpbtnu_bitangenty,      mesh::basic::meshf,   basic::opaque,      basic::front_face, basic::depth_test_and_write, primitive_type::triangle),
	/* mesh_bitangentz_as_color   */ pipeline_state_desc(VSpntu,                  PSpbtnu_bitangentz,      mesh::basic::meshf,   basic::opaque,      basic::front_face, basic::depth_test_and_write, primitive_type::triangle),
	/* mesh_bitangent_as_color    */ pipeline_state_desc(VSpntu,                  PSpbtnu_bitangent,       mesh::basic::meshf,   basic::opaque,      basic::front_face, basic::depth_test_and_write, primitive_type::triangle),
	/* mesh_tangentx_as_color     */ pipeline_state_desc(VSpntu,                  PSpbtnu_tangentx,        mesh::basic::meshf,   basic::opaque,      basic::front_face, basic::depth_test_and_write, primitive_type::triangle),
	/* mesh_tangenty_as_color     */ pipeline_state_desc(VSpntu,                  PSpbtnu_tangenty,        mesh::basic::meshf,   basic::opaque,      basic::front_face, basic::depth_test_and_write, primitive_type::triangle),
	/* mesh_tangentz_as_color     */ pipeline_state_desc(VSpntu,                  PSpbtnu_tangentz,        mesh::basic::meshf,   basic::opaque,      basic::front_face, basic::depth_test_and_write, primitive_type::triangle),
	/* mesh_tangent_as_color      */ pipeline_state_desc(VSpntu,                  PSpbtnu_tangent,         mesh::basic::meshf,   basic::opaque,      basic::front_face, basic::depth_test_and_write, primitive_type::triangle),
	/* mesh_normalx_as_color      */ pipeline_state_desc(VSpntu,                  PSpbtnu_normalx,         mesh::basic::meshf,   basic::opaque,      basic::front_face, basic::depth_test_and_write, primitive_type::triangle),
	/* mesh_normaly_as_color      */ pipeline_state_desc(VSpntu,                  PSpbtnu_normaly,         mesh::basic::meshf,   basic::opaque,      basic::front_face, basic::depth_test_and_write, primitive_type::triangle),
	/* mesh_normalz_as_color      */ pipeline_state_desc(VSpntu,                  PSpbtnu_normalz,         mesh::basic::meshf,   basic::opaque,      basic::front_face, basic::depth_test_and_write, primitive_type::triangle),
	/* mesh_normal_as_color       */ pipeline_state_desc(VSpntu,                  PSpbtnu_normal,          mesh::basic::meshf,   basic::opaque,      basic::front_face, basic::depth_test_and_write, primitive_type::triangle),
	/* mesh_u0_as_color           */ pipeline_state_desc(VSpntu,                  PSpbtnu_texcoord0_u,     mesh::basic::meshf,   basic::opaque,      basic::front_face, basic::depth_test_and_write, primitive_type::triangle),
	/* mesh_v0_as_color           */ pipeline_state_desc(VSpntu,                  PSpbtnu_texcoord0_v,     mesh::basic::meshf,   basic::opaque,      basic::front_face, basic::depth_test_and_write, primitive_type::triangle),
	/* mesh_uv0_as_color          */ pipeline_state_desc(VSpntu,                  PSpbtnu_texcoord0,       mesh::basic::meshf,   basic::opaque,      basic::front_face, basic::depth_test_and_write, primitive_type::triangle),
	/* mesh_simple_texture        */ pipeline_state_desc(VSpntu,                  PSpbtnu_texture2d,       mesh::basic::meshf,   basic::opaque,      basic::front_face, basic::depth_test_and_write, primitive_type::triangle),
	/* mesh_wire                  */ pipeline_state_desc(VSpntu,                  PSpbtnu_texcoord0,       mesh::basic::meshf,   basic::opaque,      basic::front_wire, basic::depth_test_and_write, primitive_type::triangle),
};
match_array_e(pipeline_states, pipeline_state);

void sign_device(gpu::device* dev)
{
	dev->new_rsos<signature>(root_signatures);
	dev->new_psos<pipeline_state>(pipeline_states);
}

}}
