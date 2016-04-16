// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// D3D-style API for working with modern GPUs

#pragma once
#include <oArch/arch.h>
#include <oMath/quantize.h>
#include <oBase/gpu_api.h>
#include <oBase/vendor.h>
#include <oCore/version.h>
#include <oCore/ref.h>
#include <oMemory/sbb_allocator.h>
#include <oMesh/element.h>
#include <oString/fixed_string.h>
#include <oSurface/format.h>
#include <oSurface/image.h>
#include <array>
#include <atomic>

typedef unsigned char BYTE; // because HLSL compiler generates BYTE arrays without defining it

namespace ouro { class window; namespace gpu {

enum class blend_op         : uint8_t { add, subtract, rev_subtract, min_, max_, };
enum class blend_type       : uint8_t { zero, one, src_color, inv_src_color, src_alpha, inv_src_alpha, dest_alpha, inv_dest_alpha, dest_color, inv_dest_color, src_alpha_sat, blend_factor, inv_blend_factor, src1_color, inv_src1_color, src1_alpha, inv_src1_alpha, };
enum class clear_type       : uint8_t { depth, stencil, depth_stencil, };
enum class comparison       : uint8_t { never, less, equal, less_equal, greater, not_equal, greater_equal, always, };
enum class cull_mode        : uint8_t { none, front, back, };
enum class filter_type      : uint8_t { point, linear, anisotropic, comparison_point, comparison_linear, comparison_anisotropic, };
enum class flush_type       : uint8_t { periodic, immediate, immediate_and_sync, };
enum class map_type         : uint8_t { discard, no_overwrite, };
enum class logic_op         : uint8_t { clear, set, copy, copy_inverted, noop, invert, and, nand, or, nor, xor, equiv, and_reverse, and_inverted, or_reverse, or_inverted, };
enum class primitive_type   : uint8_t { undefined, point, line, triangle, patch };
enum class stencil_op       : uint8_t { keep, zero, replace, incr_sat, decr_sat, invert, incr, decr, };
enum class resource_usage   : uint8_t { default, upload, readback, immutable };
enum class texture_layout   : uint8_t { unknown, linear, undefined_swizzle, standard_swizzle, };
enum class resource_type    : uint8_t { unknown, buffer, texture1d, texture2d,                                                                texture3d,                                };
enum class rtv_dimension    : uint8_t { unknown, buffer, texture1d, texture1darray, texture2d, texture2darray, texture2dms, texture2dmsarray, texture3d,                                };
enum class dsv_dimension    : uint8_t { unknown,         texture1d, texture1darray, texture2d, texture2darray, texture2dms, texture2dmsarray,                                           };
enum class uav_dimension    : uint8_t { unknown, buffer, texture1d, texture1darray, texture2d, texture2darray,                                texture3d,                                };
enum class srv_dimension    : uint8_t { unknown, buffer, texture1d, texture1darray, texture2d, texture2darray, texture2dms, texture2dmsarray, texture3d, texturecube, texturecubearray, };
enum class uav_extension    : uint8_t { none, raw, append, counter, };
enum class present_mode     : uint8_t { headless, windowed, fullscreen_cooperative, fullscreen_exclusive, };
namespace resource_binding  { enum flag : uint16_t { render_target = 1, depth_stencil = 2, unordered_access = 4, not_shader_resource = 8, cross_adapter = 16, simultaneous_access = 32, constant = 64, structured = 128, texturecube = 256, };}
namespace stage_binding     { enum flag : uint8_t  { vertex = 1, hull = 2, domain = 4, geometry = 8, pixel = 16, compute = 32, graphics = vertex|hull|domain|geometry|pixel, tessellated = vertex|hull|domain|pixel, simple_graphics = vertex|pixel, all = vertex|hull|domain|geometry|pixel|compute };}
namespace dsv_flag          { enum flag : uint8_t  { none, read_only_depth = 1, read_only_stencil = 2, };}
namespace color_write_mask  { enum flag : uint8_t  { red = 1, green = 2, blue = 4, alpha = 8, all = (red|green|blue|alpha), };}

struct buffer_rtv           { uint32_t first_element; uint32_t num_elements;                                                                                                          };
struct buffer_uav           { uint32_t first_element; uint32_t num_elements; uint32_t structure_byte_stride; uint32_t counter_offset_in_bytes; uav_extension extension;               };
struct buffer_srv           { uint32_t first_element; uint32_t num_elements; uint32_t structure_byte_stride;                                                                          };
struct tex1d_view           { uint32_t mip_slice;                                                                                                                                     };
struct tex1d_array_view     { uint32_t mip_slice;         uint32_t first_array_slice;                        uint32_t array_size;                                                     };
struct tex2d_dsv            { uint32_t mip_slice;                                                                                                                                     };
struct tex2d_rtv            { uint32_t mip_slice;                                                                                 uint32_t plane_slice;                               };
struct tex2d_array_rtv      { uint32_t mip_slice;         uint32_t first_array_slice;                        uint32_t array_size; uint32_t plane_slice;                               };
struct tex2d_array_dsv      { uint32_t mip_slice;         uint32_t first_array_slice;                        uint32_t array_size;                                                     };
struct tex2dms_array_view   {                             uint32_t first_array_slice;                        uint32_t array_size;                                                     };
struct tex3d_rtv            { uint32_t mip_slice;         uint32_t first_wslice;                             uint32_t wsize;                                                          };
struct tex2dms_view         { uint32_t unused;                                                                                                                                        };
struct tex1d_array_srv      { uint32_t most_detailed_mip; uint32_t mip_levels; uint32_t first_array_slice;   uint32_t array_size;                       float resource_min_lod_clamp; };
struct tex2d_array_srv      { uint32_t most_detailed_mip; uint32_t mip_levels; uint32_t first_array_slice;   uint32_t array_size; uint32_t plane_slice; float resource_min_lod_clamp; };
struct texcube_array_srv    { uint32_t most_detailed_mip; uint32_t mip_levels; uint32_t first_2d_array_face; uint32_t num_cubes;                        float resource_min_lod_clamp; };
struct tex1d_srv            { uint32_t most_detailed_mip; uint32_t mip_levels;                                                                          float resource_min_lod_clamp; };
struct tex2d_srv            { uint32_t most_detailed_mip; uint32_t mip_levels;                                                    uint32_t plane_slice; float resource_min_lod_clamp; };
struct tex3d_srv            { uint32_t most_detailed_mip; uint32_t mip_levels;                                                                          float resource_min_lod_clamp; };
struct texcube_srv          { uint32_t most_detailed_mip; uint32_t mip_levels;                                                                          float resource_min_lod_clamp; };

struct memcpy_dest          {       void* data; uint32_t row_pitch; uint32_t slice_pitch; };
struct memcpy_src           { const void* data; uint32_t row_pitch; uint32_t slice_pitch; };
struct copy_box             { uint32_t left; uint32_t top; uint32_t front; uint32_t right; uint32_t bottom; uint32_t back; };

struct device_init
{
	device_init(const char* name = "gpu device")
		: name(name)
		, api_version(version_t(11,0))
		, min_driver_version(version_t())
		, adapter_index(0)
		, virtual_desktop_x(0)
		, virtual_desktop_y(0)
		, use_software_emulation(false)
		, use_exact_driver_version(false)
		, multithreaded(true)
		, enable_driver_reporting(false)
		, max_persistent_mesh_bytes(256 * 1024 * 1024)
		, max_transient_mesh_bytes(64 * 1024 * 1024)
		, max_rsos(16)
		, max_psos(128)
		, max_csos(128)
	{}

