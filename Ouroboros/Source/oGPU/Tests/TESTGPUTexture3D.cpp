// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include "gpu_test_common.h"

using namespace ouro;
using namespace ouro::gpu;

static const bool kIsDevMode = false;

struct gpu_test_texture3d : public tests::gpu_texture_test
{
	gpu_test_texture3d() : gpu_texture_test("GPU test: texture3d", kIsDevMode) {}

	ref<gpu::srv> init(tests::pipeline_state* out_pipeline_state) override
	{
		*out_pipeline_state = tests::pipeline_state::pos_tex3d;

		surface::info_t si;
		si.semantic = surface::semantic::custom3d;
		si.format = surface::format::b8g8r8a8_unorm;
		si.dimensions = int3(64,64,64);
		surface::image image(si);
		{
			surface::lock_guard lock(image);
			surface::fill_image_t img;
			img.argb_surface = (uint32_t*)lock.mapped.data;
			img.row_width    = si.dimensions.x;
			img.num_rows     = si.dimensions.y;
			img.row_pitch    = lock.mapped.row_pitch;
			surface::fill_color_cube(&img, lock.mapped.depth_pitch, si.dimensions.z);
		}
		return dev_->new_texture("Test 3D", image);
	}
};

oGPU_COMMON_TEST(texture3d);
