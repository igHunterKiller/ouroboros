// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include "btt_test.h"

#include <oMath/btt.h>

using namespace ouro;

// returns the number of ibvs. Pass nullptrs to out_vbv and out_ibvs first to return the value to
// allocate for out_ibvs.
uint32_t populate_btt_vbv_ibvs(const char* name, gpu::device* dev, uint16_t max_depth, gpu::vbv* out_vbv, gpu::ibv* out_ibvs, allocator& alloc = default_allocator)
{
	uint32_t last_patch_index    = prim::btt_calc_index(max_depth, 0);

	// Set up the various patch LoDs & links
	if (out_ibvs)
	{
		uint32_t max_patch_indices = prim::btt_num_tris(max_depth) * 3;
		uint32_t indices_bytes     = max_patch_indices * sizeof(uint16_t);
		uint32_t btt_bytes         = prim::btt_calc_memory_bytes(max_depth);
		uint32_t total_temp_bytes  = btt_bytes + indices_bytes;
		blob btt      = alloc.scoped_allocate(total_temp_bytes, "temp populate_btt_vbv_ibvs");
		uint16_t* indices          = (uint16_t*)((char*)btt + btt_bytes);

		prim::btt_init(btt, max_depth);

		for (uint32_t patch_index = 0; patch_index <= last_patch_index; patch_index++)
		{
			auto num_indices = prim::btt_write_indices(btt, max_depth, patch_index, indices);
			//out_ibvs[patch_index] = dev->new_ibv(name, num_indices, indices);
			if (1)
				throw std::invalid_argument("fixme");
		}
	}
	
	// Set up the patch's vertex field
	if (out_vbv)
	{
		auto num_verts = prim::btt_num_vertices(max_depth);
		auto verts = default_allocator.scoped_allocate(num_verts * sizeof(float3), "temp patch verts");
		prim::btt_vertices(max_depth, verts, sizeof(float3));
		//*out_vbv = dev->new_vbv(name, sizeof(float3), num_verts, verts);
		throw std::invalid_argument("fixme");
	}

	return last_patch_index + 1;
}

btt_test::btt_test()
{
#pragma message("btt_test needs resurrection")
#if 0
	auto dev = renderer_.dev();

	depth_target_    = dev->new_dsv("depth", dev->get_presentation_rtv(), surface::format::d32_float_s8x24_uint);
	depth_srv_       = dev->new_srv(depth_target_->get_resource());
	mouse_depth_rtv_ = dev->new_rtv("depth under mouse", 1, 1, surface::format::r32_float);
	mouse_depth_rb_  = dev->new_rb(mouse_depth_rtv_->get_resource());
	
	btt_test::on_view_default();

	// set up a ground plane
	if (0)
	{
		static const float ground_verts[] = 
		{
			-10.0f, -10.0f, 0.0f,
			-10.0f,  10.0f, 0.0f,
			 10.0f, -10.0f, 0.0f,
			 10.0f, -10.0f, 0.0f,
			-10.0f,  10.0f, 0.0f,
			 10.0f,  10.0f, 0.0f,
		};

		//ground_plane_ = dev->new_vbv("ground plane", sizeof(float3), 6, ground_verts);
		throw std::invalid_argument("fixme");
	}

	static uint16_t s_max_depth = 7;
	uint32_t n = populate_btt_vbv_ibvs("terrain patches", dev, s_max_depth, nullptr, nullptr);
	btt_patch_indices_.resize(n);
	populate_btt_vbv_ibvs("terrain patches", dev, s_max_depth, &btt_verts_, btt_patch_indices_.data(), default_allocator);
#endif
}

btt_test::~btt_test()
{
}

void btt_test::update(float delta_time)
{
#if 0
	float mouse_VSdepth = 0.0f;
	mouse_depth_rb_->copy_to(&mouse_VSdepth, sizeof(float));
	if (mouse_VSdepth < (pov_.far_dist() * 0.95f))
		pov_.pointer_depth(mouse_VSdepth);

	float3 mouse_WSpos = pov_.unproject_pointer(mouse_.x(), mouse_.y());
	display_mouse_position(mouse_WSpos);
#endif
}