	sstring name;                                // name associated with this device in debug output
	version_t api_version;                       // specific version of api features   
	version_t min_driver_version;                // if major & minor are 0, an internal QA-verified version is checked.
	int adapter_index;                           // use nth adapter for the device or if < 0, discover 
	int virtual_desktop_x;                       // the adapter from the virtual desktop coords
	int virtual_desktop_y;                       // 
	bool use_software_emulation;                 // if false a create without available HW will fail
	bool use_exact_driver_version;               // if true the current driver version must be exactly min_driver_version
	bool multithreaded;                          // if true the device is thread-safe
	bool enable_driver_reporting;                // control if driver warnings/errors are reported.
	uint32_t max_persistent_mesh_bytes;          // capacity in bytes for all persistent mesh data (new_ibv and new_vbv)
	uint32_t max_transient_mesh_bytes;           // capacity in bytes for all transient mesh data (new_transient_indices, new_transient_vertices)
	uint16_t max_rsos;                           // maximum number of root signature objects that will be created
	uint16_t max_psos;                           // maximum number of graphics pipeline state objects that will be created
	uint16_t max_csos;                           // maximum number of compute pipeline state objects that will be created
};

struct device_desc
{
	sstring name;                                // name associated with this device in debug output
	mstring device_description;                  // description as provided by the device vendor
	mstring driver_description;                  // description as provided by the driver vendor
	uint64_t native_memory;                      // bytes present on the device (AKA VRAM)
	uint64_t dedicated_system_memory;            // bytes reserved by the system to accommodate data transfer to the device
	uint64_t shared_system_memory;               // bytes reserved in system memory used instead of a separate bank of native_memory
	version_t driver_version;                    // driver version used by the created device
	version_t feature_version;                   // feature level supported by this device
	int adapter_index;                           // index of adapter used to create the device
	gpu_api api;                                 // describe which implementation API is used
	vendor vendor;                               // vendor of the adapter
	bool is_software_emulation;                  // true if not a HW implementation
	bool driver_reporting_enabled;               // true if device was initialized with debug reporting enabled
	bool profiler_attached;                      // true if PIX or some other profiler is running
};

struct viewport_desc
{
	float top_left_x; float top_left_y;
	float width;      float height;
	float min_depth;  float max_depth;
};

struct rt_blend_desc
{
	bool blend_enable;
	bool logic_op_enable;
	blend_type src_blend_rgb;
	blend_type dest_blend_rgb;
	blend_op blend_op_rgb;
	blend_type src_blend_alpha;
	blend_type dest_blend_alpha;
	blend_op blend_op_alpha;
	logic_op logic_op;
	uint8_t render_target_write_mask;
};

struct blend_desc
{
	bool alpha_to_coverage_enable;
	bool independent_blend_enable;
	rt_blend_desc render_targets[8];
};

struct rasterizer_desc
{
	bool wireframe;
	cull_mode cull_mode;
	bool front_ccw;
	bool scissor_enable;
	int depth_bias;
	float depth_bias_clamp;
	float slope_scaled_depth_bias;
	bool depth_clip_enable;
	bool multisample_enable;
	bool antialiased_line_enabled;
	bool conservative_rasterization;
	uint32_t forced_sample_count;
};

struct depth_stencil_desc
{
	bool depth_enable;
	bool depth_write;
	comparison depth_func;
	bool stencil_enable;
	uint8_t stencil_mask_read;
	uint8_t stencil_mask_write;

