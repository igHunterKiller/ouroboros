// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include "gpu_test_common.h"

using namespace ouro;
using namespace ouro::gpu;

static const int sSnapshotFrames[] = { 0 };
static const bool kIsDevMode = false;

struct oGPU_LINE_VERTEX
{
	float3 Position;
	uint32_t Color;
};

struct oGPU_LINE
{
	float3 Start;
	uint32_t StartColor;
	float3 End;
	uint32_t EndColor;
};

class gpu_test_lines : public tests::gpu_test
{
public:
	gpu_test_lines() : gpu_test("GPU test: lines", kIsDevMode, sSnapshotFrames) {}
	void initialize()
	{
		auto first_tri = primitive::first_tri_mesh(primitive::tessellation_type::solid);
		const float3* positions = (const float3*)first_tri.positions;

		oGPU_LINE s_lines[] = 
		{
			{ positions[0], color::red,   positions[1], color::green },
			{ positions[1], color::green, positions[2], color::blue },
			{ positions[2], color::blue,  positions[0], color::red },
		};

		vertices_ = dev_->new_vbv("lines", sizeof(oGPU_LINE_VERTEX), countof(s_lines) * 2, s_lines);
	}

	void render()
	{
		auto cl = dev_->immediate();
		cl->set_rtvs(1, &primary_target_, depth_dsv_);
		cl->set_pso(tests::pipeline_state::lines_untransformed);
		cl->set_vertices(0, 1, &vertices_);
		cl->draw(vertices_.num_vertices);
	}

private:
	vbv vertices_;
};

oGPU_COMMON_TEST(lines);
