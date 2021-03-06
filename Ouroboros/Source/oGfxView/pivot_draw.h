// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#pragma once

#include "gfx_view.h"
#include <oGfx/scene.h>
#include <oMesh/model.h>
#include <array>

namespace ouro {

class pivot_draw : public gfx_view
{
public:
	pivot_draw();
	~pivot_draw();

	void initialize() override;
	void deinitialize() override;
	void on_view_default() override;
	void on_resized(uint32_t new_width, uint32_t new_height) override;
	void update(float delta_time) override;
	void submit_scene(gfx::renderer_t& renderer) override;

	virtual void request_model_load(gfx::model_registry& registry, const uri_t& uri_ref) override;

private:
	std::array<gfx::pivot_t*, 100> pivots_;
	gfx::texture2d_t texture_;
	gfx::model_t     model_;
};

}