	stencil_op stencil_front_face_fail_op;
	stencil_op stencil_front_face_depth_fail_op;
	stencil_op stencil_front_face_pass_op;
	comparison stencil_front_face_func;

	stencil_op stencil_back_face_fail_op;
	stencil_op stencil_back_face_depth_fail_op;
	stencil_op stencil_back_face_pass_op;
	comparison stencil_back_face_func;
};

struct sampler_desc
{
	filter_type filter;
	bool clamp; // false implies wrap
	comparison comparison_func;
	uint8_t max_anisotropy;
	float border_color[4];
	float mip_bias;
	float min_lod;
	float max_lod;
};

struct root_signature_desc
{
	root_signature_desc()
		: num_samplers(0)
		, samplers(nullptr)
		, num_cbvs(0)
		, struct_strides(nullptr)
		, max_num_structs(nullptr)
	{}

	root_signature_desc(uint32_t _num_samplers, const sampler_desc* _samplers, uint32_t _num_cbvs, const uint32_t* _struct_strides, const uint32_t* _max_num_structs)
		: num_samplers(_num_samplers)
		, samplers(_samplers)
		, num_cbvs(_num_cbvs)
		, struct_strides(_struct_strides)
		, max_num_structs(_max_num_structs)
	{}

	template<uint32_t NumSamplers, uint32_t NumCBVs>
	root_signature_desc(const sampler_desc (&_samplers)[NumSamplers], const uint32_t (&_struct_strides)[NumCBVs], const uint32_t (&_max_num_structs)[NumCBVs])
		: num_samplers(NumSamplers)
		, samplers(_samplers)
		, num_cbvs(NumCBVs)
		, struct_strides(_struct_strides)
		, max_num_structs(_max_num_structs)
	{}

	uint32_t num_samplers;
	const sampler_desc* samplers;

	uint32_t num_cbvs;
	const uint32_t* struct_strides;
	const uint32_t* max_num_structs;
};

struct compute_state_desc
{
	const void* cs_bytecode;
	uint32_t node_mask;
	uint32_t flags;
};

struct pipeline_state_desc
{
	pipeline_state_desc()
		: vs_bytecode(nullptr), hs_bytecode(nullptr), ds_bytecode(nullptr), gs_bytecode(nullptr), ps_bytecode(nullptr)
		, num_input_elements(1), input_elements(nullptr)
		, sample_mask(~0u)
		, num_render_targets(0), dsv_format(surface::format::unknown)
		, primitive_type(primitive_type::undefined)
		, sample_count(1), sample_quality(0), node_mask(~0u), flags(0)
	{ rtv_formats.fill(surface::format::unknown); }

	template<size_t num_elements>
	pipeline_state_desc(const void* vs, const void* ps, const mesh::element_t (&input)[num_elements], const blend_desc& blend, const rasterizer_desc& rasterizer, const depth_stencil_desc& depth_stencil, const gpu::primitive_type& prim = gpu::primitive_type::triangle)
		: vs_bytecode(vs), hs_bytecode(nullptr), ds_bytecode(nullptr), gs_bytecode(nullptr), ps_bytecode(ps)
		, num_input_elements(num_elements), input_elements(input), blend_state(blend), rasterizer_state(rasterizer), depth_stencil_state(depth_stencil)
		, sample_mask(~0u)
		, num_render_targets(0), dsv_format(surface::format::unknown)
		, primitive_type(prim)
		, sample_count(1), sample_quality(0), node_mask(~0u), flags(0)
	{ rtv_formats.fill(surface::format::unknown); }

	const void* vs_bytecode;
	const void* hs_bytecode;
	const void* ds_bytecode;
	const void* gs_bytecode;
	const void* ps_bytecode;

	uint32_t num_input_elements;
	const mesh::element_t* input_elements;

	blend_desc blend_state;
	rasterizer_desc rasterizer_state;
	depth_stencil_desc depth_stencil_state;

	uint32_t sample_mask;
	uint32_t num_render_targets;
	std::array<surface::format, 8> rtv_formats;
	surface::format dsv_format;
	primitive_type primitive_type;