void btt_test::render_mouse_depth()
{
#pragma message("render_mouse_depth disabled")
#if 0
	if (!mouse_.pressed(mouse_button::middle))
		return;

	auto dev = renderer_.dev();

	auto cl = dev->immediate();

	cl->set_pso(gfx::pipeline_state::mouse_depth);

	auto desc = depth_target_->get_resource()->get_desc();

	float2 mouse_ss_texcoord = pov_.viewport().viewport_to_texcoord(float2(mouse_.x(), mouse_.y()));

	gfx::misc_constants misc;
	misc.a = float4(mouse_ss_texcoord, 0.0f, 0.0f);
	cl->set_cbv(oGFX_CBV_MISC, &misc, sizeof(misc));

	cl->set_rtv(mouse_depth_rtv_); // unbinds depth
	cl->set_srvs(oGFX_SRV_DEPTH, 1, &depth_srv_);
	cl->set_vertices(0, 1, nullptr);
	cl->draw(3);

	gpu::copy_box mouse_box = { 0,0,0, 1,1,1 };
	cl->copy_texture_region(mouse_depth_rb_, 0, 0, 0, 0, mouse_depth_rtv_->get_resource(), 0, &mouse_box);
	cl->set_srvs(oGFX_SRV_DEPTH, 1, nullptr); // ensure this isn't set when later it's set as a dsv
#endif
}

void btt_test::submit_scene(gfx::renderer_t& render)
{
#pragma message("btt_test rendering disabled")
#if 0
	auto dev = renderer_.dev();

	auto cl = dev->immediate();
	cl->clear_rtv(dev->get_presentation_rtv(), color::almost_black);
	cl->clear_dsv(depth_target_);
	cl->set_rtv(dev->get_presentation_rtv(), depth_target_);
	cl->set_rso(gfx::signature::graphics);

	{
		auto desc = dev->get_presentation_rtv()->get_resource()->get_desc();
		gfx::view_constants vc(pov_.view(), pov_.projection(), desc.width, desc.height, 0);
		cl->set_cbv(oGFX_CBV_VIEW, &vc, sizeof(vc));
	}

	{
		cl->set_pso(gfx::pipeline_state::pos_only_wire);
		cl->set_vertices(0, 1, &btt_verts_);

		gfx::draw_constants draw;

		float3 base(-3.0f, -8.0f,  0.0f);
		float3 spacing(0.5f, 0.5f, 0.0f);

		uint16_t level = 0, link = 0;
		uint16_t cur_level = 0;
		uint16_t i = 0;
		float3 pos = base;
		while (i < btt_patch_indices_.size())
		{
			pos.x = base.x;
			while (cur_level == level)
			{
				if (i >= btt_patch_indices_.size())
					break;

				draw.set_transform(translate(pos), pov_.view(), pov_.projection());

				cl->set_cbv(oGFX_CBV_DRAW, &draw, sizeof(draw));

				cl->set_indices(btt_patch_indices_[i]);
				cl->draw_indexed(btt_patch_indices_[i].num_indices);

				pos.x += 1.0f + spacing.x;
				pos.z += 1.0f;

				i++;
				prim::btt_calc_depth_and_link(i, &level, &link);
			}

			cur_level = level;
			pos.y += 1.0f + spacing.y;
		}
	}

	render_mouse_depth();

	//cl->set_pso(gfx::pipeline_state::pos_only);
	//cl->set_vertices(0, 1, &ground_plane_);
	//cl->draw(ground_plane_.num_vertices);
	
	//cl->set_indices(chunk_indices_);
	//cl->set_vertices(0, 1, &chunk_verts_);
	//cl->draw_indexed(chunk_indices_.num_indices);
#endif
}

void btt_test::on_view_default()
{
	focus(float3(0.0f, 0.0f, -5.0f), float3(0.0f, 0.0f, 0.0f));
}

void btt_test::on_resized(uint32_t new_width, uint32_t new_height)
{
#if 0
	auto dev = renderer_.dev();

	depth_target_ = dev->new_dsv("depth", dev->get_presentation_rtv(), surface::format::d32_float_s8x24_uint);
	depth_srv_    = dev->new_srv(depth_target_->get_resource());
#endif
}
