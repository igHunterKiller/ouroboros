// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include "gpu_test_common.h"
#include <oMath/projection.h>
#include <oMath/matrix.h>

using namespace ouro;
using namespace ouro::gpu;

static const int sSnapshotFrames[] = { 0, 22, 45 };
static const bool kIsDevMode = false;

struct gpu_test_instanced_triangle : public tests::gpu_test
{
	gpu_test_instanced_triangle() : gpu_test("GPU test: instanced triangle", kIsDevMode, sSnapshotFrames) {}

	void initialize() override
	{
		tri_.initialize(dev_);
	}
	
	void render() override
	{
		auto cl = dev_->immediate();
		cl->set_rtvs(1, &primary_target_, depth_dsv_);

		const float4x4 V = lookat_lh(float3(0.0f, 0.0f, -3.5f), kZero3, kYAxis);

		float4x4 P = proj_fovy_lh(oDEFAULT_FOVY_RADIANSf, dev_->get_aspect_ratio(), 0.001f, 1000.0f);

		const float rotationStep = get_frame() * 1.0f;
		const float4x4 tx0 = rotate(float3(radians(rotationStep) * 0.75f, radians(rotationStep), radians(rotationStep) * 0.5f))
                       * translate(float3(-0.5f, 0.5f, 0.0f));

		const float4x4 tx1 = rotate(float3(radians(rotationStep) * 0.5f, radians(rotationStep), radians(rotationStep) * 0.75f))
                       * translate(float3(0.5f, -0.5f, 0.0f));

		test_constants instances[2];
		instances[0].set(tx0, V, P, color::black);
		instances[1].set(tx1, V, P, color::black);

		cl->set_cbv(oGPU_TEST_CB_CONSTANTS_SLOT, instances, sizeof(test_constants) * 2);
		cl->set_pso(tests::pipeline_state::pos);
		tri_.draw(cl, 2);
	}

private:
	tests::first_tri tri_;
};

oGPU_COMMON_TEST(instanced_triangle);
