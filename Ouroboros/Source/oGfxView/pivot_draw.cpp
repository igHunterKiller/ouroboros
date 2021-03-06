// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include "pivot_draw.h"

#include <oGfx/draw_constants.h>
#include <oGPU/gpu.h>
#include <oMesh/primitive.h>
#include <oMesh/codec.h>
#include <oCore/color.h>


using namespace ouro;

extern ouro::gfx::render_settings_t g_render_settings;

pivot_draw::pivot_draw()
{
}

pivot_draw::~pivot_draw()
{
}

void pivot_draw::initialize()
{
	pivot_draw::on_view_default();

	// ------------------------------------
	// App-specific init

	pivots_.fill(nullptr);

	int n = (int)gfx::primitive_model::count / 2;

	float3 pos(-6.0f, 1.0f, 0.0f);

	int nhalf = n / 2;
	int npivots = 0;
	for (int y = 0, i = 0; y < 2; y++)
	{
		for (int x = 0; x < nhalf; x++, i++)
		{
			gfx::pivot_t piv(i, translate(float3(pos.x + x * 4.0f, pos.y - y * 4.0f, 0.0f)), float4(0.0f, 0.0f, 0.0f, 1.0f), float3(1.0f, 1.0f, 1.0f));
			pivots_[i] = scene_.new_pivot(piv);
			npivots++;
		}
	}

	gfx::pivot_t piv(npivots, translate(float3(0.0f, 0.0f, 0.0f)), float4(0.0f, 0.0f, 0.0f, 1.0f), float3(1.0f, 1.0f, 1.0f));
	pivots_[npivots++] = scene_.new_pivot(piv);


	texture_ = load2d("test/textures/UVTest.png");
	model_   = load_mesh("test/geometry/cube.obj");
}

void pivot_draw::deinitialize()
{
	texture_ = gfx::texture2d_t();
	model_   = gfx::model_t();
}

void pivot_draw::on_view_default()
{
	//float4 scene_sphere = scene_.calc_bounding_sphere();

	focus(float3(0.0f, 0.0f, -22.0f), kZero3);
}

void pivot_draw::on_resized(uint32_t new_width, uint32_t new_height)
{
}

void pivot_draw::update(float delta_time)
{
}