	uint32_t sample_count;
	uint32_t sample_quality;
	uint32_t node_mask;
	uint32_t flags;
};

struct resource_desc
{
	uint32_t width;
	uint32_t height;
	uint16_t depth_or_array_size;
	uint16_t mip_levels;
	uint16_t alignment;
	uint8_t sample_count;
	uint8_t sample_quality;
	resource_type type;
	resource_usage usage;
	surface::format format;
	texture_layout layout;
	uint16_t resource_bindings;
};

struct rtv_desc
{
	surface::format format;
	rtv_dimension dimension;
	union
	{
		buffer_rtv buffer;
		tex1d_view texture1d;
		tex1d_array_view texture1darray;
		tex2d_rtv texture2d;
		tex2d_array_rtv texture2darray;
		tex2dms_view texture2dms;
		tex2dms_array_view texture2dmsarray;
		tex3d_rtv texture3d;
	};
};

struct dsv_desc
{
	surface::format format;
	dsv_dimension dimension;
	uint32_t dsv_flags;
	union
	{
		tex1d_view texture1d;
		tex1d_array_view texture1darray;
		tex2d_dsv texture2d;
		tex2d_array_dsv texture2darray;
		tex2dms_view texture2dms;
		tex2dms_array_view texture2dmsarray;
	};
};

struct uav_desc
{
	surface::format format;
	uav_dimension dimension;
	union
	{
		buffer_uav buffer;
		tex1d_view texture1d;
		tex1d_array_view texture1darray;
		tex2d_rtv texture2d;
		tex2d_array_rtv texture2darray;
		tex3d_rtv texture3d;
	};
};

struct srv_desc
{
	surface::format format;
	srv_dimension dimension;
	uint32_t shader4_component_mapping;
	union
	{
		buffer_srv buffer;
		tex1d_srv texture1d;
		tex1d_array_srv texture1darray;
		tex2d_srv texture2d;
		tex2d_array_srv texture2darray;
		tex2dms_view texture2dms;
		tex2dms_array_view texture2dmsarray;
		tex3d_srv texture3d;
		texcube_srv texturecube;
		texcube_array_srv texturecubearray;
	};
};

struct stats_desc
{
	uint32_t num_input_vertices;
	uint32_t num_input_primitives;
	uint32_t num_gs_output_primitives;
	uint32_t num_pre_clip_rasterizer_primitives;
	uint32_t num_post_clip_rasterizer_primitives;
	uint32_t num_vs_calls;
	uint32_t num_hs_calls;
	uint32_t num_ds_calls;
	uint32_t num_gs_calls;
	uint32_t num_ps_calls;
	uint32_t num_cs_calls;
};

struct device_child
{
	const char* name(char* dst, size_t dst_size) const;
	template<size_t size> const char* name(char (&dst)[size]) const { return name(dst, size); }
};

struct resource : device_child
{
	void reference();
	void release();

	resource_desc get_desc() const;

	// readback resources only
	bool copy_to(void* dst, uint32_t row_pitch = 0, uint32_t depth_pitch = 0, bool flip = false, bool blocking = true) const;

	// texture2d's only, call only once the resource has been resolved (not during rendering)
	surface::image make_snapshot(const allocator& a = default_allocator);
};

struct rso             : device_child {                                                 void reference(); void release(); }; // root signature object
struct pso             : device_child {                                                 void reference(); void release(); }; // graphics pipeline state object
struct cso             : device_child {                                                 void reference(); void release(); }; // compute pipeline state object
struct sao             : device_child {                                                 void reference(); void release(); }; // sampler object
struct timer           : device_child { double get_time(bool blocking = true);          void reference(); void release(); }; // returns -1 if not ready
struct fence           : device_child { bool finished(bool blocking = true);            void reference(); void release(); }; // returns false if not ready
struct occlusion_query : device_child { uint32_t get_pixel_count(bool blocking = true); void reference(); void release(); }; // returns 0xffffffff if not ready
struct stats_query     : device_child { stats_desc get_stats(bool blocking = true);     void reference(); void release(); }; // values have 0xffffffff if not ready
struct view            : device_child { resource* get_resource() const;                 void reference(); void release(); }; // base class for view
struct rtv             : view         { rtv_desc get_desc() const;                                                        }; // render target view
struct dsv             : view         { dsv_desc get_desc() const;                                                        }; // depth stencil view
struct uav             : view         { uav_desc get_desc() const;                                                        }; // unordered access view
struct srv             : view         { srv_desc get_desc() const;                                                        }; // shader resource view

struct ibv
{
	ibv() : offset(0), num_indices(0), transient(false), is_32bit(false) {}
	ibv(uint32_t offset, uint32_t num_indices, bool transient = false, bool is_32bit = false) : offset(offset), num_indices(num_indices), transient(transient), is_32bit(is_32bit) {}

	uint32_t offset;           // bytes from index buffer base
	uint32_t num_indices : 30; //
	uint32_t transient   : 1;  // else persistent
	uint32_t is_32bit    : 1;  // else is 16-bit
};

struct vbv
{
	vbv() : offset(0), num_vertices(0), transient(false), vertex_stride_uints_minus_1(0) {}
	vbv(uint32_t offset, uint32_t num_vertices, uint32_t stride_bytes, bool transient = false) : offset(offset), num_vertices(num_vertices), transient(transient) { vertex_stride_bytes(stride_bytes); }

	uint32_t offset;                            // bytes from vertex buffer base
	uint32_t num_vertices                 : 26; //
	uint32_t transient                    : 1;  // else persistent
	uint32_t vertex_stride_uints_minus_1  : 5;  // (sizeof(vertex) / 4) - 1 to make the most of the bit space
	inline void vertex_stride_bytes(uint32_t stride_bytes) { vertex_stride_uints_minus_1 = (stride_bytes / sizeof(uint32_t)) - 1; }
	inline uint32_t vertex_stride_bytes() const { return ((vertex_stride_uints_minus_1 + 1) & ((1<<5)-1)) * sizeof(uint32_t); }
};

class graphics_command_list : public device_child
{
public:
	// Lifetime
	void reference();
	void release();

	// Debugging
	void set_marker(const char* marker);
	void push_marker(const char* marker);
	void pop_marker();

