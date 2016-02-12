// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include "gpu_test_common.h"

using namespace ouro;
using namespace ouro::gpu;

static const bool kIsDevMode = false;

struct gpu_test_texturecubemip : public tests::gpu_texture_test
{
	gpu_test_texturecubemip() : gpu_texture_test("GPU test: texturecube mip", kIsDevMode) {}

	ref<gpu::srv> init(tests::pipeline_state* out_pipeline_state) override
	{
		*out_pipeline_state = tests::pipeline_state::pos_texcube;
		auto _0 = tests::surface_load(filesystem::data_path() / "Test/Textures/CubePosX.png", false);
		auto _1 = tests::surface_load(filesystem::data_path() / "Test/Textures/CubeNegX.png", false);
		auto _2 = tests::surface_load(filesystem::data_path() / "Test/Textures/CubePosY.png", false);
		auto _3 = tests::surface_load(filesystem::data_path() / "Test/Textures/CubeNegY.png", false);
		auto _4 = tests::surface_load(filesystem::data_path() / "Test/Textures/CubePosZ.png", false);
		auto _5 = tests::surface_load(filesystem::data_path() / "Test/Textures/CubeNegZ.png", false);

		const surface::image* images[6] = { &_0, &_1, &_2, &_3, &_4, &_5 };
		surface::image image;
		image.initialize_array(images, 6, true);
		image.set_semantic(surface::semantic::customcube);
		return dev_->new_texture("Test cube", image);
	}
};

oGPU_COMMON_TEST(texturecubemip);
