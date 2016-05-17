// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oMesh/obj.h>
#include <oString/atof.h> // fast-path float parsing
#include <oString/string.h>
#include <oString/string_fast_scan.h> // fast-path string parsing

oDEFINE_WHITESPACE_PARSING();

namespace ouro { 

const char* as_string(const mesh::mtl::texture_type& t)
{
	static const char* s_names[] =
	{
		"regular",
		"cube_right",
		"cube_left",
		"cube_top",
		"cube_bottom",
		"cube_back",
		"cube_front",
		"sphere",
	};

	return as_string(t, s_names);
}

oDEFINE_TO_FROM_STRING(mesh::mtl::texture_type);

namespace mesh {

obj::obj()
	: obj_path_("")
	, mtl_path_("")
  , aabb_min_( FLT_MAX)
  , aabb_max_(-FLT_MAX)
{
}

obj::obj(const init_t& init)
  : indices_  (uint_vector  ::allocator_type(init.mesh_alloc, "obj indices"  ))
  , positions_(float3_vector::allocator_type(init.mesh_alloc, "obj positions"))
  , texcoords_(float3_vector::allocator_type(init.mesh_alloc, "obj texcoords"))
  , normals_  (float3_vector::allocator_type(init.mesh_alloc, "obj normals"  ))
  , groups_		(group_vector ::allocator_type(init.mesh_alloc, "obj groups"   ))
	, obj_path_("")
	, mtl_path_("")
  , aabb_min_( FLT_MAX)
  , aabb_max_(-FLT_MAX)
	, init_(init)
{}

static void atof3(const char** oRESTRICT pp_str, float3* oRESTRICT vec)
{
	     atof(pp_str, &vec->x);
	     atof(pp_str, &vec->y);
	if (!atof(pp_str, &vec->z))
		vec->z = 0.0f;
}

static uint32_t adjust_index(int32_t index, int32_t num_vertices)
{
	if (index > 0)
		return index - 1;
	else if (index < 0)
		return num_vertices + index;
	return 0;
}

static void move_past_number(const char** pp_str)
{
	while (isdigit(**pp_str) || **pp_str == '-')
		(*pp_str)++;
	move_past_line_whitespace(pp_str);
}

static void parse_string(const char** oRESTRICT pp_str, char* oRESTRICT dst, size_t dst_size)
{
	move_past_line_whitespace(pp_str);
	const char* start = *pp_str;
	move_to_line_end(pp_str);
	size_t len = std::distance(start, *pp_str);
	strncpy(dst, dst_size, start, len);
}

template<size_t size> static inline void parse_string(const char** pp_str, char (&dst)[size]) { parse_string(pp_str, dst, size); }

void obj::parse_index(const char** oRESTRICT pp_str, const index_t& counts, index_t* oRESTRICT index)
{
	// p  p/t  p//n  p/t/n

	move_past_line_whitespace(pp_str);
	index->pos = adjust_index(atoi(*pp_str), counts.pos);
	index->tex = uint32_t(-1);
	index->nrm = uint32_t(-1);
	move_past_number(pp_str);

	if (**pp_str == '/') // p/t  p//n  p/t/n
	{
		(*pp_str)++; move_past_line_whitespace(pp_str);
		if (**pp_str == '/') // p//n
		{
			(*pp_str)++; move_past_line_whitespace(pp_str);
			index->nrm = adjust_index(atoi(*pp_str), counts.nrm);
		}
		else // p/t  p/t/n
			index->tex = adjust_index(atoi(*pp_str), counts.tex);

		move_past_number(pp_str);
		if (**pp_str == '/') // p/t/n
		{
			(*pp_str)++;
			index->nrm = adjust_index(atoi(*pp_str), counts.nrm);
			move_past_number(pp_str);
		}
	} // else p
}

uint32_t obj::push_vertex(const index_t& ptn, index_map& map, const float3_vector& psrc, const float3_vector& tsrc, const float3_vector& nsrc)
{
	if (ptn.pos >= (uint32_t)psrc.size())
		throw std::invalid_argument("invalid position index");

	uint32_t index;
	auto it = map.find(ptn);
	if (it != map.end())
		index = it->second;
	else
	{
		map[ptn] = index = (uint32_t)positions_.size();

		positions_.push_back(psrc[ptn.pos]);
		texcoords_.push_back((ptn.tex < (uint32_t)tsrc.size()) ? tsrc[ptn.tex] : float3(0.0f));
		normals_  .push_back((ptn.nrm < (uint32_t)nsrc.size()) ? nsrc[ptn.nrm] : float3(0.0f));
	}

	return index;
}

void obj::push_triangle(const index_t& ptn_a, const index_t& ptn_b, const index_t& ptn_c, index_map& map, const float3_vector& psrc, const float3_vector& tsrc, const float3_vector& nsrc)
{
	indices_.push_back(push_vertex(ptn_a, map, psrc, tsrc, nsrc));
	indices_.push_back(push_vertex(ptn_b, map, psrc, tsrc, nsrc));
	indices_.push_back(push_vertex(ptn_c, map, psrc, tsrc, nsrc));
}

void obj::parse(const char* oRESTRICT path, const void* oRESTRICT data, size_t data_size)
{
	const auto option           = init_.option;
	const auto flip_handedness  = !right_handed();
	const auto est_num_indices  = init_.est_num_indices;
	const auto est_num_vertices = init_.est_num_vertices;
	const auto temp_alloc       = init_.temp_alloc;

	// reinitialize final storage
	strlcpy(obj_path_, path);
	mtl_path_[0] = '\0';
	aabb_min_    = float3( FLT_MAX);
	aabb_max_    = float3(-FLT_MAX);

	indices_  .clear(); indices_  .reserve(est_num_indices);
	positions_.clear(); positions_.reserve(est_num_vertices);
	texcoords_.clear(); texcoords_.reserve(est_num_vertices);
	normals_  .clear(); normals_  .reserve(est_num_vertices);
	groups_   .clear(); groups_   .reserve(16);

	// initialize temp containers
	float3_vector pos(float3_vector::allocator_type(temp_alloc, "obj temp positions"));
	float3_vector tex(float3_vector::allocator_type(temp_alloc, "obj temp texcoords"));
	float3_vector nrm(float3_vector::allocator_type(temp_alloc, "obj temp normals"  ));

	pos.reserve(est_num_vertices);
	tex.reserve(est_num_vertices);
	nrm.reserve(est_num_vertices);

	// initialize a linear allocator for index map - we won't be freeing entries individually
	size_t   index_map_bytes  = est_num_indices * sizeof(index_map::value_type);
	auto     index_map_memory = temp_alloc.scoped_allocate(index_map_bytes, "index_map linear_allocator");

	// reindex from obj's flat list into one where all elements have the same index
	linear_allocator index_map_linear_alloc(index_map_memory, index_map_bytes);
	index_map        map(0, index_map::hasher(), index_map::key_equal(), allocator_type(&index_map_linear_alloc, &index_map_bytes));

	// scope tracking
	group_t     group;
	            group.material[0]   = '\0';
	            group.start_index   = 0;
	            group.num_indices   = 0;
							group.has_normals   = false;
							group.has_texcoords = false;
	            strlcpy(group.name, "default");
	uint32_t    num_groups          = 0;

	// read head
	const char* cur                 = (const char*)data;
	const char* end                 = cur + data_size;

	while (cur < end)
	{
		move_past_line_whitespace(&cur);

		switch (*cur++)
		{
			case 'v':
			{
				float3 vec;
				switch (*cur++)
				{
					case ' ':
						atof3(&cur, &vec);
						if (flip_handedness) vec.z = -vec.z;
						pos.push_back(vec);
						aabb_min_ = min(aabb_min_, vec);
						aabb_max_ = max(aabb_max_, vec);
						break;
					case 't':
						atof3(&cur, &vec);
						if (flip_handedness) vec.y = 1.0f - vec.y;
						tex.push_back(vec);
						group.has_texcoords = true;
						break;
					case 'n':
						atof3(&cur, &vec);
						if (flip_handedness) vec.z = -vec.z;
						nrm.push_back(vec);
						group.has_normals = true;
						break;
					default:
						throw std::invalid_argument("invalid vertex token");
				}

				break;
			}

			case 'f': // p, p//n, p/t/n, p/t
			{
				index_t ptn[4];
				index_t counts = { (uint32_t)pos.size(), (uint32_t)tex.size(), (uint32_t)nrm.size() };
				
				parse_index(&cur, counts, ptn + 0);
				parse_index(&cur, counts, ptn + 1);
				parse_index(&cur, counts, ptn + 2);

				if (!is_newline(*cur))
				{
					parse_index(&cur, counts, ptn + 3);
					push_triangle(ptn[0], ptn[2], ptn[3], map, pos, tex, nrm); // break up quad immediately
				}
				
				push_triangle(ptn[0], ptn[1], ptn[2], map, pos, tex, nrm);
				break;
			}

			case 'g':
				if (num_groups) // close out previous group
				{
					group.num_indices = (uint32_t)indices_.size() - group.start_index;
					groups_.push_back(group);
					group.start_index = (uint32_t)indices_.size();
				}
				num_groups++;
				parse_string(&cur, group.name);
				break;

			case 'u':
				// if a new material is specified, but no new group, then insert one
				// the important grouping is by material. Groups with the same material
				// are fine to merge.
				cur += 6;
				if (group.material[0] == '\0')
					parse_string(&cur, group.material);
				else
				{
					// close out previous group
					group.num_indices = (uint32_t)indices_.size() - group.start_index;
					groups_.push_back(group);
					group.start_index = (uint32_t)indices_.size();
					num_groups++;
					parse_string(&cur, group.material);
					strlcpy(group.name, group.material);
				}
				break;

			case 'm':
				cur += 6;
				if (mtl_path_[0] != '\0')
					throw std::invalid_argument("unsupported: two mtllibs specified");
				parse_string(&cur, mtl_path_);
				break;
		}
		
		move_to_line_end(&cur);
		move_past_newline(&cur);
	}

	// close out last group
	group.num_indices = (uint32_t)indices_.size() - group.start_index;
	groups_.push_back(group);
}

mtl::mtl()
	: mtl_path_("")
{
}

mtl::mtl(const init_t& init)
	: mtl_path_("")
	, materials_(material_vector::allocator_type(init.alloc, "mtl materials"))
	, init_(init)
{
	materials_.reserve(init.est_num_materials);
}

void mtl::parse(const char* oRESTRICT path, const void* oRESTRICT data, size_t data_size)
{
	strlcpy(mtl_path_, path);

	// read head
	const char* cur = (const char*)data;
	const char* end = cur + data_size;

	material_vector::pointer mat = nullptr;

	while (cur < end)
	{
		move_past_line_whitespace(&cur);

		switch (*cur)
		{
			case 'n':
				cur += 6; // "newmtl"
				materials_.resize(materials_.size() + 1);
				mat = &materials_.back();
				move_past_line_whitespace(&cur);
				parse_string(&cur, mat->name);
				break;
			case 'N': // Ni: index of refraction; Ns: specular exponent
			{
				float* dst = (*(++cur) == 'i') ? &mat->refraction_index : &mat->specularity;
				atof(&(++cur), dst);
				break;
			}
			case 'T': // Tf: transmission rgb; Tr: transparency (1 - d)
				if (*(++cur) == 'f')
					atof3(&(++cur), &mat->transmission_color);
				else if (*cur == 'r') 
				{
					float val;
					atof(&(++cur), &val);
					mat->transparency = 1.0f - val;
				}
				break;
			case 'd': // aka 1 - Tr
				atof(&(++cur), &mat->transparency);
				break;
			case 'K':
			{
				float3 dummy, *dst = nullptr;
				switch (*(++cur))
				{
					case 'a': dst = &mat->ambient_color;  break;
					case 'e': dst = &mat->emissive_color; break;
					case 'd': dst = &mat->diffuse_color;  break;
					case 's': dst = &mat->specular_color; break;
					case 'm': dst = &dummy;               break; // I can't find Km documented anywhere.
				}
				atof3(&(++cur), dst);
				break;
			}
			case 'm':
			{
				texture* tex = nullptr;
				     if (!_memicmp("map_Ka",   cur, 6))                                      tex = &mat->ambient;
				else if (!_memicmp("map_Kd",   cur, 6))                                      tex = &mat->diffuse;
				else if (!_memicmp("map_Ks",   cur, 6))                                      tex = &mat->specular;
				else if (!_memicmp("map_d",    cur, 5) || !_memicmp("map_opacity", cur, 11)) tex = &mat->alpha;
				else if (!_memicmp("map_Bump", cur, 8) || !_memicmp("bump",        cur,  4)) tex = &mat->bump;
				else break;
				parse_texture(&cur, tex);
				break;
			}
			case 'i':
			{
				cur += 5; // "illum"
				move_past_line_whitespace(&cur);
				auto illum_int = atoi(cur);
				mat->illum = (illum_int >= 0 && illum_int < 11) ? (illumination)illum_int : illumination::color_on_ambient_off;
				move_past_number(&cur);
				break;
			}
			default:
				break;
		}

		move_to_line_end(&cur);
		move_past_newline(&cur);
	}
}

static void parse_texture_option(const char** oRESTRICT pp_str, float3* oRESTRICT out_option)
{
	move_to_whitespace(pp_str);
	atof(pp_str, &out_option->x);
	
	if (atof(pp_str, &out_option->y))
	{
		if (!atof(pp_str, &out_option->z))
			out_option->z = 0.0f;
	}

	else
	{
		out_option->y = 0.0f;
		out_option->z = 0.0f;
	}
}

static void parse_toggle(const char** pp_str, bool* out_toggle)
{
	move_next_word(pp_str);
	bool toggle = !!_memicmp(*pp_str, "on", 2);
	*pp_str += toggle ? 2 : 3;
	*out_toggle = toggle;
}

void mtl::parse_texture(const char** pp_str, texture* tex)
{
	move_next_word(pp_str);
	while (**pp_str)
	{
		if (**pp_str == '-' && *((*pp_str)-1) == ' ')
		{
			if (*(++(*pp_str)) == 'o')
				parse_texture_option(pp_str, &tex->origin_offset);

			else if (**pp_str == 's')
				parse_texture_option(pp_str, &tex->scale);

			else if (**pp_str == 't')
				parse_texture_option(pp_str, &tex->turbulance);

			else if (**pp_str == 'm' && *((*pp_str)+1) == 'm')
			{
				*pp_str += 2;
				atof(pp_str, &tex->brightness_gain.x);
				atof(pp_str, &tex->brightness_gain.y);
			}

			else if (**pp_str == 'b' && *((*pp_str)+1) == 'm')
			{
				*pp_str += 2;
				atof(pp_str, &tex->bump_multiplier);
			}

			else if (!_memicmp(*pp_str, "blendu", 6))
				parse_toggle(pp_str, &tex->blendu);

			else if (!_memicmp(*pp_str, "blendv", 6))
				parse_toggle(pp_str, &tex->blendv);

			else if (!_memicmp(*pp_str, "boost", 5))
			{
				*pp_str += 5;
				atof(pp_str, &tex->boost);
			}

			else if (!_memicmp(*pp_str, "texres", 6))
			{
				*pp_str += 6;
				move_past_line_whitespace(pp_str);
				tex->resolution.x = atoi(*pp_str);
				move_next_word(pp_str);
				tex->resolution.y = atoi(*pp_str);
				move_to_whitespace(pp_str);
			}

			else if (!_memicmp(*pp_str, "clamp", 5))
				parse_toggle(pp_str, &tex->clamp);

			else if (!_memicmp(*pp_str, "imfchan", 7))
			{
				*pp_str += 7;
				move_past_line_whitespace(pp_str);
				tex->imfchan = *(*pp_str)++;
			}

			else if (!_memicmp(*pp_str, "type", 4))
			{
				*pp_str += 4;
				move_past_line_whitespace(pp_str);
				from_string(&tex->type, *pp_str);
				move_to_whitespace(pp_str);
			}
		}

		else
		{
			move_past_line_whitespace(pp_str);
			const char* start = *pp_str;
			move_to_line_end(pp_str);
			strncpy(tex->path, start, *pp_str - start);
			break;
		}
	}
}

}}
