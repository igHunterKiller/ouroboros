// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#pragma once
#include <oGPU/gpu.h>
#include "gpu_test_constants.h"

namespace ouro { namespace tests {

enum class pipeline_state
{
	unknown,
	
	buffer_test,

	pos_untransformed,
	pos,

	lines_untransformed,

	pos_texcoords_as_color,
	pos_tex1d,
	pos_tex2d,
	pos_tex3d,
	pos_texcube,

	count,
};

gpu::root_signature_desc get_root_signature_desc();
gpu::pipeline_state_desc get_pipeline_state_desc(const pipeline_state& state);

}}
