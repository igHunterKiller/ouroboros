// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oCore/countof.h>
#include <oGfx/film.h>

namespace ouro { namespace gfx {

struct render_target_init
{
	const char* name;
	surface::format format;
	uint32_t divide_width;
	uint32_t divide_height;
};

static const render_target_init s_color_targets[] = 
{
	{ "gbuffer_normals",                 surface::format::r10g10b10a2_unorm,   1, 1 },
	{ "gbuffer_diffuse_emissive",        surface::format::r8g8b8a8_unorm,      1, 1 },
	{ "gbuffer_specular_gloss",          surface::format::r8g8b8a8_unorm,      1, 1 },
	{ "linear_depth",                    surface::format::r32_float,           1, 1 },
	{ "halfres_linear_depth",            surface::format::r32_float,           2, 2 },
	{ "forward",                         surface::format::r11g11b10_float,     1, 1 },
	{ "tonemapped",                      surface::format::r11g11b10_float,     1, 1 },
};
match_array(s_color_targets, film_t::color_count);

static const render_target_init s_depth_targets[] = 
{
	{ "hyper_depth",                     surface::format::d24_unorm_s8_uint,   1, 1 },
};
match_array(s_depth_targets, film_t::depth_count);

film_t::film_t(film_t&& that)
{
	dev_ = that.dev_; that.dev_ = nullptr;

	for (size_t i = 0; i < depth_dsvs_.size(); i++)
	{
		depth_dsvs_[i] = std::move(that.depth_dsvs_[i]);
		depth_srvs_[i] = std::move(that.depth_srvs_[i]);
	}

	for (size_t i = 0; i < color_rtvs_.size(); i++)
	{
		color_rtvs_[i] = std::move(that.color_rtvs_[i]);
		color_srvs_[i] = std::move(that.color_srvs_[i]);
	}
}

film_t& film_t::operator=(film_t&& that)
{
	if (this != &that)
	{
		dev_ = that.dev_; that.dev_ = nullptr;
	
		for (size_t i = 0; i < depth_dsvs_.size(); i++)
		{
			depth_dsvs_[i] = std::move(that.depth_dsvs_[i]);
			depth_srvs_[i] = std::move(that.depth_srvs_[i]);
		}

		for (size_t i = 0; i < color_rtvs_.size(); i++)
		{
			color_rtvs_[i] = std::move(that.color_rtvs_[i]);
			color_srvs_[i] = std::move(that.color_srvs_[i]);
		}
	}

	return *this;
}

void film_t::initialize(gpu::device* dev, uint32_t width, uint32_t height)
{
	dev_ = dev;

	for (size_t i = 0; i < depth_dsvs_.size(); i++)
	{
		const render_target_init& r = s_depth_targets[i];
		depth_dsvs_[i] = dev->new_dsv(r.name, width / r.divide_width, height / r.divide_height, r.format);
		depth_srvs_[i] = dev->new_srv(depth_dsvs_[i]->get_resource());
	}

	for (size_t i = 0; i < color_rtvs_.size(); i++)
	{
		const render_target_init& r = s_color_targets[i];
		color_rtvs_[i] = dev->new_rtv(r.name, width / r.divide_width, height / r.divide_height, r.format);
		color_srvs_[i] = dev->new_srv(color_rtvs_[i]->get_resource());
	}
}

void film_t::deinitialize()
{
	color_rtvs_.fill(nullptr);
	depth_dsvs_.fill(nullptr);
	color_srvs_.fill(nullptr);
	depth_srvs_.fill(nullptr);
	dev_ = nullptr;
}

void film_t::resize(uint32_t width, uint32_t height)
{
	gpu::device* dev = dev_;
	deinitialize();
	initialize(dev, width, height);
}

}}
