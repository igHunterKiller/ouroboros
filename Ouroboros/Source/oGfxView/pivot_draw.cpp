// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include "pivot_draw.h"

#include <oGfx/draw_constants.h>
#include <oGPU/gpu.h>
#include <oMesh/primitive.h>
#include <oMesh/codec.h>

#include <oGfx/vertex_layouts.h>

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
	for (int y = 0, i = 0; y < 2; y++)
	{
		for (int x = 0; x < nhalf; x++, i++)
		{
			gfx::pivot_t piv(i, translate(float3(pos.x + x * 4.0f, pos.y - y * 4.0f, 0.0f)), float4(0.0f, 0.0f, 0.0f, 1.0f), float3(1.0f, 1.0f, 1.0f));
			pivots_[i] = scene_.new_pivot(piv);
		}
	}

	texture_ = load2d("test/textures/UVTest.png");
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

	for (size_t i = 0; i < npivots; i++)
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

		renderer.submit(0, gfx::render_pass::geometry, gfx::render_technique::draw_prim, prim);

#if 0
		cl->set_pso(gfx::pipeline_state::lines_color);
		model = renderer_.get_model_registry()->primitive(line_shapes[i]);
		renderer.get_model_registry()->set_model(cl, model);

		// draw a model
		{
			auto subsets = model->subsets();
			auto nsubsets = model->info().num_subsets;
			for (uint16_t s = 0; s < nsubsets; s++)
				cl->draw_indexed(subsets[s].num_indices);
		}
#endif
	}

	// draw grid
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
	oTrace("request_model_load: %s", uri_ref.c_str());

	model_ = registry.load(uri_ref, nullptr);
}
