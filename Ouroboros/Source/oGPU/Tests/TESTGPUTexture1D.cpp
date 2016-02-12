// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include "gpu_test_common.h"

using namespace ouro;
using namespace ouro::gpu;

static const bool kIsDevMode = false;

struct gpu_test_texture1d : public tests::gpu_texture_test
{
	gpu_test_texture1d() : gpu_texture_test("GPU test: texture 1D", kIsDevMode) {}

	ref<gpu::srv> init(tests::pipeline_state* out_pipeline_state) override
	{
		*out_pipeline_state = tests::pipeline_state::pos_tex1d;
		auto image = tests::make_1D(512, false);
		return dev_->new_texture("Test 1D", image);
	}
};

oGPU_COMMON_TEST(texture1d);
