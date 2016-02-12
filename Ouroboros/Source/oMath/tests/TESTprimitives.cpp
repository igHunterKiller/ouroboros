// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// NOTE: Dependencies are bad here because the test uses GPU to render it though
// oMath would seem like a simpler test. Figure this out later: priority is to 
// prevent regressions on primitives.

#include <oBase/unit_test.h>
#include <oMath/primitive.h>
#include <oMath/matrix.h>
#include <oMath/projection.h>

#include "../../oGPU/tests/gpu_test_common.h"

#include <oSystem/filesystem.h>

using namespace ouro;

class test_primitives : public tests::gpu_test
{
public:
	test_primitives(bool interactive)
		: gpu_test("oMath_primitives", interactive, s_snapshot_frames, countof(s_snapshot_frames), int2(640, 480))
	{}

	void initialize() override;
	void deinitialize() override;

	void render() override;

	float rotation_step();

protected:
	static const int s_snapshot_frames[2];
	static const int kNumPrimitives = 10;

	gpu::ibv ibvs_    [kNumPrimitives];
	gpu::vbv vbvs_    [kNumPrimitives];
	uint16_t nindices_[kNumPrimitives];
};

const int test_primitives::s_snapshot_frames[2] = { 0, 1 };

void test_primitives::initialize()
{
	static const char* names[] = { "cube", "circle", "cone", "cylinder", "torus", "tetrahedron", "hexahedron", "octahedron", "icosahedron", "dodecahedron" };
	static_assert(countof(names) == countof(ibvs_), "array mismatch");
	
	static const uint16_t kFacet  = 20;
	static const uint16_t kDivide = 20;
	static const auto     tes     = primitive::tessellation_type::textured;

	uint16_t max_verts = 0;
	
	// Get info from primitives
	primitive::info_t infos[kNumPrimitives];
	infos[0] = primitive::cube_info        (tes                 );
	infos[1] = primitive::circle_info      (tes, kFacet         );
	infos[2] = primitive::cone_info        (tes, kFacet         );
	infos[3] = primitive::cylinder_info    (tes, kFacet         );
	infos[4] = primitive::torus_info       (tes, kFacet, kDivide);
	infos[5] = primitive::tetrahedron_info (tes);
	infos[6] = primitive::hexahedron_info  (tes);
	infos[7] = primitive::octahedron_info  (tes);
	infos[8] = primitive::icosahedron_info (tes);
	infos[9] = primitive::dodecahedron_info(tes);

	// Allocate enough space for all primitives
	uint32_t bytes = 0;
	for (int i = 0; i < countof(infos); i++)
	{
		max_verts    = std::max(max_verts, infos[i].nvertices);
		nindices_[i] = infos[i].nindices;
		bytes       += infos[i].total_bytes();
	}

	auto mem = default_allocator.scoped_allocate(bytes, "test_primitives");

	// Set up memory topology by patching mesh pointers into working buffer
	primitive::mesh_t meshes[kNumPrimitives];
	meshes[0] = primitive::mesh_t(infos[0], mem);
	for (int i = 1; i < countof(infos); i++)
	{
		void* new_base = meshes[i-1].indices + infos[i-1].nindices;
		meshes[i]      = primitive::mesh_t(infos[i], new_base);
	}

	// Tessellate the primitives
	primitive::cube_tessellate        (&meshes[0], tes);
	primitive::circle_tessellate      (&meshes[1], tes, kFacet);
	primitive::cone_tessellate        (&meshes[2], tes, kFacet);
	primitive::cylinder_tessellate    (&meshes[3], tes, kFacet, 1.0f);
	primitive::torus_tessellate       (&meshes[4], tes, kFacet, kDivide, 0.5f);
	primitive::tetrahedron_tessellate (&meshes[5], tes);
	primitive::hexahedron_tessellate  (&meshes[6], tes);
	primitive::octahedron_tessellate  (&meshes[7], tes);
	primitive::icosahedron_tessellate (&meshes[8], tes);
	primitive::dodecahedron_tessellate(&meshes[9], tes);

	struct vtx
	{
		float3 pos;
		float3 tex;
	};

	// Interleave vertex data and move its all into gpu memory
	auto vtx_mem = default_allocator.scoped_allocate(sizeof(vtx) * max_verts, "test_primitives vtx_mem");

	for (int i = 0; i < countof(ibvs_); i++)
	{
		const float3* positions = (const float3*)meshes[i].positions;
		const float2* texcoords = (const float2*)meshes[i].texcoords;

		vtx* v = (vtx*)vtx_mem;
		for (int j = 0; j < infos[i].nvertices; j++)
		{
			v[j].pos = positions[j];
			v[j].tex = texcoords ? float3(texcoords[j], 0.0f) : float3(0.0f, 0.0f, 0.0f);
		}

		ibvs_[i] = dev_->new_ibv(names[i], infos[i].nindices, meshes[i].indices);
		vbvs_[i] = dev_->new_vbv(names[i], sizeof(vtx), infos[i].nvertices, v);
	}
}

void test_primitives::deinitialize()
{
	for (int i = 0; i < countof(ibvs_); i++)
	{
		dev_->del_vbv(vbvs_[i]);
		dev_->del_ibv(ibvs_[i]);
	}
}

float test_primitives::rotation_step()
{
	static const float sCapture[] = 
	{
		613.0f,
		1277.0f,
	};

	return is_devmode() ? static_cast<float>(dev_->get_num_presents()) : sCapture[get_frame()];
}

void test_primitives::render()
{
	auto state = tests::pipeline_state::pos_texcoords_as_color;

	auto* cl = dev_->immediate();
	cl->set_rtvs(1, &primary_target_, depth_dsv_);
	
	float4x4 V = lookat_lh(float3(0.0f, 0.0f, -4.5f), kZero3, kYAxis);
	float4x4 P = proj_fovy_lh(oDEFAULT_FOVY_RADIANSf, dev_->get_aspect_ratio(), 0.001f, 1000.0f);

	cl->set_pso(state);

	float4x4 base = scale(0.3f) * rotate(float3(0.0f, radians(180.0f), 0.0f)); 

	for (int i = 0; i < countof(ibvs_); i++)
	{
		float rotationStep = rotation_step();
		float4x4 W = base * rotate(float3(radians(rotationStep) * 0.75f, radians(rotationStep), radians(rotationStep) * 0.5f));
		W          = W    * translate(float3( -1.5f + (i%4), -1.0f + (i/4), 0.0f));

		test_constants cb(W, V, P, color::black);
		cl->set_cbv(oGPU_TEST_CB_CONSTANTS_SLOT, &cb, sizeof(cb));

		cl->set_indices(ibvs_[i]);
		cl->set_vertices(0, 1, &vbvs_[i]);
		cl->draw_indexed(nindices_[i]);
	}
}

oTEST(oMath_primitives)
{
	static const bool interactive = false;
	test_primitives t(interactive);
	t.run(srv);
}
