// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include "gpu_test_common.h"

using namespace ouro;
using namespace ouro::gpu;

static const int sSnapshotFrames[] = { 0, 1 };
static const bool kIsDevMode = false;

class gpu_test_clear : public tests::gpu_test
{
public:
	gpu_test_clear() : gpu_test("GPU test: clear", kIsDevMode, sSnapshotFrames) {}

	void render() override
	{
		static uint32_t sClearColors[] = { color::lime, color::white };
		static int s_frame_id = 0;
		dev_->immediate()->clear_rtv(dev_->get_presentation_rtv(), sClearColors[s_frame_id++ % countof(sClearColors)]);
	}
};

oGPU_COMMON_TEST(clear);
