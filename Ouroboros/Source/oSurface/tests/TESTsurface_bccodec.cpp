// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include <oCore/assert.h>
#include <oSystem/filesystem.h>
#include <oGPU/gpu.h>
#include <oSurface/surface.h>
#include <oSurface/codec.h>

using namespace ouro;
using namespace ouro::surface;

static void convert_and_test(unit_test::services& services, gpu::device* dev, gpu::rtv* target, const format& target_format, const char* filename_suffix, uint nth_test, bool save_to_desktop = false)
{
	path_t TestImagePath = "Test/Textures/lena_1.png";

	auto file = services.load_buffer(TestImagePath);
	auto source = decode(file, file.size());
	
	services.trace("Converting image to %s...", as_string(target_format));
	auto converted_encoded = encode(source, file_format::dds, target_format);

	if (save_to_desktop)
	{
		path_t filename = TestImagePath.filename();
		filename.insert_basename_suffix(filename_suffix);
		filename.replace_extension(".dds");
		filesystem::save(filesystem::desktop_path() / filename, converted_encoded, converted_encoded.size());
	}

	// uncompress using the GPU since there's no code in ouro to decode BC formats
	auto decoded = decode(converted_encoded);

	auto tex = dev->new_texture("texture", decoded);

	auto cl = dev->immediate();
	cl->set_srvs(0, 1, &tex);
	cl->draw(3);
	auto snapshot = target->get_resource()->make_snapshot();
	services.check(snapshot, nth_test);
}

oTEST(oSurface_surface_bccodec)
{
	auto dev = gpu::new_device(gpu::device_init());
	dev->new_rso("root signature", gpu::root_signature_desc(1, &gpu::basic::linear_clamp, 0, nullptr, nullptr), 0);
	dev->new_pso("fullscreen tri", gpu::pipeline_state_desc(gpu::basic::VSfullscreen_tri, gpu::basic::PStex2d, mesh::basic::pos, gpu::basic::opaque, gpu::basic::front_face, gpu::basic::no_depth_stencil), 0);
	auto target = dev->new_rtv("target", 512, 512, surface::format::b8g8r8a8_unorm);

	auto cl = dev->immediate();
	cl->set_rso(0);
	cl->set_pso(0);
	cl->set_rtv(target);

	convert_and_test(srv, dev, target, format::bc1_unorm, "_BC1", 0);
	convert_and_test(srv, dev, target, format::bc3_unorm, "_BC3", 1);
	convert_and_test(srv, dev, target, format::bc7_unorm, "_BC7", 2);
	//convert_and_test(srv, dev, target, format::bc6h_uf16, "_BC6HU", 2);
}