	// Query
	void begin_timer(timer* t);
	void end_timer(timer* t);
	void insert_fence(fence* f);
	void begin_occlusion_query(occlusion_query* q);
  void end_occlusion_query(occlusion_query* q);

	// Clear
	void clear_dsv(dsv* view, const clear_type& clear_type = clear_type::depth_stencil, float depth = 1.0f, uint8_t stencil = 0);
	void clear_rtv(rtv* view, const float color_rgba[4]);
	void clear_rtv(rtv* view, uint32_t argb) { float4 c = truetofloat4(argb); float a[4]; a[0] = c.x; a[1] = c.y; a[2] = c.z; a[3] = c.w; clear_rtv(view, a); }
	void clear_uav_float(uav* view, const float values[4]);
	void clear_uav_uint(uav* view, const uint32_t values[4]);

	// Transient Mesh Allocation (write to the returned pointer and commit to retrieve a view valid only for this frame)

	void* new_transient_indices(uint32_t num_indices, bool is_32bit = false);
	ibv commit_transient_indices();

	void* new_transient_vertices(uint32_t vertex_stride, uint32_t num_vertices);
	template<typename vertexT> vertexT* new_transient_vertices(uint32_t num_vertices) { return (vertexT*)new_transient_vertices(sizeof(vertexT), num_vertices); }
	vbv commit_transient_vertices();

	// Dispatch
	void dispatch(uint32_t thread_group_x, uint32_t thread_group_y, uint32_t thread_group_z);
	void dispatch_indirect(resource* args_buffer, uint32_t args_offset);
	void draw_indexed(uint32_t num_indices_per_instance, uint32_t num_instances = 1, uint32_t start_index_location = 0, int32_t base_vertex_location = 0, uint32_t start_instance_location = 0);
	void draw(uint32_t num_vertices, uint32_t num_instances = 1, uint32_t start_vertex_location = 0, uint32_t start_instance_location = 0);
	void draw_indirect(resource* args_buffer, uint32_t args_offset);
	
	// Output
	
	// Set graphics pipeline (pso) render targets and unordered access targets
	void set_rtvs(uint32_t num_rtvs, rtv* const* rtvs, dsv* dsv = nullptr, uint32_t num_viewports = 0, const viewport_desc* viewports = nullptr, uint32_t uav_start = 0, uint32_t num_uavs = 0, uav* const* uavs = nullptr, const uint32_t* initial_counts = nullptr);
	void set_rtv(rtv* target, dsv* depth = nullptr) { set_rtvs(1, &target, depth); }
	
	// Set unordered access targets on graphics compute pipeline (cso)
	void set_uavs(uint32_t uav_start, uint32_t num_uavs, uav* const* uavs, const uint32_t* initial_counts = nullptr);

	// State
	void reset(rso* root_signature = nullptr, pso* pipeline_state = nullptr);
	void set_rso(rso* o, uint32_t stage_bindings = stage_binding::graphics);
	void set_rso(uint32_t rso_index, uint32_t stage_bindings = stage_binding::graphics); template<typename enum_class> void set_rso(const enum_class& state, uint32_t stage_bindings = stage_binding::graphics) { set_rso((uint32_t)state, stage_bindings); }
	void set_pso(pso* o);
	void set_pso(uint32_t pso_index); template<typename enum_class> void set_pso(const enum_class& state) { set_pso((uint32_t)state); }
	void set_cso(cso* o);
	void set_cso(uint32_t cso_index); template<typename enum_class> void set_cso(const enum_class& state) { set_cso((uint32_t)state); }
	void set_cbv(uint32_t cbv_index, const void* src, uint32_t src_size, uint32_t stage_bindings = stage_binding::graphics);
	void set_srvs(uint32_t srv_start, uint32_t num_srvs, const srv* const* srvs, uint32_t stage_bindings = stage_binding::graphics);
	void set_blend_factor(float blend_factor[4]);
	void set_stencil_ref(uint32_t stencil_ref);
	void set_indices(const ibv& view);
	void set_vertices(uint32_t vbv_start, uint32_t num_vbvs, const vbv* vbvs);

	// Direct memory operations
	bool map(memcpy_dest* mapped, resource* r, uint32_t subresource = 0, map_type type = map_type::discard);
	void unmap(resource* r, uint32_t subresource = 0);
	void update(resource* r, uint32_t subresource, const copy_box* box, const void* src, uint32_t src_row_pitch, uint32_t src_depth_pitch);
	
	void copy(resource* dest, resource* src);
	void copy_texture_region(resource* dest, uint32_t dest_subresource, uint32_t dest_x, uint32_t dest_y, uint32_t dest_z, resource* src, uint32_t src_subresource, const copy_box* src_box = nullptr);
	void copy_structure_count(resource* dest, uint32_t dest_offset_uints, uav* src);

	void generate_mips(srv* view);
};

class device
{
public:
	// Basics
	void reference();
	void release();
	device_desc get_desc() const;

	
	// Runtime
	void begin_frame();
	void end_frame();
	void reset();
	void execute(uint32_t num_command_lists, graphics_command_list* const* command_lists);
	void flush(const flush_type& type = flush_type::immediate);
	graphics_command_list* immediate() { return imm_; }

