// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include "gpu_pipeline_state.h"
#include <oCore/countof.h>
#include <oMesh/element.h>

#include <VStest_buffer.h>
#include <VStest_pass_through_color.h>
#include <VStest_pos.h>
#include <VStest_texture1d.h>
#include <VStest_texture2d.h>
#include <VStest_texture3d.h>
#include <VStest_texturecube.h>
#include <VStest_color.h>
#include <PStest_buffer.h>
#include <PStest_color.h>
#include <PStest_texcoords_as_color.h>
#include <PStest_texture1d.h>
#include <PStest_texture2d.h>
#include <PStest_texture3d.h>
#include <PStest_texturecube.h>

using namespace ouro::gpu;

namespace ouro { 
	
const char* as_string(const tests::pipeline_state& state)
{
	const char* s_names[] = 
	{
		"unknown",
		"buffer_test",
		"pos_untransformed",
		"pos_color",
		"lines_untransformed",
		"geometry_texcoords_as_color",
		"geometry_texture1d_only",
		"geometry_texture2d_only",
		"geometry_texture3d_only",
		"geometry_texturecube_only",
	};
	match_array_e(s_names, tests::pipeline_state);
	return s_names[(int)state];
}

	namespace tests {

root_signature_desc get_root_signature_desc()
{
	root_signature_desc desc;
	desc.num_samplers = 1;
	desc.samplers = &basic::linear_wrap;
	desc.num_cbvs = 1;
	
	static const uint32_t stride = sizeof(test_constants);
	static const uint32_t max_structs = 2;
	
	desc.struct_strides = &stride;
	desc.max_num_structs = &max_structs;

	return desc;
}

pipeline_state_desc get_pipeline_state_desc(const pipeline_state& state)
{
	static const pipeline_state_desc s_state[] = 
	{
		pipeline_state_desc(),
		pipeline_state_desc(VStest_buffer,             PStest_buffer,             mesh::basic::pos,     basic::opaque, basic::front_face, basic::no_depth_stencil, primitive_type::point),
		pipeline_state_desc(basic::VSpass_through_pos, basic::PSwhite,            mesh::basic::pos,     basic::opaque, basic::two_sided,  basic::no_depth_stencil                       ),
		pipeline_state_desc(VStest_pos,                basic::PSwhite,            mesh::basic::pos,     basic::opaque, basic::two_sided,  basic::depth_test_and_write                   ),
		pipeline_state_desc(VStest_pass_through_color, PStest_color,              mesh::basic::pos_col, basic::opaque, basic::two_sided,  basic::no_depth_stencil, primitive_type::line ),
		pipeline_state_desc(VStest_texture2d,          PStest_texcoords_as_color, mesh::basic::pos_uv0, basic::opaque, basic::front_face, basic::depth_test_and_write                   ),
		pipeline_state_desc(VStest_texture1d,          PStest_texture1d,          mesh::basic::pos_uv0, basic::opaque, basic::front_face, basic::depth_test_and_write                   ),
		pipeline_state_desc(VStest_texture2d,          PStest_texture2d,          mesh::basic::pos_uv0, basic::opaque, basic::front_face, basic::depth_test_and_write                   ),
		pipeline_state_desc(VStest_texture3d,          PStest_texture3d,          mesh::basic::pos_uvw, basic::opaque, basic::front_face, basic::depth_test_and_write                   ),
		pipeline_state_desc(VStest_texturecube,        PStest_texturecube,        mesh::basic::pos_uvw, basic::opaque, basic::front_face, basic::depth_test_and_write                   ),
	};
	match_array_e(s_state, pipeline_state);
	
	if (state < pipeline_state::unknown || state >= pipeline_state::count)
		throw std::out_of_range(stringf("invalid state %d", (int)state));
	
	return s_state[(int)state];
}

}}
