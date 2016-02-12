// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include "gpu_pipeline_state.h"
#include "gpu_fixed_state.h"
#include <oCore/countof.h>
#include <oCore/stringf.h>

#include <VSpos_col.h>
#include <PSvertex_color.h>

using namespace ouro::gpu;

namespace ouro { 

const char* as_string(const gfx2::pipeline_state& state)
{
	const char* s_names[] = 
	{
		"unknown",
		"lines",
	};
	match_array_e(s_names, gfx2::pipeline_state);
	return s_names[(int)state];
}

namespace gfx2 {

pipeline_state_desc get_pipeline_state_desc(const pipeline_state& state)
{
	static const pipeline_state_desc s_state[] = 
	{
		pipeline_state_desc(),
		pipeline_state_desc(VSpos_col, PSvertex_color, mesh::basic::pos_col, basic::opaque, basic::two_sided, basic::depth_test_and_write, primitive_type::line),
	};
	match_array_e(s_state, pipeline_state);
	if (state < pipeline_state::unknown || state >= pipeline_state::count)
		throw std::out_of_range(stringf("invalid state %d", (int)state));
	return s_state[(int)state];
}

}}
