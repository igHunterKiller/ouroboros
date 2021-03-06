// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#pragma once

#include "gfx_view.h"

namespace ouro {

class btt_test : public gfx_view
{
public:
	btt_test();
	~btt_test();

	void initialize() override {}
	void deinitialize() override {}
	void on_view_default() override;
	void on_resized(uint32_t new_width, uint32_t new_height) override;
	void update(float delta_time) override;
	void submit_scene(gfx::renderer_t& renderer) override;

private:
	ref<gpu::dsv> depth_target_;
	ref<gpu::srv> depth_srv_;
	ref<gpu::rtv> mouse_depth_rtv_;
	ref<gpu::resource> mouse_depth_rb_;

	std::vector<gpu::ibv> btt_patch_indices_;
	gpu::vbv btt_verts_;
	gpu::vbv ground_plane_;

	void render_mouse_depth();
};

}