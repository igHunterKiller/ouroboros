// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Wavefront OBJ 3D object file format support. Positions/texcoords/normals are always float3's.

#pragma once
#include <oMath/hlsl.h>
#include <oMemory/allocate.h>
#include <oMemory/std_linear_allocator.h>
#include <unordered_map>
#include <vector>

namespace ouro { namespace mesh { 

class obj
{
public:
	static const uint32_t path_max_len = 512;
	static const uint32_t name_max_len = 64;

	enum class load_option : uint8_t { ccw_right, ccw_left, cw_right, cw_left, standard = ccw_right };

	struct init_t
	{
		init_t()
			: mesh_alloc             (default_allocator)
			, temp_alloc             (default_allocator)
			, est_num_vertices       (100000)
			, est_num_indices        (100000)
			, option                 (load_option::standard)
		{}

		allocator   mesh_alloc;       // final memory
		allocator   temp_alloc;       // working/temp memory
		uint32_t    est_num_vertices; // pre-allocate working memory
		uint32_t    est_num_indices;  // pre-allocate working memory
		load_option option;           // how to interpret 3d space
	};

	struct group_t
	{
		char     name[name_max_len];
		char     material[name_max_len];
		uint32_t start_index;
		uint32_t num_indices;
		bool     has_normals;
		bool     has_texcoords;
	};

	obj();
	obj(const init_t& init);
	obj(const init_t& init, const char* oRESTRICT path, const void* oRESTRICT data, size_t data_size) : obj(init) { parse(path, data, data_size); }

	void            parse(const char* oRESTRICT path, const void* oRESTRICT data, size_t data_size);                      // replaces contents with a newly parsed version of the specified obj file
	const char*     obj_path()            const { return obj_path_;                                        }
	const char*     mtl_path()            const { return mtl_path_;                                        }
	uint32_t        num_indices()         const { return static_cast<uint32_t>(indices_  .size());         }
	uint32_t        num_vertices()        const { return static_cast<uint32_t>(positions_.size());         }
	uint32_t        num_groups()          const { return static_cast<uint32_t>(groups_.size());            }
	const uint32_t* indices()             const { return indices_.data();                                  }
	const float3*   positions()           const { return positions_.empty() ? nullptr : positions_.data(); }
	const float3*   texcoords()           const { return texcoords_.empty() ? nullptr : texcoords_.data(); }
	const float3*   normals()             const { return normals_.empty()   ? nullptr : normals_  .data(); }
	const group_t*  groups()              const { return groups_.empty()    ? nullptr : groups_   .data(); }
	float3          aabb_min()            const { return aabb_min_;                                        }
	float3          aabb_max()            const { return aabb_max_;                                        }
	const init_t&   init()                const { return init_;                                            }
	bool            ccw_faces()           const { return init_.option == load_option::ccw_right || init_.option == load_option::ccw_left; }
	bool            right_handed()        const { return init_.option == load_option::ccw_right || init_.option == load_option::cw_right; }

	// Editable versions. Missing components per vertex will be filled with zero to preserve vertex indexing, 
	// so missing values can be written in-place.
	float3*         texcoords()                 { return texcoords_.empty() ? nullptr : texcoords_.data(); }
	float3*         normals()                   { return normals_.empty()   ? nullptr : normals_  .data(); }

private:
	struct index_t
	{
		uint32_t pos, tex, nrm;
		bool operator==(const index_t& that) const { return pos == that.pos && tex == that.tex && nrm == that.nrm; }
		uint64_t hash() const { std::hash<uint32_t> h; return h(pos) + 37 * h(tex) + 37 * h(nrm); }
	};

	struct index_hasher : public std::unary_function<index_t, uint64_t> { uint64_t operator()(const index_t& i) const { return i.hash(); } };

	typedef std::vector<uint32_t, ouro_std_allocator<uint32_t>>                                          uint_vector;
	typedef std::vector<float3,   ouro_std_allocator<float3>>                                            float3_vector;
	typedef std::vector<group_t,  ouro_std_allocator<group_t>>                                           group_vector;
	typedef ouro::std_linear_allocator<std::pair<const index_t, uint32_t>>                               allocator_type;
	typedef std::unordered_map<index_t, uint32_t, index_hasher, std::equal_to<index_t>, allocator_type> index_map;
	
	uint32_t push_vertex(const index_t& ptn,                                                 index_map& map, const float3_vector& psrc, const float3_vector& tsrc, const float3_vector& nsrc);
	void     push_triangle(const index_t& ptn_a, const index_t& ptn_b, const index_t& ptn_c, index_map& map, const float3_vector& psrc, const float3_vector& tsrc, const float3_vector& nsrc);
	void     parse_index(const char** pp_str, const index_t& counts, index_t* index);

	uint_vector   indices_;
	float3_vector positions_;
	float3_vector texcoords_;
	float3_vector normals_;
	group_vector  groups_;
	char          obj_path_[path_max_len];
	char          mtl_path_[path_max_len];
	float3        aabb_min_;
	float3        aabb_max_;
	init_t        init_;
};

class mtl
{
	// http://local.wasp.uwa.edu.au/~pbourke/dataformats/mtl/

public:
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

	struct init_t
	{
		init_t()
			: alloc(default_allocator)
			, est_num_materials(32)
		{}

		allocator alloc;             // final memory
		uint32_t  est_num_materials; // pre-allocate working memory
	};

	struct texture
	{
		texture()
			: path("")
			, boost(0.0f)
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

		char         path[obj::path_max_len];
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

	struct material
	{
		material()
			: name              ("")
			, ambient_color     (0.0f, 0.0f, 0.0f)
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

		char         name[obj::name_max_len];
		texture      ambient;
		texture      diffuse;
		texture      alpha;
		texture      specular;
		texture      transmission;
		texture      bump;
	};

	mtl();
	mtl(const init_t& init);
	mtl(const init_t& init, const char* oRESTRICT path, const void* oRESTRICT data, size_t data_size) : mtl(init) { parse(path, data, data_size); }

	void            parse(const char* oRESTRICT path, const void* oRESTRICT data, size_t data_size);                      // replaces contents with a newly parsed version of the specified mtl file
	const char*     mtl_path()     const { return mtl_path_;                                        }

private:
	typedef std::vector<material, ouro_std_allocator<material>> material_vector;

	void parse_texture(const char** pp_str, texture* tex);

	char            mtl_path_[obj::path_max_len];
	material_vector materials_;
	init_t          init_;
};

}}
