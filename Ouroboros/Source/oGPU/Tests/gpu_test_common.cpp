// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include "gpu_test_common.h"
#include <oSystem/filesystem.h>
#include <oMath/projection.h>
#include <oMath/matrix.h>

using namespace ouro::gpu;

namespace ouro { namespace tests {

surface::image surface_load(const path_t& path, bool mips)
{
	blob b = filesystem::load(path);
	auto sb = surface::decode(b, surface::format::unknown, mips ? surface::mip_layout::tight : surface::mip_layout::none);

	// force 4-channel format
	auto src_format = sb.info().format;
	auto with_alpha = surface::as_4chana(src_format);
	sb = sb.convert(with_alpha);

	if (mips)
		sb.generate_mips();
	return sb;
}

surface::image make_1D(int width, bool mips)
{
	surface::info_t si;
	si.semantic = surface::semantic::custom1d;
	si.dimensions = int3(width, 1, 1);
	si.mip_layout = mips ? surface::mip_layout::tight : surface::mip_layout::none;
	si.format = surface::format::b8g8r8a8_unorm;
	surface::image s(si);

	{
		surface::lock_guard lock(s);
		static const uint32_t sColors[] = { color::black, color::navy, color::green, color::teal, color::maroon, color::purple, color::olive, color::silver, color::gray, color::blue, color::lime, color::aqua, color::red, color::fuchsia, color::yellow, color::white };
		uint32_t* texture1Ddata = (uint32_t*)lock.mapped.data;
		for (uint i = 0; i < si.dimensions.x; i++)
			texture1Ddata[i] = sColors[i % countof(sColors)];
	}

	if (mips)
		s.generate_mips();

	return s;
}

void test_cube::initialize(gpu::device* dev, bool uvw)
{
	dev_ = dev;

	if (uvw)
	{
		struct vtx
		{
			float3 pos;
			float3 tex;
		};

		auto cube_info = primitive::cube_info(primitive::tessellation_type::solid);
		auto mem       = alloca(cube_info.total_bytes());
		auto vtx_mem   = default_allocator.scoped_allocate(sizeof(vtx) * cube_info.nvertices, "test_cube verts");
		vtx* verts     = (vtx*)vtx_mem;

		primitive::mesh_t cube_mesh(cube_info, mem);
		primitive::cube_tessellate(&cube_mesh, cube_info.type);

		const float3* src_pos = (const float3*)cube_mesh.positions;
			
		for (uint32_t ii = 0; ii < cube_info.nvertices; ii++)
		{
			verts[ii].pos = src_pos[ii];
			verts[ii].tex = src_pos[ii] * 0.5f + 0.5f;
		}

		i = dev_->new_ibv("test_cube index buffer", cube_info.nindices, cube_mesh.indices);
		v = dev_->new_vbv("test_cube vertex buffer", sizeof(vtx), cube_info.nvertices, verts);
	}

	else
	{
		struct vtx
		{
			float3 pos;
			float2 tex;
		};

		auto cube_info = primitive::cube_info(primitive::tessellation_type::textured);
		auto mem       = alloca(cube_info.total_bytes());
		auto vtx_mem   = default_allocator.scoped_allocate(sizeof(vtx) * cube_info.nvertices, "test_cube verts");
		vtx* verts     = (vtx*)vtx_mem;

		primitive::mesh_t cube_mesh(cube_info, mem);
		primitive::cube_tessellate(&cube_mesh, cube_info.type);

		uint16_t* idx  = (uint16_t*)(cube_mesh.indices);
		float3* pos    = (float3*)(cube_mesh.positions);
		float2* tex    = (float2*)(cube_mesh.texcoords);
		
		for (uint32_t ii = 0; ii < cube_info.nvertices; ii++)
		{
			verts[ii].pos = pos[ii];
			verts[ii].tex.x = tex[ii].x;
			verts[ii].tex.y = 1.0f - tex[ii].y;
		}

		i = dev_->new_ibv("test_cube index buffer", cube_info.nindices, idx);
		v = dev_->new_vbv("test_cube vertex buffer", sizeof(vtx), cube_info.nvertices, verts);
	}
}

void test_cube::deinitialize()
{
	if (dev_)
	{
		dev_->del_vbv(v);
		dev_->del_ibv(i);
	}
}

void gpu_test::create(const char* title, bool interactive, const int* snapshot_frame_ids, size_t num_snapshot_frame_ids, const int2& resolution)
{
	frame_ = 0;
	nth_snapshot_ = 0;
	running_ = true;
	devmode_ = interactive;
	all_snapshots_succeeded_ = true;

	if (snapshot_frame_ids && num_snapshot_frame_ids)
		snapshot_frames_.assign(snapshot_frame_ids, snapshot_frame_ids + num_snapshot_frame_ids);

	{
		window::init_t i;
		i.title = title;
		i.alt_f4_closes = true;
		i.on_event = std::bind(&gpu_test::on_event, this, std::placeholders::_1);
		i.shape.state = devmode_ ? window_state::restored : window_state::hidden;
		i.shape.style = window_style::sizable;
		i.shape.client_size = resolution;
		win_ = window::make(i);
	}

	{
		gpu::device_init i("gpu_test device");
		i.api_version = version_t(10,0);
		i.enable_driver_reporting = true;
		dev_ = new_device(i, win_.get());
	}

	primary_target_ = dev_->get_presentation_rtv();

	depth_dsv_ = nullptr;
	recreate_dsv();

	dev_->new_rso("root signature", get_root_signature_desc(), 0);
	
	for (int i = 0; i < (int)pipeline_state::count; i++)
		dev_->new_pso(as_string((pipeline_state)i), get_pipeline_state_desc((pipeline_state)i), i);

	dev_->immediate()->set_rso(0);
}

void gpu_test::recreate_dsv()
{
	depth_dsv_ = nullptr;
	if (primary_target_)
		depth_dsv_ = dev_->new_dsv("depth", primary_target_, surface::format::d24_unorm_s8_uint);
}

void gpu_test::check_snapshot(unit_test::services& services)
{
	if (snapshot_frames_.end() != find(snapshot_frames_.begin(), snapshot_frames_.end(), frame_))
	{
		surface::image snap = primary_target_->get_resource()->make_snapshot();
		services.check(snap, nth_snapshot_);
		nth_snapshot_++;
	}
}

void gpu_test::on_event(const window::basic_event& e)
{
	if (!dev_)
		return;

	switch (e.type)
	{
		case event_type::sizing:
		{
			dev_->on_window_resizing();
			break;
		}

		case event_type::sized:
		{
			dev_->on_window_resized();
			primary_target_ = dev_->get_presentation_rtv();
			recreate_dsv();
			break;
		}

		case event_type::closing:
			running_ = false;
			break;
		default:
			break;
	}
}

void gpu_test::run(unit_test::services& services)
{
	oASSERT(win_->is_window_thread(), "Run must be called from same thread that created the window");
	bool all_frames_succeeded = true;

	initialize();

	// Flush window init
	win_->flush_messages();

	while (running_ && (devmode_ || frame_ <= snapshot_frames_.back()))
	{
		win_->flush_messages();
		dev_->immediate()->clear_rtv(primary_target_, get_clear_color());
		dev_->immediate()->clear_dsv(depth_dsv_);
		render();
		dev_->flush(flush_type::immediate_and_sync);

		try
		{
			check_snapshot(services);
		}

		catch (std::exception&)
		{
			if (!devmode_)
			{
				deinitialize();
				std::rethrow_exception(std::current_exception());
			}
		}

		if (devmode_)
			dev_->present();
		frame_++;
	}

	deinitialize();

	if (!all_frames_succeeded)
		throw std::system_error(std::errc::protocol_error, std::system_category(), "Image compares failed, see debug output/log for specifics.");
}

const int gpu_texture_test::s_snapshot_frames[2] = { 0, 1 };

void gpu_texture_test::initialize()
{
	srv_ = init(&state_);
	auto desc = srv_->get_resource()->get_desc();
	uvw_ = desc.type == gpu::resource_type::texture3d || (desc.resource_bindings & gpu::resource_binding::texturecube);
	mesh_.initialize(dev_, uvw_);
}

void gpu_texture_test::deinitialize()
{
	mesh_.deinitialize();
}

float gpu_texture_test::rotation_step()
{
	static const float sCapture[] = 
	{
		774.0f,
		1036.0f,
	};

	return is_devmode() ? static_cast<float>(dev_->get_num_presents()) : sCapture[get_frame()];
}

void gpu_texture_test::render()
{
	auto* cl = dev_->immediate();
	cl->set_rtvs(1, &primary_target_, depth_dsv_);
	
	float4x4 V = lookat_lh(float3(0.0f, 0.0f, -4.5f), kZero3, kYAxis);

	float4x4 P = proj_fovy_lh(oDEFAULT_FOVY_RADIANSf, dev_->get_aspect_ratio(), 0.001f, 1000.0f);

	float rotationStep = rotation_step();
	float4x4 W = rotate(float3(radians(rotationStep) * 0.75f, radians(rotationStep), radians(rotationStep) * 0.5f));

	test_constants cb(W, V, P, color::black);
	cl->set_cbv(oGPU_TEST_CB_CONSTANTS_SLOT, &cb, sizeof(cb));
	cl->set_srvs(oGPU_TEST_SRV_TEXTURE_SLOT, 1, &srv_);
	cl->set_pso(state_);

	mesh_.draw(cl);
}

}}
