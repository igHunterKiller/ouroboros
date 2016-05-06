// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oGfx/renderer.h>
#include <oGfx/gpu_signature.h>

#include <oMath/btt.h>
#include <oMath/gizmo.h>
#include <oMath/primitive.h>

#include <oGUI/window.h>
#include <oCore/countof.h>
#include <oSystem/filesystem.h>

// About submission:
// I need threads to communicate back what their allocs were so all can be processed.
// I need a master list alloc, which is big-ish... but really ~1MB for 50,000 instructions.
// 2MB -> 128k tasks, that should be good.
// 2MB of alloc data seems appropriate. It seems 2MB covers all threads for tasklist. Alloc 4 and call it a day?

namespace ouro {
	
template<> const char* as_string(const gfx::fullscreen_mode& mode)
{
	static const char* s_names[] = 
	{
		"lit",
		"points",
		"wireframe",
		"texcoord",
		"texcoordu",
		"texcoordv",
		"bittangentx",
		"bittangenty",
		"bittangentz",
		"bittangent",
		"tangentx",
		"tangenty",
		"tangentz",
		"tangent",
		"normalx",
		"normaly",
		"normalz",
		"normal",
	};
	return as_string(mode, s_names);
}

template<> const char* as_string(const gfx::render_pass& pass)
{
	const char* s_names[] = 
	{
		"initialize",
		"geometry",
		"shadow",
		"depth_resolve",
		"tonemap",
		"debug",
		"resolve",
	};
	return as_string(pass, s_names);
}

template<> const char* as_string(const gfx::render_technique& technique)
{
	const char* s_names[] = 
	{
		"pass_begin",
		"view_begin",
		"linearize_depth",
		"draw_lines",
		"draw_prim",
		"draw_subset",
		"draw_axis",
		"draw_gizmo",
		"draw_grid",
		"debug_draw_normals",
		"debug_draw_tangents",
		"view_end",
		"pass_end",
	};
	return as_string(technique, s_names);
}

	namespace gfx {

oTHREAD_LOCAL uint32_t renderer_t::local_tasklist_frame_id_;
oTHREAD_LOCAL void*    renderer_t::local_tasklist_;
oTHREAD_LOCAL uint32_t renderer_t::local_heap_frame_id_;
oTHREAD_LOCAL void*    renderer_t::local_heap_;
oTHREAD_LOCAL void*    renderer_t::local_heap_end_;

static const uint16_t pass_shift      = 56;
static const uint16_t technique_shift = 48;
static const uint64_t pass_mask       = 0xff00000000000000;
static const uint64_t technique_mask  = 0x00ff000000000000;
static const uint64_t priority_mask   = ~(pass_mask|technique_mask);

struct task_t
{
	uint64_t key;
	const void* data;
};

struct technique_context_t
{
	gfx::model_registry* models;
	gfx::texture2d_registry* texture2ds;
	gpu::graphics_command_list* gcl;
	gpu::rtv* presentation_target;
	const gfx::film_t* film;
	const pov_t* pov;
	const task_t* tasks;
	uint32_t num_tasks;
	render_pass pass;
	render_technique technique;

	render_settings_t render_settings;

