// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Buffers used for various render passes sized proportionately to the main 
// presentation buffer.

#pragma once
#include <oMath/pov.h>
#include <oGPU/gpu.h>
#include <oGfx/film.h>
#include <oGfx/gpu_signature.h>
#include <oGfx/scene.h>
#include <oGfx/model_registry.h>
#include <oGfx/texture_registries.h>
#include <oCore/flexible_array.h>

namespace ouro { class window; namespace gfx {

enum class fullscreen_mode
{
	lit,
	points,
	wireframe,
	texcoord,
	texcoordu,
	texcoordv,
	bittangentx,
	bittangenty,
	bittangentz,
	bittangent,
	tangentx,
	tangenty,
	tangentz,
	tangent,
	normalx,
	normaly,
	normalz,
	normal,
	count,
};

struct render_settings_t
{
	fullscreen_mode mode;
};

enum class render_pass : uint8_t
{
	initialize,
	geometry,
	depth_resolve,
	shadow,
	tonemap,
	debug,
	resolve,
	count,
};

enum class render_technique : uint8_t
{
	pass_begin, // 
	view_begin, // param nullptr, submit only one

	// 'begin' techniques must be before others to sort correctly

	linearize_depth, // param nullptr, submit only one

	draw_lines,      // param is lines_submission_t
	draw_prim,       // param is primitive_submission_t
	draw_subset,     // param is model_subset_submission_t
	draw_axis,       // param is nullptr, 1 per frame
	draw_gizmo,      // param is gizmo::tessellation_info_t, 1 per frame
	draw_grid,       // param is grid_submission_t

	debug_draw_normals,  // param is debug_model_normal_submission_t
	debug_draw_tangents, // param is ?

	// 'end' techniques must be after others to sort correctly

	view_end,   // param nullptr, submit only one
	pass_end,   //
	count,
};

struct geometry_view_t
{
	gpu::ibv indices;
	gpu::vbv vertices[3]; // [0] PNT, [1] UV0, [2] Optional color or NULL
	uint32_t start_index;
	uint32_t num_indices;
};

struct render_line_t
{
	float3 p0;
	float3 p1;
	uint32_t argb;
};

struct debug_model_normals_submission_t
{
	float4x4 world;
	gpu::vbv vertices;
	uint32_t start_vertex;
	uint32_t num_vertices;
	uint32_t argb;
	float    scale;
};

struct debug_model_tangents_submission_t
{
	float4x4 world;
	gpu::vbv vertices;
	uint32_t start_vertex;
	uint32_t num_vertices;
	uint32_t pos_argb;
	uint32_t neg_argb;
	float    scale;
};

struct lines_submission_t
{
	render_line_t* lines; // allocate using renderer
	uint32_t nlines;
};

struct primitive_submission_t
{
	float4x4 world;
	float4 color;
	gpu::srv* texture;
	primitive_model type;
};

struct model_subset_submission_t
{
	float4x4            world;
	uint64_t            mat_hash;
	gfx::pipeline_state state;
	uint8_t             num_srvs;
	uint8_t             num_constants;
	uint8_t             pada;
	uint32_t            padb;
	geometry_view_t     geometry;
};

struct grid_submission_t
{
	float total_width;
	float grid_width;
	uint32_t argb;
};

struct renderer_init_t
{
};

class renderer_t
{
public:
	renderer_t() {}
	~renderer_t() {}

	void initialize(const renderer_init_t& init, window* win);
	void deinitialize();

	void flush(uint32_t max_operations = ~0u);

	void on_window_resizing();
	void on_window_resized(uint32_t new_width, uint32_t new_height);

	// == Submission ==	

	// All submission calls should occur between these two calls from a single thread.
	// allocate() and submit() can be called concurrently.
	void begin_submit();
	void end_submit();

	// All submissions between these pairs will result in an offscreen or main render.
	void begin_view(const pov_t* pov, const render_settings_t* render_settings);
	void end_view(/*offscreen to resolve to or display target*/);

	// Allocates submission memory, mainly passed as data to submit(). Default alignment is 16-bytes.
  void* allocate(uint32_t bytes, uint32_t alignment);
	inline void* allocate(uint32_t bytes) { return allocate(bytes, 16); }
	template<typename T> T* allocate(uint32_t n = 1) { return (T*)allocate(sizeof(T) * n, 16); }

	// Submits a renderer command, which requires a 48-bit priority (upper 16-bits reserved)
	// that will be used to sort all submissions, a pass where all things execute in pass 
	// order (master sort key) and a technique that indicates what function will handle the
	// data.
	template<typename pass_enum, typename technique_enum>
	void submit(uint64_t priority, const pass_enum& pass, const technique_enum& technique, void* data) { internal_submit(priority, (uint8_t)pass, (uint8_t)technique, data); }


	// == API that should go away ==

	gpu::device* dev() { return dev_; }
	film_t* film() { return &film_; }

	model_registry* get_model_registry() { return &models_; }
	texture2d_registry* get_texture2d_registry() { return &texture2ds_; }

private:
// _____________________________________________________________________________
// Scheduling

	void internal_submit(uint64_t priority, uint8_t pass, uint8_t technique, void* data);
	void* consolidate_master_tasklist(uint32_t* out_num_tasks);
	void trace_master_tasklist(const void* master_tasks, uint32_t num_tasks);
	void kick(void* tasks, uint32_t num_tasks);

	ref<gpu::device> dev_;
	const pov_t* pov_;
	const render_settings_t* render_settings_;
	film_t film_;
	model_registry models_;
	texture2d_registry texture2ds_;
	uint32_t frame_id_;

	flexible_array_t<void*, true>* global_heaps_;
	flexible_array_t<void*, true>* global_tasklists_;

	bool submission_overflow_;

	// threadlocal support for submit().
	static thread_local uint32_t local_tasklist_frame_id_;
	static thread_local void*    local_tasklist_;

	// threadlocal support for allocate().
	static thread_local uint32_t local_heap_frame_id_;
	static thread_local void* local_heap_;
	static thread_local void* local_heap_end_;

// _____________________________________________________________________________
// Terrain

	std::array<gpu::ibv, 92> btt_patch_indices_;
	gpu::vbv btt_verts_;
};
	
}}
