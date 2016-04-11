// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Wavefront OBJ 3D object file format support

#pragma once
#include <oCore/color.h>
#include <oMemory/allocate.h>
#include <oString/path.h>
#include <oMesh/mesh.h>

namespace ouro { namespace mesh { namespace obj {

// http://local.wasp.uwa.edu.au/~pbourke/dataformats/mtl/
enum class illumination : uint8_t
{
	color_on_ambient_off,
	color_on_ambient_on,
	highlight_on,
	reflection_on_ray_trace_on,
	transparency_glass_on_reflection_ray_trace_on,
	reflection_fresnel_on_ray_trace_on,
	transparency_refraction_on_reflection_fresnel_off_ray_trace_on,
	transparency_refraction_on_reflection_frensel_on_ray_trace_on,
	reflection_on_ray_trace_off,
	transparency_glass_on_reflection_ray_trace_off,
	casts_shadows_onto_invisible_surfaces,

	count,
};

enum class texture_type : uint8_t
{
	regular,
	cube_right,
	cube_left,
	cube_top,
	cube_bottom,
	cube_back,
	cube_front,
	sphere,
	
	count,
};

struct group_t
{
	mstring group_name;
	mstring material_name;
};

struct info_t
{
	info_t()
		: obj_path (nullptr)
		, mtl_path (nullptr)
		, groups   (nullptr)
		, indices  (nullptr)
		, positions(nullptr)
		, normals  (nullptr)
		, texcoords(nullptr)
	{}

	path_t          obj_path;
	path_t          mtl_path;
	const group_t*  groups;    // mesh::info_t::num_subsets describes the array length
	const subset_t* subsets;   // mesh::info_t::num_subsets describes the array length
	const uint32_t* indices;   // mesh::info_t::num_indices describes the array length
	const float3*   positions; // mesh::info_t::num_vertices describes the array length, or this is nullptr if not available
	const float3*   normals;   // mesh::info_t::num_vertices describes the array length, or this is nullptr if not available
	const float3*   texcoords; // mesh::info_t::num_vertices describes the array length, or this is nullptr if not available
	mesh::info_t    mesh_info;
};

class mesh
{
public:
	struct init_t
	{
		init_t()
			: est_num_vertices       (100000)
			, est_num_indices        (100000)
			, flip_handedness        (false)
			, counter_clockwise_faces(true)
			, calc_normals_on_error  (true)
			, calc_texcoords_on_error(false)
		{}

		uint32_t est_num_vertices;          // used to pre-allocate memory (minimizes reallocs)
		uint32_t est_num_indices;           // used to pre-allocate memory (minimizes reallocs)
		bool     flip_handedness;
		bool     counter_clockwise_faces;
		bool     calc_normals_on_error;     // either for no loaded normals or degenerates
		bool     calc_texcoords_on_error;   // uses LCSM if no texcoords in source (NOT CURRENTLY IMPLEMENTED)
	};

	mesh()                                  : impl_(nullptr) {}
	~mesh()                                 { deinitialize(); }
	mesh(const mesh& that)                  = delete;
	const mesh& operator=(const mesh& that) = delete;
	mesh(mesh&& that) : impl_(that.impl_)   { that.impl_ = nullptr; }
	mesh& operator=(mesh&& that)            { if (this != &that) { impl_ = that.impl_; that.impl_ = nullptr; } return *this; }

	mesh(const init_t& init
		, const path_t& obj_path
		, const char* obj_string
		, const allocator& mesh_alloc = default_allocator
		, const allocator& temp_alloc = default_allocator)
	{ initialize(init, obj_path, obj_string, mesh_alloc, temp_alloc); }

	void initialize(const init_t& init
		, const path_t& obj_path
		, const char* obj_string
		, const allocator& mesh_alloc = default_allocator
		, const allocator& temp_alloc = default_allocator);

	void deinitialize();

	info_t info() const;

private:
	void* impl_;
};

struct texture_info
{
	texture_info()
		: boost(0.0f)
		, brightness_gain(0.0f, 1.0f)
		, origin_offset(0.0f, 0.0f, 0.0f)
		, scale(1.0f, 1.0f, 1.0f)
		, turbulance(0.0f, 0.0f, 0.0f)
		, resolution(uint32_t(-1), uint32_t(-1))
		, bump_multiplier(1.0f)
		, type(texture_type::regular)
		, imfchan('l')
		, blendu(true)
		, blendv(true)
		, clamp(false)
	{}

	path_t       path;
	float        boost;
	float2       brightness_gain;
	float3       origin_offset;
	float3       scale;
	float3       turbulance;
	uint2        resolution;
	float        bump_multiplier;
	texture_type type;
	char         imfchan;
	bool         blendu;
	bool         blendv;
	bool         clamp;
};

struct material_info
{
	material_info()
		: ambient_color     (0.0f, 0.0f, 0.0f)
		, emissive_color    (0.0f, 0.0f, 0.0f)
		, diffuse_color     (0.5f, 0.5f, 0.5f)
		, specular_color    (1.0f, 1.0f, 1.0f)
		, transmission_color(0.0f, 0.0f, 0.0f)
		, specularity       (0.04f)
		, transparency      (1.0f) // (1: opaque, 0: transparent)
		, refraction_index  (1.0f)
		, illum             (illumination::color_on_ambient_off)
	{}

	float3       ambient_color;
	float3       emissive_color;
	float3       diffuse_color;
	float3       specular_color;
	float3       transmission_color;
	float        specularity;
	float        transparency;
	float        refraction_index;
	illumination illum;

	path_t       name;
	texture_info ambient;
	texture_info diffuse;
	texture_info alpha;
	texture_info specular;
	texture_info transmission;
	texture_info bump;
};

struct material_lib_info
{
	material_lib_info()
		: mtl_path     (nullptr)
		, materials    (nullptr)
		, num_materials(0)
	{}

	path_t               mtl_path;
	const material_info* materials;
	uint32_t             num_materials;
};

class material_lib
{
public:
	material_lib()                                          : impl_(nullptr) {}
	~material_lib()                                         { deinitialize(); }
	material_lib(const material_lib& that)                  = delete;
	const material_lib& operator=(const material_lib& that) = delete;
	material_lib(material_lib&& that) : impl_(that.impl_)   { that.impl_ = nullptr; }
	material_lib& operator=(material_lib&& that)            { if (this != &that) { impl_ = that.impl_; that.impl_ = nullptr; } return *this; }

	void initialize(const path_t& mtl_path, const char* mtl_string, const allocator& alloc);
	void deinitialize();

	material_lib_info info() const;

private:
	void* impl_;
};

}}}
