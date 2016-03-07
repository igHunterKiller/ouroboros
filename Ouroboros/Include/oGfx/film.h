// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Buffers used for various render passes sized proportionately to the main 
// presentation buffer.

#pragma once
#include <oGPU/gpu.h>

namespace ouro { namespace gfx {

class film_t
{
public:
	enum color
	{
		gbuffer_normals,
		gbuffer_diffuse_emissive,
		gbuffer_specular_gloss,
		linear_depth,
		halfres_linear_depth,
		forward,
		tonemapped,

		color_count,
	};

	enum depth
	{
		hyper_depth,
		depth_count,
	};

	film_t() {}
	~film_t() { deinitialize(); }

	film_t(film_t&& that);
	film_t& operator=(film_t&& that);

	void initialize(gpu::device* dev, uint32_t width, uint32_t height);
	void deinitialize();

	inline uint2 dimensions() const { auto desc = depth_dsvs_[hyper_depth]->get_resource()->get_desc(); return uint2(desc.width, desc.height); }

	// only call this when the buffer is not in used
	void resize(uint32_t width, uint32_t height);

	gpu::rtv* rtv(const color& c) const { return (gpu::rtv*)color_rtvs_[c].c_ptr(); }
	gpu::dsv* dsv(const depth& d) const { return (gpu::dsv*)depth_dsvs_[d].c_ptr(); }
	gpu::srv* srv(const color& c) const { return (gpu::srv*)color_srvs_[c].c_ptr(); }
	gpu::srv* srv(const depth& d) const { return (gpu::srv*)depth_srvs_[d].c_ptr(); }

private:
	ref<gpu::device> dev_;
	std::array<ref<gpu::rtv>, color_count> color_rtvs_;
	std::array<ref<gpu::dsv>, depth_count> depth_dsvs_;
	std::array<ref<gpu::srv>, color_count> color_srvs_;
	std::array<ref<gpu::srv>, depth_count> depth_srvs_;

	film_t(const film_t&);
	const film_t& operator=(const film_t&);
};
	
}}
