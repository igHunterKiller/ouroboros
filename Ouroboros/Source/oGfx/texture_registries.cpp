// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oGfx/texture_registries.h>
#include <oSurface/codec.h>
#include <oCore/assert.h>
#include <oCore/color.h>

namespace ouro { namespace gfx {

static blob make_solid_image_file(uint32_t argb, const allocator& alloc)
{
	surface::info_t info;
	info.dimensions = uint3(8,8,1);
	info.format = surface::format::b8g8r8a8_unorm; // not considering srgb yet
	info.mip_layout = surface::mip_layout::tight;
	surface::image img(info, alloc);
	img.fill(argb);
	return encode(img, surface::file_format::oimg, alloc, alloc, surface::format::unknown, surface::compression::none);
}

// todo: use this from the async_load continuation to save out a cached version of the 
// compiled file format, and then subsequent loads can load that instead.
// Open question: should the baking of unbaked assets be done in the IO/threads or queued
// and done concurrently in the create step?

void texture2d_registry::initialize(gpu::device* dev, uint32_t budget_bytes, const allocator& alloc, const allocator& io_alloc)
{
	alloc_ = alloc;

	allocate_options opts(required_alignment);
	void* memory = alloc.allocate(budget_bytes, "texture2d_registry", opts);

	auto error_placeholder = make_solid_image_file(color::white, io_alloc);

	device_resource_registry2_t<texture2d_internal>::initialize("tex2d registry", memory, budget_bytes, dev, error_placeholder, io_alloc);
}

void texture2d_registry::deinitialize()
{
	if (pool_.valid())
	{
		void* p = device_resource_registry2_t<texture2d_internal>::deinitialize();

		if (alloc_)
			alloc_.deallocate(p);

		alloc_ = allocator();
	}
}

texture2d_registry::handle texture2d_registry::load(const path_t& path)
{
	return base_t::load(path, nullptr, false);
}

void* texture2d_registry::create(const path_t& path, blob& compiled)
{
	auto fformat = surface::get_file_format(compiled);
	oCheck(fformat != surface::file_format::unknown, std::errc::invalid_argument, "[texture2d_registry] Unknown file format: %s", path.c_str());

	auto info = surface::get_info(compiled);
	oCheck(info.is_2d(), std::errc::invalid_argument, "[texture2d_registry] Not a texture2d: %s", path.c_str());

	// ensure the format is render-compatible
	auto desired_format = surface::as_texture(info.format);

	// @tony: don't want native YUV format, get back to rgb
	if (surface::is_yuv(desired_format))
		desired_format = surface::format::b8g8r8a8_unorm;

	desired_format = surface::as_unorm(desired_format); // srgb not hooked up yet in renderer

	auto img = surface::decode(compiled, desired_format, surface::mip_layout::tight);

	auto tex = pool_.create();
	tex->view = dev_->new_texture(path, img);
	oTrace("[texture2d_registry] create %p %s", tex, path.c_str());
	return tex;
}

void texture2d_registry::destroy(void* resource)
{
	auto tex = (basic_resource_type*)resource;
	tex->view = nullptr;
	pool_.destroy(tex);
}

}}
