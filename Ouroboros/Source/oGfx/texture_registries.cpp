// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oGfx/texture_registries.h>
#include <oSurface/codec.h>
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

blob encode_oimg(blob& compiled, const allocator& alloc)
{
	const allocator& temp_alloc = default_allocator;

	auto fformat = surface::get_file_format(compiled);
	if (fformat == surface::file_format::unknown)
		oThrow(std::errc::invalid_argument, "unknown file format");

	if (fformat == surface::file_format::oimg)
		return std::move(compiled);

	auto info = surface::get_info(compiled);
	auto desired_format = surface::as_texture(info.format); // ensure the format is render-compatible
	desired_format = surface::as_unorm(desired_format); // srgb not hooked up yet in renderer
	auto img = surface::decode(compiled, temp_alloc, temp_alloc, desired_format, surface::mip_layout::tight);
	return encode(img, surface::file_format::oimg, alloc, temp_alloc, surface::format::unknown, surface::compression::none);
}

void texture2d_registry::initialize(gpu::device* dev, uint32_t bookkeeping_bytes, const allocator& alloc, const allocator& io_alloc)
{
	resource_placeholders_t placeholders;
	placeholders.compiled_missing = make_solid_image_file(color::default_gray, io_alloc);
	placeholders.compiled_loading = placeholders.compiled_missing.alias();
	placeholders.compiled_failed = make_solid_image_file(color::red, io_alloc);

	auto mem = alloc.allocate(bookkeeping_bytes, "texture2d_registry", memory_alignment::cacheline);
  initialize_base(dev, "texture2d_registry", bookkeeping_bytes, alloc, io_alloc, placeholders);
}

void texture2d_registry::deinitialize()
{
	if (!valid())
		return;

	deinitialize_base();
}

void* texture2d_registry::create(const char* name, blob& compiled)
{
	auto fformat = surface::get_file_format(compiled);
	oCheck(fformat != surface::file_format::unknown, std::errc::invalid_argument,"Unknown file format: %s", name ? name : "(null)");

	auto info = surface::get_info(compiled);
	oCheck(info.is_2d(), std::errc::invalid_argument, "%s is not a 2d texture", name ? name : "(null)");

	// ensure the format is render-compatible
	auto desired_format = surface::as_texture(info.format);

	// @tony: don't want native YUV format, get back to rgb
	if (surface::is_yuv(desired_format))
		desired_format = surface::format::b8g8r8a8_unorm;

	desired_format = surface::as_unorm(desired_format); // srgb not hooked up yet in renderer

	auto img = surface::decode(compiled, desired_format, surface::mip_layout::tight);

	auto tex = pool_.create();
	tex->view = dev_->new_texture(name, img);
	oTrace("[texture2d_registry] create %s", name);
	return tex;
}

void texture2d_registry::destroy(void* entry)
{
	auto tex = (basic_resource_type*)entry;
	tex->view = nullptr;
	pool_.destroy(tex);
}

texture2d_t texture2d_registry::load(const path_t& path)
{
	return default_load(path);
}

void texture2d_registry::unload(const texture2d_t& tex)
{
	remove(tex);
}

}}
