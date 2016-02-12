// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#pragma once
#include <oCore/countof.h>
#include <oString/path.h>
#include <oMath/primitive.h>
#include <oBase/unit_test.h>
#include <oGUI/window.h>

#include "gpu_pipeline_state.h"

#define oGPU_COMMON_TEST(_Name) oTEST(oGPU_##_Name) { gpu_test_##_Name t; t.run(srv); }

namespace ouro { namespace tests {

surface::image surface_load(const path_t& path, bool mips);

surface::image make_1D(int width, bool mips);

class first_tri
{
public:
	void initialize(gpu::device* dev)
	{
		dev_ = dev;

		auto first_tri = primitive::first_tri_mesh(primitive::tessellation_type::solid);
		i_ = dev_->new_ibv("indices", 3, first_tri.indices);
		v_ = dev_->new_vbv("vertices", sizeof(float3), 3, first_tri.positions);
	}

	void deinitialize()
	{
		dev_->del_ibv(i_);
		dev_->del_vbv(v_);
	}

	void draw(gpu::graphics_command_list* cl, uint num_instances = 1)
	{
		cl->set_indices(i_);
		cl->set_vertices(0, 1, &v_);
		cl->draw_indexed(i_.num_indices, num_instances);
	}

private:
	ref<gpu::device> dev_;
	gpu::ibv i_;
	gpu::vbv v_;
};

class test_cube
{
public:
	void initialize(gpu::device* dev, bool uvw = false);
	void deinitialize();

	void draw(gpu::graphics_command_list* cl, uint32_t num_instances = 1)
	{
		cl->set_indices(i);
		cl->set_vertices(0, 1, &v);
		cl->draw_indexed(i.num_indices, num_instances);
	}

private:
	ref<gpu::device> dev_;
	gpu::ibv i;
	gpu::vbv v;
};

class gpu_test
{
public:
	gpu_test(const char* title, bool interactive, const int* snapshot_frame_ids, size_t num_snapshot_frame_ids, const int2& resolution = int2(640, 480))
	{
		create(title, interactive, snapshot_frame_ids, num_snapshot_frame_ids, resolution);
	}
	
	template<size_t size>
	gpu_test(const char* title, bool interactive, const int (&snapshot_frame_ids)[size], const int2& resolution = int2(640, 480))
	{
		create(title, interactive, snapshot_frame_ids, size, resolution);
	}

	virtual ~gpu_test() {}

	virtual void initialize() {}
	virtual void deinitialize() {}

	virtual void render() = 0;

	void run(unit_test::services& services);

	uint32_t get_clear_color() const { return color::almost_black; }

private:
	void on_event(const window::basic_event& e);
	void create(const char* title, bool interactive, const int* snapshot_frame_ids, size_t num_snapshot_frame_ids, const int2& resolution);
	void recreate_dsv();
	void check_snapshot(unit_test::services& services);

	std::vector<uint32_t> snapshot_frames_;

	uint32_t nth_snapshot_;
	uint32_t frame_;
	bool running_;
	bool devmode_;
	bool all_snapshots_succeeded_;

protected:
	bool is_devmode() const { return devmode_; }
	uint32_t get_frame() const { return frame_; }

	std::shared_ptr<window> win_;
	ref<gpu::device> dev_;
	ref<gpu::rtv> primary_target_;
	ref<gpu::dsv> depth_dsv_;
};

class gpu_texture_test : public tests::gpu_test
{
public:
	gpu_texture_test(const char* title, bool interactive, const int2& resolution = int2(640, 480))
		: gpu_test(title, interactive, s_snapshot_frames, countof(s_snapshot_frames), resolution)
	{}

	virtual ref<gpu::srv> init(tests::pipeline_state* out_pipeline_state) = 0;
	
	virtual float rotation_step();

	void render() override;

protected:
	static const uint32_t linear_wrap = 0;
	static const int s_snapshot_frames[2];

	test_cube mesh_;

private:
	void initialize() override;
	void deinitialize() override;

	ref<gpu::srv> srv_;
	tests::pipeline_state state_;
	bool uvw_;
};

}}