	bool is_beyond(const render_pass& p) const { return ((tasks[0].key >> pass_shift) & 0xff) > (uint64_t)p; }
};

#define ForEachTask for (const task_t* task = ctx.tasks, *end = ctx.tasks + ctx.num_tasks; task < end; task++)

typedef flexible_array_t<task_t> tasklist_t;

typedef void (*technique_t)(technique_context_t& ctx);

void view_begin(technique_context_t& ctx)
{
	auto cl = ctx.gcl;
	auto film = ctx.film;
	auto pov = ctx.pov;

	// set up GPU topology
	cl->set_rso(gfx::signature::graphics);

	// clear and set render targets
	{
		cl->clear_rtv(ctx.presentation_target, color::almost_black);

		auto hyper_depth = film->dsv(gfx::film_t::hyper_depth);
		cl->clear_dsv(hyper_depth);

		cl->set_rtv(ctx.presentation_target, hyper_depth);
	}

	// configure per-view settings
	{
		auto desc = ctx.presentation_target->get_resource()->get_desc();
		gfx::view_constants vc(pov->view(), pov->projection(), desc.width, desc.height, 0);
		cl->set_cbv(oGFX_CBV_VIEW, &vc, sizeof(vc));
	}
}

void view_end(technique_context_t& ctx)
{
	oAssert(ctx.num_tasks == 1, "");
}

void pass_begin(technique_context_t& ctx)
{
	oAssert(ctx.num_tasks == 1, "");
}

void pass_end(technique_context_t& ctx)
{
	oAssert(ctx.num_tasks == 1, "");
}

void linearize_depth(technique_context_t& ctx)
{
	oAssert(ctx.num_tasks == 1, "Call this only once");
	oAssert(ctx.is_beyond(render_pass::geometry), "Requires valid hyper depth buffer");

	auto& cl          = *ctx.gcl;
	auto& film        = *ctx.film;
	auto linear_depth = film.rtv(film_t::linear_depth);
	auto hyper_depth  = film.srv(film_t::hyper_depth);

	cl.set_pso(pipeline_state::linearize_depth);
	cl.set_vertices(0, 1, nullptr);
	cl.set_rtv(linear_depth);
	cl.set_srvs(oGFX_SRV_DEPTH, 1, &hyper_depth);
	cl.draw(3);
	cl.set_srvs(oGFX_SRV_DEPTH, 1, nullptr);
	//cl.set_rtv(nullptr);

	// This is crappy... restore prior target that I happen to know is this... and that other handlers assume too much (though they ARE draw's, targets should be set elsewhere)
	cl.set_rtv(ctx.presentation_target, film.dsv(film_t::hyper_depth));
}

static pipeline_state get_fullscreen_pipline_state(technique_context_t& ctx, const pipeline_state& default_pso)
{
	pipeline_state pso;
	switch (ctx.render_settings.mode)
	{
		default:
		case fullscreen_mode::lit:         pso = default_pso;                              break;
		case fullscreen_mode::points:      pso = pipeline_state::pos_only_points;          break;
		case fullscreen_mode::wireframe:   pso = pipeline_state::mesh_wire;                break;
		case fullscreen_mode::texcoord:    pso = pipeline_state::mesh_uv0_as_color;        break;
		case fullscreen_mode::texcoordu:   pso = pipeline_state::mesh_u0_as_color;         break;
		case fullscreen_mode::texcoordv:   pso = pipeline_state::mesh_v0_as_color;         break;
		case fullscreen_mode::bittangentx: pso = pipeline_state::mesh_bitangentx_as_color; break;
		case fullscreen_mode::bittangenty: pso = pipeline_state::mesh_bitangenty_as_color; break;
		case fullscreen_mode::bittangentz: pso = pipeline_state::mesh_bitangentz_as_color; break;
		case fullscreen_mode::bittangent:	 pso = pipeline_state::mesh_bitangent_as_color;  break;
		case fullscreen_mode::tangentx:		 pso = pipeline_state::mesh_tangentx_as_color;   break;
		case fullscreen_mode::tangenty:		 pso = pipeline_state::mesh_tangenty_as_color;   break;
		case fullscreen_mode::tangentz:		 pso = pipeline_state::mesh_tangentz_as_color;   break;
		case fullscreen_mode::tangent:		 pso = pipeline_state::mesh_tangent_as_color;    break;
		case fullscreen_mode::normalx:		 pso = pipeline_state::mesh_normalx_as_color;    break;
		case fullscreen_mode::normaly:		 pso = pipeline_state::mesh_normaly_as_color;    break;
		case fullscreen_mode::normalz:		 pso = pipeline_state::mesh_normalz_as_color;    break;
		case fullscreen_mode::normal:			 pso = pipeline_state::mesh_normal_as_color;     break;
	}

	return pso;
}

static void set_model(gpu::graphics_command_list* cl, const mesh::model* mdl)
{
	auto& info = mdl->info();
	
	// Reconstruct ibv and bind it
	{
		gpu::ibv indices;
		indices.offset = mdl->indices_offset();
		indices.num_indices = info.num_indices;
		indices.transient = 0;
		indices.is_32bit = false;
		
		cl->set_indices(indices);
	}

	// Reconstruct vbvs and bind them
	auto nslots = info.num_slots;
	gpu::vbv vbvs[mesh::max_num_slots];
	for (uint32_t slot = 0; slot < nslots; slot++)
	{
		auto stride = mdl->vertex_stride(slot);

		auto& verts = vbvs[slot];
		verts.offset = mdl->vertices_offset(slot);
		verts.num_vertices = info.num_vertices;
		verts.transient = 0;
		verts.vertex_stride_bytes(mdl->vertex_stride(slot));
	}

	cl->set_vertices(0, nslots, vbvs);
}

void draw_prim(technique_context_t& ctx)
{
	auto& cl     = *ctx.gcl;
	auto& models = *ctx.models;
	auto& pov    = *ctx.pov;
	auto pso     = get_fullscreen_pipline_state(ctx, pipeline_state::mesh_simple_texture);

	cl.set_pso(pso);

	ForEachTask
	{
		auto& prim = *(const primitive_submission_t*)task->data;

		// set up material parameters
		cl.set_srvs(0, 1, &prim.texture);

		// bind a model
		auto model = models.primitive(prim.type);
		set_model(&cl, model);

		// set up draw constants
		{
			gfx::draw_constants draw;
			draw.set_transform(prim.world, pov.view(), pov.projection());
			draw.color = prim.color;
			draw.vertex_scale = 1.0f;
			draw.vertex_offset = ~0u;
			draw.object_id = 0;
			draw.draw_id = 0;
			draw.time = 0.0f;
			draw.slice = 0;
			draw.flags = 0;
			draw.pada = 0;
			cl.set_cbv(oGFX_CBV_DRAW, &draw, sizeof(draw));
		}

		// draw the model
		{
			auto subsets = model->subsets();
			uint32_t nsubsets = model->info().num_subsets;
			for (uint32_t s = 0; s < nsubsets; s++)
				cl.draw_indexed(subsets[s].num_indices);
		}
	}
}

static void draw_geometry(gpu::graphics_command_list& cl, const geometry_view_t& geometry, const gfx::draw_constants* draw_constants, uint32_t num_instances)
{
	if (!num_instances)
		return;
	cl.set_cbv(oGFX_CBV_DRAW, draw_constants, num_instances * sizeof(gfx::draw_constants));
	cl.set_indices(geometry.indices);
	cl.set_vertices(0, countof(geometry.vertices), geometry.vertices);
	cl.draw_indexed(geometry.num_indices, num_instances, geometry.start_index);
}

void draw_subset(technique_context_t& ctx)
{
	auto&       cl         = *ctx.gcl;
	auto&       models     = *ctx.models;
	auto&       pov        = *ctx.pov;
	const float time       = 0.0f;
	const auto& view       = pov.view();
	const auto& projection = pov.projection();
	const auto  view_proj  = view * projection;

	// iterator/shared state: subsets are sorted by pso, then by material
	auto* prev_subset      = (const model_subset_submission_t*)ctx.tasks->data;
	auto  prev_pso         = get_fullscreen_pipline_state(ctx, prev_subset->state);
	auto  prev_mat         = prev_subset->mat_hash;
	auto* subset           = prev_subset;

	gfx::draw_constants draws[64];
	uint32_t num_instances = 0;

	cl.set_pso(prev_pso);
	// set prev materials
	
	ForEachTask
	{
		     subset = (const model_subset_submission_t*)task->data;
		auto pso    = get_fullscreen_pipline_state(ctx, subset->state);
	
		cl.set_pso(pso);
		// set cbs
		{
			auto& draw         = draws[num_instances];
			draw.set_transform(subset->world, view_proj);
			draw.color         = 0;
			draw.vertex_scale  = 1.0f;
			draw.vertex_offset = ~0u;
			draw.object_id     = 0;
			draw.draw_id       = 0;
			draw.time          = time;
			draw.slice         = 0;
			draw.flags         = 0;
			draw.pada          = 0;
		}

		draw_geometry(cl, subset->geometry, draws, 1);


#if 0

		auto mat    = subset->mat_hash;

		// HACK until a material system is installed
		if (num_instances || num_instances >= countof(draws) || pso != prev_pso || mat != prev_mat)
		{
			draw_subset(cl, *subset, draws, num_instances);
			num_instances = 0;
		}

		// context rolling major state
		if (pso != prev_pso)
			cl.set_pso(pso);

		// user pointers/non-context rolling
		if (mat != prev_mat)
		{
			// set material cbs
			// set material srvs
		}

		// set cbs
		{
			auto& draw         = draws[num_instances];
			draw.set_transform(subset->world, view_proj);
			draw.color         = 0;
			draw.vertex_scale  = 1.0f;
			draw.vertex_offset = ~0u;
			draw.object_id     = 0;
			draw.draw_id       = 0;
			draw.time          = time;
			draw.slice         = 0;
			draw.flags         = 0;
			draw.pada          = 0;
		}

		// increment instance
		num_instances++;
		prev_pso = pso;
		prev_mat = mat;

#endif
	}

#if 0
	// one last flush
	if (num_instances)
		draw_subset(cl, *subset, draws, num_instances);

#endif
}

void draw_axis(technique_context_t& ctx)
{
	oAssert(ctx.num_tasks == 1, "");

	auto& cl    = *ctx.gcl;
	auto& pov   = *ctx.pov;
	auto& film  = *ctx.film;
	
	static const uint32_t kColors[]   = { color::red, color::lime, color::blue };
	static const uint16_t kConeFacet  = 10;
	static const float    kPixelInset = 28.0f;
	const float2          vp_dim      = pov.viewport().dimensions;
	const float           kScaleRatio = 1.0f / max(vp_dim);
	const float           kLineScale  = 16.0f * kScaleRatio;
	const float3          kConeScale  = 1.7f * kScaleRatio;
	const float3          center      = pov.unproject(float2(vp_dim.x - kPixelInset, kPixelInset), pov.near_dist() + 1.1f);

	static const float3 kCapRotation[3] = 
	{
		float3(0.0f,         oPIf * 0.5f, 0.0f),
		float3(-oPIf * 0.5f,        0.0f, 0.0f),
		float3(0.0f,                0.0f, 0.0f),
	};

	auto cone       = primitive::cone_info(primitive::tessellation_type::solid, kConeFacet);
	auto mem        = alloca(cone.total_bytes());
	uint32_t nverts = cone.nindices;

	primitive::mesh_t cone_mesh(cone, mem);
	primitive::cone_tessellate(&cone_mesh, cone.type, kConeFacet);

	auto verts = cl.new_transient_vertices<gfx::VTXpc>(6 + 3 * nverts);
	if (!verts)
		return;

	verts[0].position = center;                                  verts[0].color = kColors[0];
	verts[1].position = center + float3(kLineScale, 0.0f, 0.0f); verts[1].color = kColors[0];
	verts[2].position = center;                                  verts[2].color = kColors[1];
	verts[3].position = center + float3(0.0f, kLineScale, 0.0f); verts[3].color = kColors[1];
	verts[4].position = center;                                  verts[4].color = kColors[2];
	verts[5].position = center + float3(0.0f, 0.0f, kLineScale); verts[5].color = kColors[2];

	auto scale_tx = scale(kConeScale);

	auto v = verts + 6;
	for (uint32_t icone = 0; icone < 3; icone++)
	{
		const float3* indexed_verts = (float3*)cone_mesh.positions;
		const uint16_t* indices     = cone_mesh.indices;

		auto tx = scale_tx * rotate(kCapRotation[icone]);

		tx[3].xyz() = verts[icone * 2 + 1].position;

		for (uint32_t i = 0; i < cone.nindices; i += 3)
		{
			v->position = mul(tx, indexed_verts[indices[i+0]]); v->color = kColors[icone]; v++;
			v->position = mul(tx, indexed_verts[indices[i+1]]); v->color = kColors[icone]; v++;
			v->position = mul(tx, indexed_verts[indices[i+2]]); v->color = kColors[icone]; v++;
		}
	}

	auto lines_view = cl.commit_transient_vertices();
	auto faces_view = lines_view;
	faces_view.num_vertices -= 6;
	faces_view.offset       += 6 * sizeof(VTXpc);

	draw_constants draw(kIdentity4x4, pov.view(), pov.projection());
	cl.set_cbv(oGFX_CBV_DRAW, &draw, sizeof(draw));
	cl.set_pso(pipeline_state::lines_vertex_color);
	cl.set_vertices(0, 1, &lines_view);
	cl.draw(6);

	//gfx::draw_constants draw(kIdentity4x4, pov_.view(), pov_.projection());
	//cl->set_cbv(oGFX_CBV_DRAW, &draw, sizeof(draw));
	cl.set_pso(pipeline_state::pos_vertex_color);
	cl.set_vertices(0, 1, &faces_view);
	cl.draw(faces_view.num_vertices);
}

void draw_lines(technique_context_t& ctx)
{
	auto& cl  = *ctx.gcl;
	auto& pov = *ctx.pov;

	cl.set_pso(gfx::pipeline_state::lines_vertex_color);

	gfx::draw_constants draw(kIdentity4x4, pov.view(), pov.projection());
	cl.set_cbv(oGFX_CBV_DRAW, &draw, sizeof(draw));

	ForEachTask
	{
		auto lines_sub = (lines_submission_t*)task->data;
		auto lines     = lines_sub->lines;
		auto nlines    = lines_sub->nlines;
		auto nverts    = nlines * 2;
		auto lines_end = lines + nlines;

		auto verts = cl.new_transient_vertices<VTXpc>(nverts);

		if (verts)
		{
			while (lines < lines_end)
			{
				verts->position = lines->p0;
				verts->color    = lines->argb;
				verts++;
				verts->position = lines->p1;
				verts->color    = lines->argb;
				verts++;
				lines++;
			}

			auto view = cl.commit_transient_vertices();
		
			cl.set_vertices(0, 1, &view);
			cl.draw(nverts);
		}

		else
		{
			oTrace("draw_lines out of transient vertices");
		}
	}
}

void draw_gizmo(technique_context_t& ctx)
{
	oAssert(ctx.num_tasks == 1, "");

	auto& cl          = *ctx.gcl;
	auto& pov         = *ctx.pov;
	auto& film        = *ctx.film;
	auto& tess        = *(const gizmo::tessellation_info_t*)ctx.tasks->data;
	auto linear_depth = film.srv(film_t::linear_depth);

	auto nlverts = tess.num_line_vertices;
	auto nfverts = tess.num_triangle_vertices;

	if (!nlverts && !nfverts)
		return;

	auto line_verts = cl.new_transient_vertices<gizmo::vertex_t>(nlverts + nfverts);
	if (line_verts)
	{
		auto tri_verts = line_verts + nlverts;
		gizmo::tessellate(tess, line_verts, tri_verts);
		gpu::vbv view = cl.commit_transient_vertices();

		// verts are in world space, so set up just the view & projection
		gfx::draw_constants draw(kIdentity4x4, pov.view(), pov.projection());
		cl.set_cbv(oGFX_CBV_DRAW, &draw, sizeof(draw));

		bool linear_depth_set = false;

		// draw lines
		if (nlverts)
		{
			// Draw lines with vertex colors
			cl.set_pso(gfx::pipeline_state::lines_vertex_color_stipple);
			cl.set_srvs(oGFX_SRV_DEPTH, 1, &linear_depth);
			linear_depth_set = true;
			cl.set_vertices(0, 1, &view);
			cl.draw(nlverts);
		}

		// draw tris
		if (nfverts)
		{
			// triangle vertices are at end of line vertices view
			auto tri_view = view;
			tri_view.offset += nlverts * view.vertex_stride_bytes();
			tri_view.num_vertices = nfverts;

			cl.set_pso(gfx::pipeline_state::pos_vertex_color_stipple);
			
			if (!linear_depth_set)
				cl.set_srvs(oGFX_SRV_DEPTH, 1, &linear_depth);

			cl.set_vertices(0, 1, &tri_view);
			cl.draw(nfverts);
		}

		if (linear_depth_set)
			cl.set_srvs(oGFX_SRV_DEPTH, 1, nullptr);
	}
}

void draw_grid(technique_context_t& ctx)
{
	auto& cl  = *ctx.gcl;
	auto& pov = *ctx.pov;

	cl.set_pso(gfx::pipeline_state::lines_vertex_color);

	gfx::draw_constants draw(kIdentity4x4, pov.view(), pov.projection());
	cl.set_cbv(oGFX_CBV_DRAW, &draw, sizeof(draw));

	ForEachTask
	{
		auto sub        = (grid_submission_t*)task->data;
		auto width      = sub->total_width;
		auto step       = sub->grid_width;
		auto argb       = sub->argb;
		auto half_width = width * 0.5f;
		auto nwidth     = uint32_t(width  / step) + 1;
		auto nlines     = nwidth * 2;
		auto nverts     = nlines * 2;

		auto v = cl.new_transient_vertices<VTXpc>(nverts);

		if (v)
		{
			for (float z = -half_width; z <= half_width; z += step)
			{
				v->position = float3(-half_width, 0.0f, z);
				v->color    = argb;
				v++;
				v->position = float3(+half_width, 0.0f, z);
				v->color    = argb;
				v++;
			}

			for (float x = -half_width; x <= half_width; x += step)
			{
				v->position = float3(x, 0.0f, -half_width);
				v->color    = argb;
				v++;
				v->position = float3(x, 0.0f, +half_width);
				v->color    = argb;
				v++;
			}

			auto view = cl.commit_transient_vertices();
		
			cl.set_vertices(0, 1, &view);
			cl.draw(nverts);
		}

		else
		{
			oTrace("draw_grid out of transient vertices");
		}
	}
}

void debug_draw_normals(technique_context_t& ctx)
{
	auto& cl  = *ctx.gcl;
	auto& pov = *ctx.pov;
	auto& dev = *cl.device();

	cl.set_pso(gfx::pipeline_state::lines_vertex_color);

	ForEachTask
	{
		auto  sub          = (debug_model_normals_submission_t*)task->data;
		auto& world        = sub->world;
		auto  model_vbv    = sub->vertices;
		auto  start_vertex = sub->start_vertex;
		auto  argb         = sub->argb;
		auto  scale        = sub->scale;
		auto  model_vertex = dev.readable_mesh<VTXpntu>(model_vbv) + start_vertex;
		auto  nlines       = sub->num_vertices;
		auto  nverts       = nlines * 2;

		oAssert(model_vbv.vertex_stride_bytes() == sizeof(VTXpntu), "invalid vertex buffer view");

		auto v = cl.new_transient_vertices<VTXpc>(nverts);
		if (!v)
			continue;

		gfx::draw_constants draw(world, pov.view(), pov.projection());
		cl.set_cbv(oGFX_CBV_DRAW, &draw, sizeof(draw));

		auto v_end = v + nverts;
		while (v < v_end)
		{
			v->position = model_vertex->position;
			v->color    = argb;
			v++;
			v->position = model_vertex->position + model_vertex->normal * scale;
			v->color    = argb;
			v++;
			model_vertex++;
		}

		auto view = cl.commit_transient_vertices();
		
		cl.set_vertices(0, 1, &view);
		cl.draw(nverts);
	}
}

void debug_draw_tangents(technique_context_t& ctx)
{
	auto& cl  = *ctx.gcl;
	auto& pov = *ctx.pov;
	auto& dev = *cl.device();

	cl.set_pso(gfx::pipeline_state::lines_vertex_color);

	ForEachTask
	{
		auto  sub          = (debug_model_tangents_submission_t*)task->data;
		auto& world        = sub->world;
		auto  model_vbv    = sub->vertices;
		auto  start_vertex = sub->start_vertex;
		auto  pos_argb     = sub->pos_argb;
		auto  neg_argb     = sub->neg_argb;
		auto  scale        = sub->scale;
		auto  model_vertex = dev.readable_mesh<VTXpntu>(model_vbv) + start_vertex;
		auto  nlines       = sub->num_vertices;
		auto  nverts       = nlines * 2;

		oAssert(model_vbv.vertex_stride_bytes() == sizeof(VTXpntu), "invalid vertex buffer view");

		auto v = cl.new_transient_vertices<VTXpc>(nverts);
		if (!v)
			continue;

		gfx::draw_constants draw(world, pov.view(), pov.projection());
		cl.set_cbv(oGFX_CBV_DRAW, &draw, sizeof(draw));

		auto v_end = v + nverts;
		while (v < v_end)
		{
			uint32_t color = model_vertex->tangent.w < 0.0f ? neg_argb : pos_argb;
			v->position    = model_vertex->position;
			v->color       = color;
			v++;
			v->position    = model_vertex->position + model_vertex->tangent.xyz() * scale;
			v->color       = color;
			v++;
			model_vertex++;
		}

		auto view = cl.commit_transient_vertices();
		
		cl.set_vertices(0, 1, &view);
		cl.draw(nverts);
	}
}

technique_t s_techniques[] = 
{
	pass_begin,
	view_begin,
	linearize_depth,
	draw_lines,
	draw_prim,
	draw_subset,
	draw_axis,
	draw_gizmo,
	draw_grid,
	debug_draw_normals,
	debug_draw_tangents,
	view_end,
	pass_end,
};
match_array_e(s_techniques, render_technique);

// NOTE: s_techniques[255] is reserved
static_assert(countof(s_techniques) < 256, "Too many techniques for sort key's bitmask");
	
#define MB * 1024 * 1024

// returns the number of ibvs. Pass nullptrs to out_vbv and out_ibvs first to return the value to
// allocate for out_ibvs.
static uint32_t btt_initialize_vbv_ibvs(const char* name, gpu::device* dev, uint16_t max_depth, gpu::vbv* out_vbv, gpu::ibv* out_ibvs, allocator& temp_alloc = default_allocator)
{
	uint32_t last_patch_index    = prim::btt_calc_index(max_depth, 0);

	// Set up the various patch LoDs & links
	if (out_ibvs)
	{
		uint32_t max_patch_indices = prim::btt_num_tris(max_depth) * 3;
		uint32_t indices_bytes     = max_patch_indices * sizeof(uint16_t);
		uint32_t btt_bytes         = prim::btt_calc_memory_bytes(max_depth);
		uint32_t total_temp_bytes  = btt_bytes + indices_bytes;
		blob     btt               = temp_alloc.scoped_allocate(total_temp_bytes, "temp populate_btt_vbv_ibvs");
		uint16_t* indices          = (uint16_t*)((char*)btt + btt_bytes);

		prim::btt_init(btt, max_depth);

		for (uint32_t patch_index = 0; patch_index <= last_patch_index; patch_index++)
		{
			auto num_indices = prim::btt_write_indices(btt, max_depth, patch_index, indices);
			out_ibvs[patch_index] = dev->new_ibv(name, num_indices, indices);
		}
	}
	
	// Set up the patch's vertex field
	if (out_vbv)
	{
		auto num_verts = prim::btt_num_vertices(max_depth);
		auto verts = temp_alloc.scoped_allocate(num_verts * sizeof(float3), "temp patch verts");
		prim::btt_vertices(max_depth, verts, sizeof(float3));
		*out_vbv = dev->new_vbv(name, sizeof(float3), num_verts, verts);
	}

	return last_patch_index + 1;
}

void renderer_t::initialize(const renderer_init_t& init, window* win)
{
	auto registry_alloc = default_allocator;
	auto io_alloc = default_allocator;

	uint32_t model_registry_bytes                 = 256 MB;
	uint32_t model_registry_bookkeeping_bytes     = 16  MB;
	uint32_t transient_mesh_bytes                 = 16  MB;
	uint32_t texture2d_registry_bytes             = 512 MB;
	uint32_t texture2d_registry_bookkeeping_bytes = 16  MB;

	// @tony: texture budget? D3D doesn't help much here.

	// init the device & sign it
	{
		gpu::device_init i("oGfx Renderer GPU Device");
		i.enable_driver_reporting = true;
		i.max_transient_mesh_bytes = transient_mesh_bytes;
		i.max_persistent_mesh_bytes = model_registry_bytes;
		dev_ = gpu::new_device(i, win);

		// sets up basic topology used by gfx as well as defines the 
		// enumerations to be used for set_pso().
		sign_device(dev_);

		// init terrain primitives
		if (0)
		{
			static uint16_t s_max_depth = 7;
			uint32_t n = btt_initialize_vbv_ibvs("terrain patches", dev_, s_max_depth, nullptr, nullptr);
			oAssert(n == btt_patch_indices_.size(), "size mismatch");
			btt_initialize_vbv_ibvs("terrain patches", dev_, s_max_depth, &btt_verts_, btt_patch_indices_.data(), default_allocator);
		}
	}

	// set up system back-buffers
	auto client_size = win->client_size();
	film_.initialize(dev_, client_size.x, client_size.y);

	// initialize resource managers
	models_.initialize(dev_, model_registry_bookkeeping_bytes, registry_alloc, io_alloc);

	texture2ds_.initialize(dev_, texture2d_registry_bookkeeping_bytes, registry_alloc, io_alloc);

	frame_id_ = 0;

	// FIXME: these max's could be exposed to the init struct.

	static const uint32_t global_list_bytes = 32 * sizeof(void*);

	global_heaps_ = (flexible_array_t<void*, true>*)default_allocate(2 * global_list_bytes, "renderer global tasklists");
	global_heaps_->initialize(flexible_array_bytes_t(), global_list_bytes);

	global_tasklists_ = (flexible_array_t<void*, true>*)((char*)global_heaps_ + global_list_bytes);
	global_tasklists_->initialize(flexible_array_bytes_t(), global_list_bytes);
}

void renderer_t::deinitialize()
{
	global_tasklists_->deinitialize();
	default_deallocate(global_heaps_->deinitialize());
	global_heaps_ = nullptr;
	global_tasklists_ = nullptr;

	// deinit terrain primitives
	{
		for (auto& v : btt_patch_indices_)
			dev_->del_ibv(v);
		dev_->del_vbv(btt_verts_);
	}
	
	// ensure all registries are done processing loads
	filesystem::join();

	texture2ds_.deinitialize();
	models_.deinitialize();
	film_.deinitialize();
}

void renderer_t::begin_submit()
{
	frame_id_++;
	submission_overflow_ = false;
	dev_->begin_frame();
}

void renderer_t::end_submit()
{
	uint32_t num_tasks = 0;
	void* master_tasks = consolidate_master_tasklist(&num_tasks);

	//trace_master_tasklist(master_tasks, num_tasks);

	kick(master_tasks, num_tasks);
	
	global_heaps_->visit([](void* ptr, void* user) { default_deallocate(ptr); }, nullptr);
	global_heaps_->clear();

	dev_->end_frame();
	dev_->present();

	default_deallocate(master_tasks);

	global_tasklists_->visit([](void* ptr, void* user) { default_deallocate(ptr); }, nullptr);
	global_tasklists_->clear();

	flush();
}

void renderer_t::begin_view(const pov_t* pov, const render_settings_t* render_settings)
{
	pov_ = pov;
	render_settings_ = render_settings;
}

void renderer_t::end_view()
{
}

void* renderer_t::consolidate_master_tasklist(uint32_t* out_num_tasks)
{
	if (submission_overflow_)
	{
#if 0
		// Build something simpler
		if (on_overflow)
		{
			// Reset render plan to be trivially simple, overwriting frame data so as not to need new allocation
			global_overflow = false;
			nglobal_tasklists = 1;
			global_tasklists[0]->ntasks = 0;
			local_tasklist = global_tasklists[0];
			max_global_taskslists = 1;

			// Call the handler
			on_overflow();

			// Then restore the global limits
			max_global_taskslists = kMaxGlobalTasklists;

			if (global_overflow)
				oThrow(std::errc::invalid_argument, "overflow handler itself overflowed, make it simpler");
		}

		else
			oThrow(std::errc::invalid_argument, "frame overflow with no overflow handler");
#endif

		return nullptr;
	}

	auto& tasklists = *(flexible_array_t<tasklist_t*, true>*)global_tasklists_;

	// accumulate task count
	uint32_t num_tasks = 0;
	tasklists.visit([](tasklist_t* t, void* user) { *(uint32_t*)user += t->size(); }, &num_tasks);

	// reserve count for pushing/popping debug/profiling markers per pass and a terminator task
	num_tasks += (int)render_pass::count * 2 + 1;

	// FIXME: alloc, OOM handling
	auto master_tasklist = (task_t*)default_allocate(num_tasks * sizeof(task_t), "renderer master tasklist");
	if (!master_tasklist)
	{
		submission_overflow_ = true;
		return nullptr;
	}

	task_t* task = master_tasklist;

	// A stable sort will keep first things first, and last things last. So in order to insert
	// first things first, they need to appear before other entries, so manually insert them
	// into the master list prior to appending added tasks.
	for (uint64_t pass = 0; pass < uint64_t(render_pass::count); pass++, task++)
	{
		task->key = (pass << pass_shift) | (uint64_t(render_technique::pass_begin) << technique_shift)/* | priority highest: 0*/;
		task->data = as_string((render_pass)pass);
	}

	// append user tasks
	tasklists.visit([](tasklist_t* t, void* user)
	{
		task_t** task_iter = (task_t**)user;
		auto size = t->size();
		memcpy(*task_iter, t->data(), size * sizeof(task_t));
		*task_iter += size;
	}, &task);
	
	// append extra tasks after added tasks to ensure they always appear last
	for (uint64_t pass = 0; pass < uint64_t(render_pass::count); pass++, task++)
	{
		task->key = (pass << pass_shift) | (uint64_t(render_technique::pass_end) << technique_shift) | priority_mask; // (lowest prio: -1)
		task->data = as_string((render_pass)pass);
	}

	// Finally append a task that's like no other, allows a test-for-end hoist in kick()
	task->key = uint64_t(-1);
	task->data = "master tasklist end";
	task++;

	// Sort master list for final processing order
	std::stable_sort(master_tasklist, task, [](const task_t& a, const task_t& b)->bool { return a.key < b.key; } );

	*out_num_tasks = num_tasks;
	return master_tasklist;
}

void renderer_t::trace_master_tasklist(const void* master_tasks, uint32_t num_tasks)
{
	auto tasks = (const task_t*)master_tasks;

	for (uint32_t i = 0; i < num_tasks; i++)
	{
		if (tasks[i].key == uint64_t(-1))
			continue;

		auto pass      = (render_pass)     ((tasks[i].key & pass_mask)      >> pass_shift);
		auto technique = (render_technique)((tasks[i].key & technique_mask) >> technique_shift);

		char buf[256];
		const char* extra = "";

		switch (technique)
		{
			case render_technique::pass_begin:
			case render_technique::pass_end:
				extra = (const char*)tasks[i].data;
				break;

			default:
				snprintf(buf, "0x%p", tasks[i].data);
				extra = buf;
				break;
		}

		oTrace("[RENDER] %05d %-20s %-20s %s", i, as_string(pass), as_string(technique), extra);
	}
}

void renderer_t::kick(void* t, uint32_t num_tasks)
{
	task_t* tasks = (task_t*)t;

  // Transform submission tasks into GPU commands calls
  // Can execute different passes concurrently -> write to different cmdbuffers
  // Need to split up big passes: gbuffer static, shadows static
  
  // All passes are in-order. All same techniques are adjacent. Scan for a change
  // and schedule the job for the workchunk
  
  const uint64_t test_mask = pass_mask | technique_mask;

	technique_context_t ctx;
	ctx.models = &models_;
	ctx.texture2ds = &texture2ds_;
	ctx.gcl = dev_->immediate();
	ctx.presentation_target = dev_->get_presentation_rtv();
	ctx.film = film();
	ctx.pov = pov_;
	ctx.render_settings = render_settings_ ? *render_settings_ : render_settings_t();
  
  auto cur = tasks;
  auto end = tasks + num_tasks;
  while (cur < end && cur->key != uint64_t(-1))
  {
    // Look ahead to count tasks that share the same technique. A check for the 
    // final end-of-list isn't necessary in the loop because there's a single task
    // at the end of the list that won't be matched by any other.
    auto match = cur->key & test_mask;
    auto technique_end = cur + 1;
    for (; (technique_end->key & test_mask) == match; technique_end++);
    auto nmatches = (uint32_t)(technique_end - cur);
		auto key = cur->key;
    uint32_t technique = (key >> technique_shift) & 0xff;
		uint32_t pass = (key >> pass_shift) & 0xff;
    auto fn = s_techniques[technique];
    
    // Maybe if a pass is flagged for split, do the alternative logic below,
    // otherwise ensure it runs to the end of the technique as well as its pass
    // so a whole pass is in one bin. (I WANT to split gbuffer, shadows, but I don't want 
    // to split post, do I? Or something like updating buffers.

    // Can this be kicked to another thread/fiber? The challenge is that per-technique is
    // a bit fine-grained. Per-pass maybe makes more sense?

		ctx.tasks     = cur;
		ctx.num_tasks = nmatches;
		ctx.pass      = (render_pass)pass;
		ctx.technique = (render_technique)technique;

    fn(ctx);
    
    // Alternative: I got one list for ALL render commands and the count.
    // get a subcount = count / nthreads.
    // get last subentry, check technique, and run until technique end.
    // Schedule out all those things to write to command buffers for the frame (tagged_alloc for a bunch of those things)
    // "Last one to finish" Writes all those command buffers to the device. Per-thread-per-pass. Likely a thread will only alloc one cmdbuf for a pass
    // because the whole pass is on that thread. THis isn't true, but can I make it true?



    cur = technique_end;
  }

	// NOTE should have some kind of GPU write-back to notify CPU frame is done, but this kick() isn't anywhere
	// near hooked up correctly, so this is just a reminder.
}

void* renderer_t::allocate(uint32_t bytes, uint32_t alignment)
{
	if (local_heap_frame_id_ != frame_id_)
	{
		local_heap_frame_id_ = frame_id_;
		static const uint32_t local_heap_page_bytes = 2 * 1024 * 1024;
		local_heap_ = default_allocate(local_heap_page_bytes, "renderer local heap");
		local_heap_end_ = (char*)local_heap_ + local_heap_page_bytes;

		void** new_heap = global_heaps_->add();
		if (!new_heap)
			submission_overflow_ = true;
		*new_heap = local_heap_;
	}

	void* p = align(local_heap_, alignment);
	void* end = (char*)p + bytes;

	if (end >= local_heap_end_)
	{
		local_heap_frame_id_ = 0;
		return allocate(bytes, alignment);
	}

	local_heap_ = end;
	return p;
}

void renderer_t::internal_submit(uint64_t priority, const uint8_t pass, const uint8_t technique, void* data)
{
	// validate input
	if ((priority & priority_mask) != priority)
		oThrow(std::errc::invalid_argument, "invalid priority, must match priority_mask");

	tasklist_t* tasklist = (tasklist_t*)local_tasklist_;

	// if a different frame or out of space, allocate a new tasklist
	if (local_tasklist_frame_id_ != frame_id_ || !tasklist || tasklist->full())
	{
		local_tasklist_frame_id_ = frame_id_;
		static const uint32_t tasklist_page_bytes = 256 * 1024;
		tasklist = new (default_allocate(tasklist_page_bytes, "renderer tasklist")) tasklist_t(flexible_array_bytes_t(), tasklist_page_bytes);

		void** new_tasklist = global_tasklists_->add();
		if (!new_tasklist)
			submission_overflow_ = true;
		*new_tasklist = local_tasklist_ = tasklist;
	}

	// pack task
	auto task = tasklist->add();
	task->key = (uint64_t(pass) << pass_shift) | (uint64_t(technique) << technique_shift) | priority;
	task->data = data;
}

void renderer_t::flush(uint32_t max_operations)
{
	auto completed = models_.flush(max_operations);
	if (completed >= max_operations)
		return;
	max_operations -= completed;

	completed = texture2ds_.flush(max_operations);
	if (completed >= max_operations)
		return;
	max_operations -= completed;
}

void renderer_t::on_window_resizing()
{
	if (!dev_)
		return;
	
	dev_->on_window_resizing();
}

void renderer_t::on_window_resized(uint32_t new_width, uint32_t new_height)
{
	// Can I derive the info for film from dev? film probably shouldn't
	// be dynamically resized anyway, though the depth has to be 1:1, no?

	// Also, dev knows about the window, can it hook at a lower level?
	// At least the renderer here knows about the window, can it hook too?

	if (!dev_)
		return;

	film_.resize(new_width, new_height);
	dev_->on_window_resized();
}

}}
