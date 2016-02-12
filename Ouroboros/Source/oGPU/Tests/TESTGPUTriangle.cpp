// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include "gpu_test_common.h"

using namespace ouro;
using namespace ouro::gpu;

static const int sSnapshotFrames[] = { 0 };
static const bool kIsDevMode = false;

struct gpu_test_triangle : public tests::gpu_test
{
	gpu_test_triangle() : gpu_test("GPU test: triangle", kIsDevMode, sSnapshotFrames) {}

	void initialize() override
	{
		tri_.initialize(dev_);
	}

	void render() override
	{
		auto cl = dev_->immediate();
		cl->set_pso(tests::pipeline_state::pos_untransformed);
		cl->set_rtvs(1, &primary_target_, depth_dsv_);
		tri_.draw(cl);
	}

private:
	tests::first_tri tri_;
};

oGPU_COMMON_TEST(triangle);
