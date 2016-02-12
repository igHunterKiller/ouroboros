// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include "gpu_test_common.h"

using namespace ouro;
using namespace ouro::gpu;

static const bool kIsDevMode = false;

struct gpu_test_texture2dmip : public tests::gpu_texture_test
{
	gpu_test_texture2dmip() : gpu_texture_test("GPU test: texture2dmip", kIsDevMode) {}

	ref<gpu::srv> init(tests::pipeline_state* out_pipeline_state) override
	{
		*out_pipeline_state = tests::pipeline_state::pos_tex2d;
		auto image = tests::surface_load(filesystem::data_path() / "Test/Textures/lena_1.png", true);
		return dev_->new_texture("Test 2D", image);
	}
};

oGPU_COMMON_TEST(texture2dmip);
