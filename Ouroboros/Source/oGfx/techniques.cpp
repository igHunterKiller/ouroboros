// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// @tony: disabled after move to gpu.h
#if 0

#include "techniques.h"
#include <oGPU/all.h>
#include <oGfx/camera.h>
#include <oGfx/film.h>
#include <oGfx/draw_constants.h>
#include <oGfx/view_constants.h>

namespace ouro { namespace gfx {

ctx_technique ctx;

void init_context(const plan_task* tasks, uint32_t num_tasks)
{
	oASSERT(num_tasks == 1, "only one init task allowed");
	auto context = (ctx_technique*)tasks->data;
	
	// assign the context shared by all techniques
	ctx = *context;
}

void init_view_constants(const plan_task* tasks, uint32_t num_tasks)
{
	oASSERT(num_tasks == 1, "only one init view task allowed");

	auto cl = ctx.cl;
	auto core = ctx.core;
	auto camera = ctx.camera;
	auto film = camera->film();
	auto& cb = core->cb_view;

	auto vc = (view_constants*)cb.map(cl);
	oCHECK(vc, "init_view_constants: map failed");
	vc->set(camera->view_frustum()->view(), camera->view_frustum()->projection(), film->dimensions(), 0);
	cb.unmap(cl);
	cb.set(cl, oGFX_VIEW_CONSTANTS);
}

void clear_color_target(const plan_task* tasks, uint32_t num_tasks)
{
	for (uint32_t i = 0; i < num_tasks; i++)
	{
		auto clear = (ctx_clear_color_target*)tasks[i].data;
		clear->target->clear(ctx.cl, clear->clear_color, clear->array_index);
	}
}

void clear_depth_target(const plan_task* tasks, uint32_t num_tasks)
{
	for (uint32_t i = 0; i < num_tasks; i++)
	{
		auto clear = (ctx_clear_depth_target*)tasks[i].data;
		clear->target->clear(ctx.cl, clear->clear_type, clear->array_index, clear->depth, clear->stencil);
	}
}


void draw_model_forward(const plan_task* tasks, uint32_t num_tasks)
{
	auto cl = ctx.cl;
	auto core = ctx.core;
	auto camera = ctx.camera;
	auto film = camera->film();
	const auto& geo = core->models.geometry_buffer();

	auto ctarget = film->get_color(film::forward);
	auto dtarget = film->get_depth(film::hyper_depth);

	//ctarget->set_target(cl, dtarget);
	ctx.primary_target->set_target(cl, dtarget);

	uint32_t vertex_offsets[mesh::max_num_slots];
	uint32_t vertex_strides[mesh::max_num_slots];

	for (uint32_t i = 0; i < num_tasks; i++)
	{
		auto draw = (ctx_draw*)tasks[i].data;
		auto model = draw->model;
		auto info = model->info();
		auto tex = draw->texture;
	
		const bool wire = false;

		gpu::rasterizer_state::value lut[] = 
		{
			gpu::rasterizer_state::front_face,
			gpu::rasterizer_state::back_face,
			gpu::rasterizer_state::front_face,
			gpu::rasterizer_state::front_wireframe,
		};

		auto rs = lut[(int)info.face_type];

		auto prim = info.primitive_type;

		// set fixed state
		core->bs.set(cl, gpu::blend_state::opaque);
		core->dss.set(cl, gpu::depth_stencil_state::test_and_write);
		core->rs.set(cl, rs);
		core->ss.set(cl, gpu::sampler_state::linear_wrap, gpu::sampler_state::linear_wrap);

		// set pipeline state
		core->ls.set(cl, core->models.vertex_layout(), prim);
		core->vs.set(cl, core->models.vertex_shader());
		core->ps.set(cl, pixel_shader::constant_color);
	
		// set resources
		if (tex)
			tex->set(cl, 0);
		else
		{
			gpu::texture2d nil;
			nil.set(cl, 0);
		}
	
		// set constants
		{
			auto cb = core->cb_draw.find_best_fit(1);
			auto dc = (draw_constants*)cb->map(cl);
			oCHECK(dc, "draw_model_forward: map failed");
			
			dc->set_transform(draw->transform, camera->view_frustum()->view(), camera->view_frustum()->projection());
			dc->color = draw->color;
			dc->time = 0.0f;
			dc->vertex_scale = float(1 << info.log2scale);
			dc->object_id = 0;
			dc->draw_id = 0;
			dc->slice = 0;
			dc->vertex_offset = 0;
			dc->padA = 0;
			dc->padB = 0;

			cb->unmap(cl);
			cb->set(cl, oGFX_DRAW_CONSTANTS_SLOT);
		}

		uint32_t nslots = info.num_slots;

		auto index_offset = model->indices_offset();

		for (uint32_t slot = 0; slot < nslots; slot++)
		{
			vertex_offsets[slot] = model->vertices_offset(slot);
			vertex_strides[slot] = model->vertices_stride(slot);
		}

		geo.set(cl, index_offset, vertex_offsets, vertex_strides, nslots);
		geo.draw(cl, info.num_indices, 0, 0);
	}
}

void draw_lines(const plan_task* tasks, uint32_t num_tasks)
{
	auto cl = ctx.cl;
	auto core = ctx.core;
	auto camera = ctx.camera;
	auto film = camera->film();

	auto& verts = core->dynamic_lines;

	const uint32_t vertex_capacity = verts.num_vertices();

	// set target
	auto ctarget = film->get_color(film::forward);
	auto dtarget = film->get_depth(film::hyper_depth);

	//ctarget->set_target(cl, dtarget);
	ctx.primary_target->set_target(cl, dtarget);

	// set fixed state
	core->ls.set(cl, vertex_layout::pos_col, mesh::primitive_type::lines);
	core->bs.set(cl, gpu::blend_state::opaque);
	core->dss.set(cl, gpu::depth_stencil_state::test_and_write);
	core->rs.set(cl, gpu::rasterizer_state::front_face);

	core->vs.set(cl, vertex_shader::pos_col_prim);
	core->ps.set(cl, pixel_shader::vertex_color_prim);

	// set constants
	{
		auto cb = core->cb_draw.find_best_fit(1);
		auto dc = (draw_constants*)cb->map(cl);
		oCHECK(dc, "draw_lines: map failed");
			
		dc->set_transform(kIdentity4x4, camera->view_frustum()->view(), camera->view_frustum()->projection());
		dc->color = 0;
		dc->time = 0.0f;
		dc->vertex_scale = 0.0f;
		dc->object_id = 0;
		dc->draw_id = 0;
		dc->slice = 0;
		dc->vertex_offset = 0;
		dc->padA = 0;
		dc->padB = 0;

		cb->unmap(cl);
		cb->set(cl, oGFX_DRAW_CONSTANTS_SLOT);
	}

	for (uint32_t i = 0; i < num_tasks; i++)
	{
		auto lines = (const ctx_lines*)tasks[i].data;
		auto nverts = lines->num_vertices;

		if (nverts <= vertex_capacity)
		{
			verts.update(cl, 0, nverts, lines->line_vertices);
			verts.set(cl, 0);
			verts.draw_unindexed(cl, nverts); 
		}
	}
}

const technique_t gRenderTechniques[] =
{
	init_context,
	init_view_constants,
	clear_color_target,
	clear_depth_target,
	draw_model_forward,
	draw_lines,
};
match_array_e(gRenderTechniques, technique);

}}

#endif
