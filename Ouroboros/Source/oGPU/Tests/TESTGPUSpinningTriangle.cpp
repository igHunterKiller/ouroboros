// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include "gpu_test_common.h"
#include <oMath/projection.h>
#include <oMath/matrix.h>

using namespace ouro;
using namespace ouro::gpu;

static const int sSnapshotFrames[] = { 0, 1, 3, 5 };
static const bool kIsDevMode = false;

struct oFIRST_VERTEX
{
	float3 position;
	uint32_t color;
};

class gpu_test_spinning_triangle : public tests::gpu_test
{
public:
	gpu_test_spinning_triangle() : gpu_test("GPU test: spinning triangle", kIsDevMode, sSnapshotFrames) {}

	void initialize() override
	{
		tri_.initialize(dev_);
	}

	void render() override
	{
		auto cl = dev_->immediate();
		cl->set_rtvs(1, &primary_target_, depth_dsv_);

		float4x4 V = lookat_lh(float3(0.0f, 0.0f, -2.5f), kZero3, kYAxis);

		float4x4 P = proj_fovy_lh(oDEFAULT_FOVY_RADIANSf, dev_->get_aspect_ratio(), 0.001f, 1000.0f);

		float rotationRate = get_frame() * 2.0f;
		float4x4 W = rotate(float3(0.0f, radians(rotationRate), 0.0f));

		test_constants cb(W, V, P, color::black);
		cl->set_cbv(oGPU_TEST_CB_CONSTANTS_SLOT, &cb, sizeof(cb));
		cl->set_pso(tests::pipeline_state::pos);
		
		tri_.draw(cl);
	}

private:
	tests::first_tri tri_;
};

oGPU_COMMON_TEST(spinning_triangle);
