// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include "gpu_test_common.h"
#include <oMath/projection.h>
#include <oMath/matrix.h>

using namespace ouro;
using namespace ouro::gpu;

static const int sSnapshotFrames[] = { 0, 1 };
static const bool kIsDevMode = false;

struct gpu_test_render_target : public tests::gpu_test
{
	gpu_test_render_target() : gpu_test("GPU test: render_target", kIsDevMode, sSnapshotFrames) {}

	void initialize() override
	{
		tri_.initialize(dev_);
		cube_.initialize(dev_);

		cl_main_ = dev_->new_graphics_command_list("cl_main_");
		cl_target_ = dev_->new_graphics_command_list("cl_target_");
		
		color_target_ = dev_->new_rtv("color_target_", 256, 256, surface::format::b8g8r8a8_unorm);
		color_resource_ = dev_->new_srv(color_target_->get_resource());
	}

	void render() override
	{
		// DrawOrder should be respected in out-of-order submits so show that here
		// by executing the main scene THEN the render target but because the
		// draw order of the command lists defines the render target before the 
		// main scene this should come out as a cube with a triangle texture.

		render_main_scene(cl_main_, color_resource_);
		render_to_target(cl_target_, color_target_);

		graphics_command_list* cls[2] = { cl_target_, cl_main_ };
		dev_->execute(2, cls);
	}

private:
	ref<graphics_command_list> cl_main_;
	ref<graphics_command_list> cl_target_;
	ref<rtv> color_target_;
	ref<srv> color_resource_;
	tests::test_cube cube_;
	tests::first_tri tri_;

	void render_to_target(graphics_command_list* cl, rtv* target)
	{
		cl->clear_rtv(target, color::deep_sky_blue);
		cl->set_rtvs(1, &target);
		cl->set_rso(0);
		cl->set_pso(tests::pipeline_state::pos_untransformed);
		tri_.draw(cl);
	}

	void render_main_scene(graphics_command_list* cl, srv* texture)
	{
		cl->set_rtv(primary_target_, depth_dsv_);
		
		float4x4 V = lookat_lh(float3(0.0f, 0.0f, -4.5f), kZero3, kYAxis);

		float4x4 P = proj_fovy_lh(oDEFAULT_FOVY_RADIANSf, dev_->get_aspect_ratio(), 0.001f, 1000.0f);

		static const float sCapture[] = 
		{
			774.0f,
			1036.0f,
		};

		uint frame = dev_->get_num_presents();
		float rotationStep = is_devmode() ? static_cast<float>(frame) : sCapture[get_frame()];

		float4x4 W = rotate(float3(radians(rotationStep) * 0.75f, radians(rotationStep), radians(rotationStep) * 0.5f));

		test_constants cb(W, V, P, color::black);
		cl->set_rso(0);
		cl->set_pso(tests::pipeline_state::pos_tex2d);
		cl->set_cbv(oGPU_TEST_CB_CONSTANTS_SLOT, &cb, sizeof(cb));
		cl->set_srvs(0, 1, &texture);

		cube_.draw(cl);
	}
};

oGPU_COMMON_TEST(render_target);