	// Display
	void on_window_resizing();
	void on_window_resized();
	void present(uint32_t interval = 1);
	present_mode get_present_mode();
	void set_present_mode(const present_mode& mode);
	uint32_t get_num_presents() const { return npresents_; }

	srv* get_presentation_srv() const { return (srv*)swp_srv_.c_ptr(); }
	rtv* get_presentation_rtv() const { return (rtv*)swp_rtv_.c_ptr(); }
	uav* get_presentation_uav() const { return (uav*)swp_uav_.c_ptr(); }
	float get_aspect_ratio() const { auto desc = get_presentation_rtv()->get_resource()->get_desc(); return desc.width / static_cast<float>(desc.height); }

	// State Objects
	// if using the non-index version, then objects should be discarded when use is complete
	// if using the indexed api, the device will manage the lifetime

	ref<rso> new_rso(const char* name, const root_signature_desc& desc);
	ref<pso> new_pso(const char* name, const pipeline_state_desc& desc);
	ref<cso> new_cso(const char* name, const compute_state_desc& desc);

	void new_rso(const char* name, const root_signature_desc& desc, uint32_t rso_index);
	void new_pso(const char* name, const pipeline_state_desc& desc, uint32_t pso_index);
	void new_cso(const char* name, const compute_state_desc& desc, uint32_t cso_index);

	template<typename enumT> void new_rso(const char* name, const root_signature_desc& desc, const enumT& rso_index) { new_rso(name, desc, (uint32_t)rso_index); }
	template<typename enumT> void new_pso(const char* name, const pipeline_state_desc& desc, const enumT& pso_index) { new_pso(name, desc, (uint32_t)pso_index); }
	template<typename enumT> void new_cso(const char* name, const compute_state_desc& desc, const enumT& cso_index) { new_cso(name, desc, (uint32_t)cso_index); }

	template<typename enumT>
	void new_rsos(const root_signature_desc* descs)
	{
		for (uint32_t i = 0; i < (uint32_t)enumT::count; i++)
			new_rso(as_string((const enumT&)i), descs[i], i);
	}

	template<typename enumT>
	void new_psos(const pipeline_state_desc* descs)
	{
		for (uint32_t i = 0; i < (uint32_t)enumT::count; i++)
			new_pso(as_string((const enumT&)i), descs[i], i);
	}

	// Resources
	ref<graphics_command_list> new_graphics_command_list(const char* name);

	ref<resource> new_resource(const char* name, const resource_desc& desc, memcpy_src* subresource_sources = nullptr);

	// creates a simple constant buffer
	ref<resource> new_cb(const char* name, uint32_t struct_stride, uint32_t num_structs = 1);

	// creates a simple readback buffer
	ref<resource> new_rb(const char* name, uint32_t struct_stride, uint32_t num_structs = 1);

	// creates a readback buffer matching src
	ref<resource> new_rb(resource* src, bool immediate_copy = false);

	// returns a raw or structured buffer unordered access view with an optional srv
	ref<uav> new_uav(const char* name, uint32_t struct_stride, uint32_t num_structs, const uav_extension& ext, srv** out_srv = nullptr);

	// creates an immutable texture with data from src. The resource should not be destroyed because
	// it is managed solely by the srv.
	ref<srv> new_texture(const char* name, const surface::image& src);

	// Views
	ibv new_ibv(const char* name, uint32_t num_indices, const void* index_data, bool is_32bit = false);
	void del_ibv(const ibv& view);

	vbv new_vbv(const char* name, uint32_t vertex_stride, uint32_t num_vertices, const void* vertex_data);
	void del_vbv(const vbv& view);

	// if an ibv or vbv's contents needs to be read, use this base in conjunction with the view's offset
	// to access the data.
	const void* readable_mesh_base() const { return persistent_mesh_alloc_.base(); }

	ref<rtv> new_rtv(resource* r, const rtv_desc& desc);
	ref<dsv> new_dsv(resource* r, const dsv_desc& desc);
	ref<uav> new_uav(resource* r, const uav_desc& desc);
	ref<srv> new_srv(resource* r, const srv_desc& desc);

	// View the resource as-is
	ref<rtv> new_rtv(resource* r);
	ref<dsv> new_dsv(resource* r);
	ref<uav> new_uav(resource* r);
	ref<srv> new_srv(resource* r);

	// create simple render and depth-stencil targets
	ref<rtv> new_rtv(const char* name, uint32_t width, uint32_t height, const surface::format& format);
	ref<dsv> new_dsv(const char* name, uint32_t width, uint32_t height, const surface::format& format);
	ref<dsv> new_dsv(const char* name, rtv* rtv, const surface::format& format) { auto desc = rtv->get_resource()->get_desc(); return new_dsv(name, desc.width, desc.height, format); }

	// Queries
	ref<timer> new_timer(const char* name);                     
	ref<fence> new_fence(const char* name);
	ref<occlusion_query> new_occlusion_query(const char* name);
	ref<stats_query> new_stats_query(const char* name);

private:
	allocator alloc_;
	window* win_;
	void* dev_;
	ref<graphics_command_list> imm_;
	ref<fence> flush_sync_;
	void* swp_;
	ref<srv> swp_srv_;
	ref<rtv> swp_rtv_;
	ref<uav> swp_uav_;
	ref<rso>* rsos_;
	ref<pso>* psos_;
	ref<cso>* csos_;

	device_init init_;
	std::atomic<int> refcount_;
	uint32_t npresents_;
	bool supports_deferred_;