void pivot_draw::submit_scene(gfx::renderer_t& renderer)
{
	// extract all pivots from scene (frustcull precursor)

	gfx::pivot_t* pivots[128];
	size_t npivots = scene_.select(pivots);

	float4 colors[] = 
	{
		float4(1.0f, 0.0f, 0.0f, 0.5f),
		float4(0.0f, 1.0f, 0.0f, 0.5f),
	};

	uint32_t colors2[] = 
	{
		color::red,
		color::lime,
	};

	gfx::primitive_model shapes[] = 
	{
		gfx::primitive_model::circle,
		gfx::primitive_model::pyramid,
		gfx::primitive_model::box,
		gfx::primitive_model::cone,
		gfx::primitive_model::cylinder,
		gfx::primitive_model::sphere,
		gfx::primitive_model::rectangle,
		gfx::primitive_model::torus,
	};

	gfx::primitive_model line_shapes[] = 
	{
		gfx::primitive_model::circle_outline,
		gfx::primitive_model::pyramid_outline,
		gfx::primitive_model::box_outline,
		gfx::primitive_model::cone_outline,
		gfx::primitive_model::cylinder_outline,
		gfx::primitive_model::sphere_outline,
		gfx::primitive_model::rectangle_outline,
		gfx::primitive_model::torus_outline,
	};

	for (size_t i = 0; i < (npivots-1); i++)
	{
		float3x3 orientation;
		float3 position, extents;
		pivots[i]->obb(&orientation, &position, &extents);

		float4x4 tx;
		tx[0] = float4(orientation[0] * extents.x, 0.0f);
		tx[1] = float4(orientation[1] * extents.y, 0.0f);
		tx[2] = float4(orientation[2] * extents.z, 0.0f);
		tx[3] = float4(position, 1.0f);

		auto* prim = renderer.allocate<gfx::primitive_submission_t>();
		prim->world = tx;
		prim->color = colors[i % countof(colors)];
		prim->type  = shapes[i];
		prim->texture = texture_.get()->view;

		//renderer.submit(0, gfx::render_pass::geometry, gfx::render_technique::draw_prim, prim);
	}

	// draw hacked-in model
	if (model_)
	{
		const auto model = model_.get();

		const auto&    model_pivot  = *pivots[npivots - 1];
		const auto*    subset       = model->subsets();
		const auto&    minfo        = model->info();
		auto           num_vertices = minfo.num_vertices;
		const uint32_t num_subsets  = minfo.num_subsets;
		const auto     subsets_end  = subset + num_subsets;
		auto           submit       = renderer.allocate<gfx::model_subset_submission_t>(num_subsets);
		auto           world        = model_pivot.world();
		auto           idx_offset   = model->indices_offset();
		auto           vbv0         = gpu::vbv(model->vertices_offset(0), num_vertices, model->vertex_stride(0));
		auto           vbv1         = gpu::vbv(model->vertices_offset(1), num_vertices, model->vertex_stride(1));
		auto           vbv2         = gpu::vbv(model->vertices_offset(2), num_vertices, model->vertex_stride(2));

		// try to keep very diverse obj files to the same relative scale
		world = scale(3.0f / minfo.bounding_sphere.w) * world;

		while (subset < subsets_end)
		{
			auto num_indices      = subset->num_indices;
			submit->world         = world;
			submit->mat_hash      = 0;
			submit->state         = gfx::pipeline_state::mesh_uv0_as_color;
			submit->num_srvs      = 0;
			submit->num_constants = 0;
			submit->pada          = 0;
			auto& geo             = submit->geometry;
			geo.start_index       = subset->start_index;
			geo.num_indices       = num_indices;
			geo.indices           = gpu::ibv(idx_offset, num_indices);
			geo.vertices[0]       = vbv0;
			//geo.vertices[1]     = vbv1;
			//geo.vertices[2]     = vbv2;

			renderer.submit(0, gfx::render_pass::geometry, gfx::render_technique::draw_subset, submit);

			subset++;
			submit++;
		}

		// Draw its normals
		if (0)
		{
			auto sub = renderer.allocate<gfx::debug_model_normals_submission_t>();
		
			sub->world        = world;
			sub->vertices     = vbv0;
			sub->start_vertex = 0;
			sub->num_vertices = num_vertices;
			sub->argb         = color::cornflower_blue;
			sub->scale        = 1.0f;

			renderer.submit(0, gfx::render_pass::geometry, gfx::render_technique::debug_draw_normals, sub);
		}

		// Draw its tangents
		if (0)
		{
			auto sub = renderer.allocate<gfx::debug_model_tangents_submission_t>();
		
			sub->world        = world;
			sub->vertices     = vbv0;
			sub->start_vertex = 0;
			sub->num_vertices = num_vertices;
			sub->pos_argb     = color::lime;
			sub->neg_argb     = color::green;
			sub->scale        = 1.0f;

			renderer.submit(0, gfx::render_pass::geometry, gfx::render_technique::debug_draw_tangents, sub);
		}
	}

	// draw grid
	if (0)
	{
		auto grid = renderer.allocate<gfx::grid_submission_t>();
		grid->total_width = 100.0f;
		grid->grid_width  = 1.0f;
		grid->argb        = color::slate_gray;

		renderer.submit(0, gfx::render_pass::debug, gfx::render_technique::draw_grid, grid);
	}
}

void pivot_draw::request_model_load(gfx::model_registry& registry, const uri_t& uri_ref)
{
	oTrace("[pivot_draw] load model: %s", uri_ref.c_str());
	model_ = registry.load(uri_ref, nullptr);
}