	ref<resource> persistent_mesh_buffer_;
	sbb_allocator persistent_mesh_alloc_;

	// Begin Transient Management
	
	oALIGNAS(oCACHE_LINE_SIZE) std::atomic<uint32_t> transient_ring_start_;
	uint32_t transient_ring_end_;
	uint32_t transient_fence_index_;

	ref<resource> transient_mesh_buffer_;
	std::array<ref<fence>, 4> transient_fences_;
	std::array<uint32_t, 4> next_transient_ring_ends_;
	
	// End Transient Management

	friend struct rso;
	friend struct pso;
	friend struct cso;
	friend struct sao;
	friend class graphics_command_list;
	friend struct timer;
	friend ref<device> new_device(const device_init& init, window* win);

	device(const device_init& init, window* win);
	void resize(uint32_t width, uint32_t height);

	ref<graphics_command_list> new_graphics_command_list_internal(const char* name, bool deferred);

	void del_rso(rso* o);
	void del_pso(pso* o);
	void del_cso(cso* o);
	void del_sao(sao* o);
	void del_graphics_command_list(graphics_command_list* cl);
	void del_timer(timer* t);
	
	void resolve_transient_fences();
	void insert_transient_fence();
	bool allocate_transient_mesh(graphics_command_list* cl, uint32_t bytes, void** out_ptr, uint32_t* out_offset);
};

ref<device> new_device(const device_init& init, window* win = nullptr);

namespace basic
{
	static const uint32_t default_sample_mask = 0xffffffff;

	static const sampler_desc point_wrap   = { filter_type::linear, false, comparison::never, 0, { 0.0f, 0.0f, 0.0f, 0.0f }, 0.0f, -FLT_MAX, FLT_MAX };
	static const sampler_desc point_clamp  = { filter_type::linear, true,  comparison::never, 0, { 0.0f, 0.0f, 0.0f, 0.0f }, 0.0f, -FLT_MAX, FLT_MAX };
	static const sampler_desc linear_wrap  = { filter_type::linear, false, comparison::never, 0, { 0.0f, 0.0f, 0.0f, 0.0f }, 0.0f, -FLT_MAX, FLT_MAX };
	static const sampler_desc linear_clamp = { filter_type::linear, true,  comparison::never, 0, { 0.0f, 0.0f, 0.0f, 0.0f }, 0.0f, -FLT_MAX, FLT_MAX };

	static const rasterizer_desc front_face      = { false, cull_mode::back,  false, false, 0, 0.0f, 0.0f, true, false, false, false, 0 };
	static const rasterizer_desc back_face       = { false, cull_mode::front, false, false, 0, 0.0f, 0.0f, true, false, false, false, 0 };
	static const rasterizer_desc two_sided       = { false, cull_mode::none,  false, false, 0, 0.0f, 0.0f, true, false, false, false, 0 };
	static const rasterizer_desc front_wire      = { true,  cull_mode::back,  false, false, 0, 0.0f, 0.0f, true, false, false, false, 0 };
	static const rasterizer_desc back_wire       = { true,  cull_mode::front, false, false, 0, 0.0f, 0.0f, true, false, false, false, 0 };
	static const rasterizer_desc two_sided_wire  = { true,  cull_mode::none,  false, false, 0, 0.0f, 0.0f, true, false, false, false, 0 };

	static const rt_blend_desc rt_opaque      = { false, false, blend_type::one,            blend_type::zero,          blend_op::add,  blend_type::one,  blend_type::zero, blend_op::add,  logic_op::noop, color_write_mask::all }; // Output.rgba = Source.rgba
	static const rt_blend_desc rt_alpha_test  = { false, false, blend_type::one,            blend_type::zero,          blend_op::add,  blend_type::one,  blend_type::zero, blend_op::add,  logic_op::noop, color_write_mask::all }; // Same as opaque, alpha clipping test is done in user shader code
	static const rt_blend_desc rt_accumulate  = { true,  false, blend_type::one,            blend_type::one,           blend_op::add,  blend_type::one,  blend_type::one,  blend_op::add,  logic_op::noop, color_write_mask::all }; // Output.rgba = Source.rgba + Destination.rgba
	static const rt_blend_desc rt_additive    = { true,  false, blend_type::src_alpha,      blend_type::one,           blend_op::add,  blend_type::zero, blend_type::zero, blend_op::add,  logic_op::noop, color_write_mask::all }; // Output.rgb = Source.rgb * Source.a + Destination.rgb
	static const rt_blend_desc rt_multiply    = { true,  false, blend_type::dest_color,     blend_type::zero,          blend_op::add,  blend_type::zero, blend_type::zero, blend_op::add,  logic_op::noop, color_write_mask::all }; // Output.rgb = Source.rgb * Destination.rgb
	static const rt_blend_desc rt_screen      = { true,  false, blend_type::inv_dest_color, blend_type::one,           blend_op::add,  blend_type::zero, blend_type::zero, blend_op::add,  logic_op::noop, color_write_mask::all }; // Output.rgb = Source.rgb * (1 - Destination.rgb) + Destination.rgb (as reduced from webpage's 255 - [((255 - Src)*(255 - Dst))/255])
	static const rt_blend_desc rt_translucent = { true,  false, blend_type::src_alpha,      blend_type::inv_src_alpha, blend_op::add,  blend_type::zero, blend_type::zero, blend_op::add,  logic_op::noop, color_write_mask::all }; // Output.rgb = Source.rgb * Source.a + Destination.rgb * (1 - Source.a)
	static const rt_blend_desc rt_minimum     = { true,  false, blend_type::one,            blend_type::one,           blend_op::min_, blend_type::one,  blend_type::one,  blend_op::min_, logic_op::noop, color_write_mask::all }; // Output.rgba = min(Source.rgba, Destination.rgba)
	static const rt_blend_desc rt_maximum     = { true,  false, blend_type::one,            blend_type::one,           blend_op::max_, blend_type::one,  blend_type::one,  blend_op::max_, logic_op::noop, color_write_mask::all }; // Output.rgba = max(Source.rgba, Destination.rgba)
																				 
	static const blend_desc opaque      = { false, false, { rt_opaque,      rt_opaque,      rt_opaque,      rt_opaque,      rt_opaque,      rt_opaque,      rt_opaque,      rt_opaque,      } };
	static const blend_desc alpha_test  = { false, false, { rt_alpha_test,  rt_alpha_test,  rt_alpha_test,  rt_alpha_test,  rt_alpha_test,  rt_alpha_test,  rt_alpha_test,  rt_alpha_test,  } };
	static const blend_desc accumulate  = { false, false, { rt_accumulate,  rt_accumulate,  rt_accumulate,  rt_accumulate,  rt_accumulate,  rt_accumulate,  rt_accumulate,  rt_accumulate,  } };
	static const blend_desc additive    = { false, false, { rt_additive,    rt_additive,    rt_additive,    rt_additive,    rt_additive,    rt_additive,    rt_additive,    rt_additive,    } };
	static const blend_desc multiply    = { false, false, { rt_multiply,    rt_multiply,    rt_multiply,    rt_multiply,    rt_multiply,    rt_multiply,    rt_multiply,    rt_multiply,    } };
	static const blend_desc screen      = { false, false, { rt_screen,      rt_screen,      rt_screen,      rt_screen,      rt_screen,      rt_screen,      rt_screen,      rt_screen,      } };
	static const blend_desc translucent = { false, false, { rt_translucent, rt_translucent, rt_translucent, rt_translucent, rt_translucent, rt_translucent, rt_translucent, rt_translucent, } };
	static const blend_desc minimum     = { false, false, { rt_minimum,     rt_minimum,     rt_minimum,     rt_minimum,     rt_minimum,     rt_minimum,     rt_minimum,     rt_minimum,     } };
	static const blend_desc maximum     = { false, false, { rt_maximum,     rt_maximum,     rt_maximum,     rt_maximum,     rt_maximum,     rt_maximum,     rt_maximum,     rt_maximum,     } };

	static const depth_stencil_desc no_depth_stencil     = { false, false, comparison::always,     false, 0xff, 0xff, stencil_op::keep, stencil_op::keep, stencil_op::keep, comparison::always, stencil_op::keep, stencil_op::keep, stencil_op::keep, comparison::always, };
	static const depth_stencil_desc depth_test_and_write = { true,  true,  comparison::less_equal, false, 0xff, 0xff, stencil_op::keep, stencil_op::keep, stencil_op::keep, comparison::always, stencil_op::keep, stencil_op::keep, stencil_op::keep, comparison::always, };
	static const depth_stencil_desc depth_test           = { true,  false, comparison::less_equal, false, 0xff, 0xff, stencil_op::keep, stencil_op::keep, stencil_op::keep, comparison::always, stencil_op::keep, stencil_op::keep, stencil_op::keep, comparison::always, };

	extern const BYTE VSfullscreen_tri[];   // Inputs[float3 position]                     Outputs[float4 SV_Position; float2 TEXCOORD] renders a fullscreen tri (more efficient than fullscreen quad). bind no indices or vertices and draw(3)
	extern const BYTE VSpass_through_pos[]; // Inputs[float3 position]                     Outputs[float4 SV_Position]                  returns input position as SV_Position - useful for first renderer bring-up and trivial test cases
	extern const BYTE PSblack[];            // Inputs[null]                                Outputs[float4(0,0,0,0) SV_Target] 
	extern const BYTE PSwhite[];            // Inputs[null]                                Outputs[float4(1,1,1,1) SV_Target]
	extern const BYTE PStex2d[];            // Inputs[float4 SV_Position; float2 TEXCOORD] Outputs[float4 sampled color SV_Target]      texture in srv slot 0, sampler in sao slot 0
};

}}

#define oGPU_REFREL(T) inline void ref_reference(ouro::gpu::T* p) { p->reference(); } inline void ref_release(ouro::gpu::T* p) { p->release(); } 

oGPU_REFREL(device)
oGPU_REFREL(rso)
oGPU_REFREL(pso)
oGPU_REFREL(cso)
oGPU_REFREL(sao)
oGPU_REFREL(graphics_command_list)
oGPU_REFREL(resource)
oGPU_REFREL(fence)
oGPU_REFREL(timer)
oGPU_REFREL(occlusion_query)
oGPU_REFREL(stats_query)
oGPU_REFREL(view)
oGPU_REFREL(rtv)
oGPU_REFREL(dsv)
oGPU_REFREL(srv)
oGPU_REFREL(uav)

#undef oGPU_REFREL
