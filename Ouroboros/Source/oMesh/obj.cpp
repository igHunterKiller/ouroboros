// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oCore/countof.h>
#include <oCore/finally.h>
#include <oCore/macros.h>
#include <oMesh/obj.h>
#include <oBase/unordered_map.h>
#include <oString/atof.h>
#include <oString/fixed_string.h>
#include <oString/string_fast_scan.h>
#include <oMemory/std_linear_allocator.h>

oDEFINE_WHITESPACE_PARSING();

namespace ouro {

template<> const char* as_string(const mesh::obj::texture_type& type)
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
	return as_string(type, s_names);
}

oDEFINE_TO_FROM_STRING(mesh::obj::texture_type)

	namespace mesh { namespace obj {

// To translate from several index streams - one per vertex element stream - to 
// a single index buffer we'll need to replicate vertices by their unique 
// combination of all vertex elements. This basically means an index is uniquely
// identified by a hash of the streams it indexes. We'll build the hash as well
// parse faces but then it all can be freed at once. To prevent a length series
// of deallocs for indices - i.e. int values - use a linear allocator and then
// free all entries once the map goes out of scope.
typedef uint64_t key_t;
typedef uint32_t val_t;
typedef std::pair<const key_t, val_t> pair_type;
typedef ouro::std_linear_allocator<pair_type> allocator_type;
typedef unordered_map<key_t, val_t, std_noop_hash<key_t>, std::equal_to<key_t>, std::less<key_t>, allocator_type> index_map_t;

static const uint32_t invalid_index = ~0u;

static inline const char* parse_string(char* dst, size_t dst_size, const char* str)
{
	move_next_word(&str);
	const char* start = str;
	move_to_line_end(&str);
	size_t len = std::distance(start, str);
	strncpy(dst, dst_size, start, len);
	return str;
}

template<size_t size> static inline const char* parse_string(fixed_string<char, size>& dst, const char* r) { return parse_string(dst, dst.capacity(), r); }

static const uint32_t kMaxNumVertsPerFace = 4;

struct face
{
	face() 
		: num_indices(0)
		, group_range_index(0)
	{
		for (auto& i : index)
			i.fill(invalid_index);
	}

	enum semantic
	{
		position,
		texcoord,
		normal,

		semantic_count,
	};

	std::array<std::array<uint32_t, semantic_count>, kMaxNumVertsPerFace> index;
	uint32_t num_indices; // 3 for tris, 4 for quads
	uint32_t group_range_index; // groups and ranges are indexed with the same value
};

struct vertex_data
{
	vertex_data(const allocator& alloc)
		: aabb_min ( FLT_MAX,  FLT_MAX,  FLT_MAX)
		, aabb_max (-FLT_MAX, -FLT_MAX, -FLT_MAX)
		, positions(ouro_std_allocator<float3>  (alloc, "obj vertex_data"))
		, normals  (ouro_std_allocator<float3>  (alloc, "obj vertex_data"))
		, texcoords(ouro_std_allocator<float3>  (alloc, "obj vertex_data"))
		, faces    (ouro_std_allocator<face>    (alloc, "obj vertex_data"))
		, groups   (ouro_std_allocator<group_t> (alloc, "obj vertex_data"))
		, subsets  (ouro_std_allocator<subset_t>(alloc, "obj vertex_data"))
	{}

	void reserve(uint32_t new_num_vertices, uint32_t new_num_faces)
	{
		positions.reserve(new_num_vertices);
		normals.reserve(new_num_vertices);
		texcoords.reserve(new_num_vertices);
		faces.reserve(new_num_faces);
		groups.reserve(20);
	}

	float3 aabb_min;
	float3 aabb_max;

	std::vector<float3, ouro_std_allocator<float3>>     positions;
	std::vector<float3, ouro_std_allocator<float3>>     normals;
	std::vector<float3, ouro_std_allocator<float3>>     texcoords;
	
	// these are assigned after negative indices have been handled, but are 
	// otherwise directly-from-file values.
	std::vector<face,     ouro_std_allocator<face>>     faces;
	std::vector<group_t,  ouro_std_allocator<group_t>>  groups;
	std::vector<subset_t, ouro_std_allocator<subset_t>> subsets;

	path_string mtl_path;
};

// Given a string that starts with the letter 'v', parse as a line of vector
// values and push_back into the appropriate vector.
template<typename allocT>
static const char* parse_vline(const char* vline
	, bool flip_handedness
	, std::vector<float3, allocT>& positions
	, std::vector<float3, allocT>& normals
	, std::vector<float3, allocT>& texcoords
	, float3& min_position
	, float3& max_position)
{
	float3 temp;

	vline++;
	switch (*vline)
	{
		case ' ':
			atof(&vline, &temp.x);
			atof(&vline, &temp.y);
			atof(&vline, &temp.z); if (flip_handedness) temp.z = -temp.z;
			positions.push_back(temp);
			min_position = min(min_position, temp);
			max_position = max(max_position, temp);
			break;
		case 't':
			move_to_whitespace(&vline);
			atof(&vline, &temp.x);
			atof(&vline, &temp.y);
			if (!atof(&vline, &temp.z)) temp.z = 0.0f;
			if (flip_handedness) temp.y = 1.0f - temp.y;
			texcoords.push_back(temp);
			break;
		case 'n':
			move_to_whitespace(&vline);
			atof(&vline, &temp.x);
			atof(&vline, &temp.y);
			atof(&vline, &temp.z); if (flip_handedness) temp.z = -temp.z;
			normals.push_back(temp);
			break;
		default: oThrow(std::errc::invalid_argument, "invalid token '%c'", *vline);
	}

	return vline;
}

// Fills the specified face with data from a line in an OBJ starting with the 
// 'f' (face) character. This returns a pointer into the string fline that is 
// either the end of the string, or the end of the line.
template<typename allocT>
static const char* parse_fline(const char* fline
	, uint32_t num_positions
	, uint32_t num_normals
	, uint32_t num_texcoords
	, uint32_t num_groups
	, std::vector<face, allocT>& faces)
{
	move_to_whitespace(&fline);
	move_past_line_whitespace(&fline);
	face f;
	f.num_indices = 0;
	f.group_range_index = num_groups;
	while (f.num_indices < 4 && *fline != '\0' && !is_newline(*fline))
	{
		bool foundSlash = false;
		int semantic = face::position;
		do
		{
			// parsing faces is relative to vertex parsing up to this point,
			// so figure out where the data's at.
			uint32_t zero_based_index_from_file = 0;

			// Negative indices implies the max indexable vertex OBJ will be INT_MAX, 
			// not UINT_MAX.
			if (*fline == '-')
			{
				int negative_index = atoi(fline);
				uint32_t nelements = 0;
				switch (semantic)
				{
					case face::position: nelements = num_positions; break;
					case face::normal:   nelements = num_normals;   break;
					case face::texcoord: nelements = num_texcoords; break;
				}
				zero_based_index_from_file = nelements + negative_index;
			}
			else
				zero_based_index_from_file = atoi(fline) - 1;

			f.index[f.num_indices][semantic] = zero_based_index_from_file;

			if (semantic == (face::semantic_count-1))
				break;

			while (isdigit(*fline) || *fline == '-')
				fline++;

			// support case where the texcoord channel is empty
			
			if (*fline == '/')
			{
				do
				{
					fline++;
					semantic++;
					foundSlash = true;
				} while(*fline == '/');
			}

			else
				break;

			move_past_line_whitespace(&fline);

		} while(foundSlash);

		move_next_word(&fline);
		f.num_indices++;
	}

	faces.push_back(f);

	return fline;
}

// Scans an OBJ string and appends vertex element data
static void parse_elements(const char* obj_string, bool flip_handedness, vertex_data* elements)
{
	uint32_t ngroups = 0;
	group_t group;

	const char* line = obj_string;
	while (*line)
	{
		move_past_line_whitespace(&line);
		switch (*line)
		{
			case 'v':
				line = parse_vline(line, flip_handedness, elements->positions, elements->normals, elements->texcoords, elements->aabb_min, elements->aabb_max);
				break;
			case 'f':
				line = parse_fline(line
					, (uint32_t)elements->positions.size()
					, (uint32_t)elements->normals.size()
					, (uint32_t)elements->texcoords.size()
					, (uint32_t)elements->groups.size()
					, elements->faces);
				break;
			case 'g':
				// close out previous group
				if (ngroups)
					elements->groups.push_back(group); 
				ngroups++;
				parse_string(group.group_name, line);
				break;
			case 'u':
				parse_string(group.material_name, line);
				break;
			case 'm':
				parse_string(elements->mtl_path, line);
				break;
			default:
				break;
		}
		move_to_line_end(&line);
		move_past_newline(&line);
	}

	// close out a remaining group one last time
	// NOTE: start prim / num prims are handled later after vertices have been reduced
	if (ngroups)
		elements->groups.push_back(group);

	else
	{
		group.group_name = "Default Group";
		elements->groups.push_back(group);
	}
}

// Using config data and an index hash, reduce all duplicate/face data into 
// unique indexed data. This must be called after ALL SourceElements have been
// populated.
template<typename allocT>
static void reduce_elements(const mesh::init_t& init
	, index_map_t* index_map
	, const vertex_data& src_elements
	, std::vector<uint32_t, allocT>* indices
	, vertex_data* singly_indexed_elements
	, std::vector<uint32_t, allocT>* degenerate_normals
	, std::vector<uint32_t, allocT>* degenerate_texcoords)
{
	singly_indexed_elements->subsets.resize(src_elements.groups.size());

	uint32_t resolvedIndices[4];
	singly_indexed_elements->aabb_min = src_elements.aabb_min;
	singly_indexed_elements->aabb_max = src_elements.aabb_max;
	singly_indexed_elements->groups = src_elements.groups;
	singly_indexed_elements->mtl_path = src_elements.mtl_path;
	uint32_t last_range_index = invalid_index;

	for (const face& f : src_elements.faces)
	{
		auto& subset = singly_indexed_elements->subsets[f.group_range_index];

		if (last_range_index != f.group_range_index)
		{
			if (last_range_index != invalid_index)
			{
				auto& last_subset = singly_indexed_elements->subsets[last_range_index];
				last_subset.num_indices = (uint32_t)indices->size() - last_subset.start_index;
				subset.start_index = (uint32_t)indices->size();
			}
			last_range_index = f.group_range_index;
		}

		for (uint32_t p = 0; p < f.num_indices; p++)
		{
			const key_t hash = f.index[p][face::position] + src_elements.positions.size() * f.index[p][face::texcoord] + src_elements.positions.size() * src_elements.texcoords.size() * f.index[p][face::normal];
			index_map_t::iterator it = index_map->find(hash);

			if (it == index_map->end())
			{
				uint32_t NewIndex = (uint32_t)singly_indexed_elements->positions.size();
				singly_indexed_elements->positions.push_back(src_elements.positions[f.index[p][face::position]]);
			
				if (!src_elements.normals.empty())
				{
					if (f.index[p][face::normal] != invalid_index)
						singly_indexed_elements->normals.push_back(src_elements.normals[f.index[p][face::normal]]);
					else
					{
						degenerate_normals->push_back(NewIndex);
						singly_indexed_elements->normals.push_back(float3(0.0f, 0.0f, 0.0f));
					}
				}

				if (!src_elements.texcoords.empty())
				{
					if(f.index[p][face::texcoord] != invalid_index)
						singly_indexed_elements->texcoords.push_back(src_elements.texcoords[f.index[p][face::texcoord]]);
					else
					{
						degenerate_texcoords->push_back(NewIndex);
						singly_indexed_elements->texcoords.push_back(float3(0.0f, 0.0f, 0.0f));
					}
				}
			
				(*index_map)[hash] = resolvedIndices[p] = NewIndex;
			}
		
			else
				resolvedIndices[p] = (*it).second;
		} // for each face element

		// Now that the index has either been added or reset to a pre-existing index, 
		// add the face definition to the index list

		static const uint32_t kCkCWindices[6] = { 0, 2, 1, 2, 0, 3, };
		static const uint32_t kCWindices[6] = { 0, 1, 2, 2, 3, 0, };
	
		bool UseCCW = init.counter_clockwise_faces;
		if (init.flip_handedness)
			UseCCW = !UseCCW;
	
		const uint32_t* order = UseCCW ? kCkCWindices : kCWindices;

		indices->push_back(resolvedIndices[order[0]]);
		indices->push_back(resolvedIndices[order[1]]);
		indices->push_back(resolvedIndices[order[2]]);

		// Add another triangle for the rest of the quad
		if (f.num_indices == 4)
		{
			indices->push_back(resolvedIndices[order[3]]);
			indices->push_back(resolvedIndices[order[4]]);
			indices->push_back(resolvedIndices[order[5]]);
		}
	} // for each face

	// close out last group
	last_range_index = src_elements.faces.back().group_range_index;
	auto& s = singly_indexed_elements->subsets[last_range_index];
	s.num_indices = (uint32_t)indices->size() - s.start_index;
}

struct mesh_impl
{
	mesh_impl(const mesh::init_t& init, const path_t& obj_path, const char* obj_string, const allocator& mesh_alloc, const allocator& temp_alloc);
	info_t info() const;

	vertex_data                                         data_;
	std::vector<uint32_t, ouro_std_allocator<uint32_t>> indices_;
	path_t                                              path_;
	bool                                                counter_clockwise_faces_;
};

mesh_impl::mesh_impl(const mesh::init_t& init, const path_t& obj_path, const char* obj_string, const allocator& mesh_alloc, const allocator& temp_alloc)
	: path_(obj_path)
	, counter_clockwise_faces_(init.counter_clockwise_faces)
	, data_(mesh_alloc)
{
	const size_t kInitialReserve = init.est_num_indices * sizeof(uint32_t);
	size_t       index_map_bytes = 0;

	std::vector<uint32_t, ouro_std_allocator<uint32_t>> degenerate_normals  (ouro_std_allocator<uint32_t>(temp_alloc, "degenerate_normals"));
	std::vector<uint32_t, ouro_std_allocator<uint32_t>> degenerate_texcoords(ouro_std_allocator<uint32_t>(temp_alloc, "degenerate_texcoords"));
	degenerate_normals.reserve(1000);
	degenerate_texcoords.reserve(1000);

	// Scope memory usage... more might be used below when calculating normals, 
	// etc. so free this stuff up rather than leaving it all around.
	
	{
		// create a hash map backed by a linear allocator: reduce_elements will hash geometry elements to minimize vertices,
		// then the hash is no longer needed - and no deallocation is necessary, so the linear allocator saves all the teardown free's
		void* index_map_memory = temp_alloc.allocate(kInitialReserve, "index_map linear_allocator");
		oFinally { if (index_map_memory) temp_alloc.deallocate(index_map_memory); };
		linear_allocator lin_alloc(index_map_memory, kInitialReserve);
		index_map_t index_map(0, index_map_t::hasher(), index_map_t::key_equal(), std::less<key_t>(), allocator_type(&lin_alloc, &index_map_bytes));

		// OBJ files don't contain a same-sized vertex streams for each elements, 
		// and indexing occurs uniquely between vertex elements. Eventually we will 
		// create same-sized vertex data - even by replication of data - so that a 
		// single index buffer can be used. That's the this->data_, but first get 
		// the raw vertex data off disk using off_disk_data.
		vertex_data off_disk_data(temp_alloc);
		off_disk_data.reserve(init.est_num_vertices, init.est_num_indices / 3);
		parse_elements(obj_string, init.flip_handedness, &off_disk_data);

		const uint32_t est_max_vertices = (uint32_t)max(max(off_disk_data.positions.size(), off_disk_data.normals.size()), off_disk_data.texcoords.size());
		data_.reserve(est_max_vertices, init.est_num_indices / 3);
		indices_.reserve(init.est_num_indices);

		reduce_elements(init, &index_map, off_disk_data, &indices_, &data_, &degenerate_normals, &degenerate_texcoords);

	} // End of life for from-disk vertex data and index hash

	#ifdef _DEBUG
		if (index_map_bytes > kInitialReserve)
		{
			mstring reserved, additional;
			const char* n = obj_path.c_str(); // passing the macro directly causes win32 to throw an exception
			oTrace("[obj] %s index map allocated %s additional indices beyond the initial est_num_indices=%s", n, format_commas(additional, (uint32_t)((index_map_bytes - kInitialReserve) / sizeof(uint32_t))), format_commas(reserved, init.est_num_indices));
		}
	#endif

	if (init.calc_normals_on_error)
	{
		bool calc_normals = false;
		if (data_.normals.empty())
		{
			oTrace("[obj] No normals found in %s...", obj_path.c_str());
			calc_normals = true;
		}

		else if (!degenerate_normals.empty())
		{
			// @tony: Is there a way to calculate only the degenerates?
			oTrace("[obj] %u degenerate normals in %s...", degenerate_normals.size(), obj_path.c_str());
			calc_normals = true;
		}

		if (calc_normals)
		{
			oTrace("[obj] Calculating vertex normals... (%s)", obj_path.c_str());
			sstring str_time;
			timer t;
			data_.normals.resize(data_.positions.size());
			calc_vertex_normals(data_.normals.data(), indices_.data(), (uint32_t)indices_.size(), data_.positions.data(), (uint32_t)data_.positions.size(), init.counter_clockwise_faces, true);
			format_duration(str_time, t.seconds(), true, true);
			oTrace("[obj] Calculating vertex normals done in %s. (%s)", str_time.c_str(), obj_path.c_str());
		}
	}

	if (init.calc_texcoords_on_error)
	{
		bool calc_texcoords = false;
		if (data_.texcoords.empty())
		{
			oTrace("[obj] No texcoords found in %s...", obj_path.c_str());
			calc_texcoords = true;
		}

		else if (!degenerate_texcoords.empty())
		{
			oTrace("[obj] %u degenerate texcoords in %s...", degenerate_texcoords.size(), obj_path.c_str());
			calc_texcoords = true;
		}

		if (calc_texcoords)
		{
			data_.texcoords.resize(data_.positions.size());
			sstring str_time;
			double solver_time = 0.0;
			oTrace("[obj] Calculating texture coordinates... (%s)", obj_path.c_str());
			try { ouro::mesh::calc_texcoords(data_.aabb_min, data_.aabb_max, indices_.data(), (uint32_t)indices_.size(), data_.positions.data(), data_.texcoords.data(), (uint32_t)data_.texcoords.size(), &solver_time); }
			catch (std::exception& e)
			{
				e;
				data_.texcoords.clear();
				oTraceA("[obj] Calculating texture coordinates failed. %s (%s)", obj_path, e.what());
			}

			format_duration(str_time, solver_time, true, true);
			oTrace("[obj] Calculating texture coordinates done in %s. (%s)", str_time.c_str(), obj_path.c_str());
		}
	}
}

info_t mesh_impl::info() const
{
	info_t i;
	i.obj_path                     = path_;
	i.mtl_path                     = data_.mtl_path;
	i.groups                       = data_.groups.data();
	i.subsets                      = data_.subsets.data();
	i.indices                      = indices_.data();
	i.positions                    = data_.positions.empty() ? nullptr : data_.positions.data();
	i.normals                      = data_.normals.empty() ? nullptr : data_.normals.data();
	i.texcoords                    = data_.texcoords.empty() ? nullptr : data_.texcoords.data();

	i.mesh_info.num_indices        = (uint32_t)indices_.size();
	i.mesh_info.num_vertices       = (uint32_t)data_.positions.size();
	i.mesh_info.num_subsets        = (uint16_t)data_.groups.size();
	i.mesh_info.num_slots          = 3; // each element is put in its own slots: obj supports pos, nrm, tan
	i.mesh_info.log2scale          = 0;
	i.mesh_info.primitive_type     = primitive_type::triangles;
	i.mesh_info.face_type          = counter_clockwise_faces_ ? face_type::front_cw : face_type::front_ccw;
	i.mesh_info.flags              = 0;
	i.mesh_info.bounding_sphere    = i.positions ? calc_sphere(i.positions, sizeof(float3), i.mesh_info.num_vertices) : float4(0.0f, 0.0f, 0.0f, 0.0f);
	i.mesh_info.extents            = (data_.aabb_max - data_.aabb_min) * 0.5f;
	i.mesh_info.avg_edge_length    = 1.0f;
	i.mesh_info.avg_texel_density  = float2(1.0f, 1.0f);
	i.mesh_info.layout[0]          = celement_t(element_semantic::position, 0, surface::format::r32g32b32_float, 0);
	i.mesh_info.layout[1]          = celement_t(element_semantic::normal,   0, surface::format::r32g32b32_float, 1);
	i.mesh_info.layout[2]          = celement_t(element_semantic::texcoord, 0, surface::format::r32g32b32_float, 2);
	
	lod_t lod;
	lod.opaque_color.start_subset  = 0; lod.opaque_color.num_subsets  = i.mesh_info.num_subsets;
	lod.opaque_shadow.start_subset = 0; lod.opaque_shadow.num_subsets = i.mesh_info.num_subsets;
	lod.collision.start_subset     = 0; lod.collision.num_subsets     = i.mesh_info.num_subsets;

	i.mesh_info.lods.fill(lod);

	return i;
}

void mesh::initialize(const init_t& init
	, const path_t& obj_path
	, const char* obj_string
	, const allocator& mesh_alloc
	, const allocator& temp_alloc)
{
	impl_ = mesh_alloc.allocate(sizeof(mesh_impl), "mesh_impl");
	new (impl_) mesh_impl(init, obj_path, obj_string, mesh_alloc, temp_alloc);
}

void mesh::deinitialize()
{
	if (impl_)
	{
		auto impl      = (mesh_impl*)impl_;
		auto std_alloc = impl->indices_.get_allocator();
		std_alloc.alloc_.destroy(impl);
		impl_          = nullptr;
	}
}

info_t mesh::info() const
{
	auto impl = (mesh_impl*)impl_;
	return impl->info();
}

bool from_string_opt(float3* dst, const char** pstart)
{
	const char* c = *pstart;
	move_past_line_whitespace(&++c);
	if (!from_string(dst, c))
	{
		if (!from_string((float2*)dst, c))
		{
			if (!from_string((float*)&dst, c))
				return false;

			move_to_whitespace(&c);
		}

		else
		{
			move_next_word(&c);
			move_to_whitespace(&c);
		}
	}

	else
	{
		move_next_word(&c);
		move_next_word(&c);
		move_to_whitespace(&c);
	}

	*pstart = c;
	return true;
}

static texture_info parse_texture_info(const char* _TextureLine)
{
	texture_info i;
	const char* c = _TextureLine;
	while (*c)
	{
		if (*c == '-' && *(c-1) == ' ')
		{
			c++;
			if (*c == 'o')
			{
				if (!from_string_opt(&i.origin_offset, &c))
					break;
			}

			else if (*c == 's')
			{
				if (!from_string_opt(&i.scale, &c))
					break;
			}

			else if (*c == 't')
			{
				if (!from_string_opt(&i.turbulance, &c))
					break;
			}

			else if (*c == 'm' && *(c+1) == 'm')
			{
				c += 2;
				move_past_line_whitespace(&++c);
				if (!from_string(&i.brightness_gain, c))
					break;

				move_next_word(&c);
				move_to_whitespace(&c);
			}

			else if (*c == 'b' && *(c+1) == 'm')
			{
				c += 2;
				move_past_line_whitespace(&++c);
				if (!from_string(&i.bump_multiplier, c))
					break;

				move_to_whitespace(&c);
			}

			else if (!_memicmp(c, "blendu", 6))
			{
				c += 6;
				move_past_line_whitespace(&c);
				i.blendu = !!_memicmp(c, "on", 2);
				c += i.blendu ? 2 : 3;
			}

			else if (!_memicmp(c, "blendv", 6))
			{
				c += 6;
				move_past_line_whitespace(&c);
				i.blendv = !!_memicmp(c, "on", 2);
				c += i.blendv ? 2 : 3;
			}

			else if (!_memicmp(c, "boost", 5))
			{
				c += 5;
				move_past_line_whitespace(&c);
				if (!from_string(&i.boost, c))
					break;

				move_to_whitespace(&c);
			}

			else if (!_memicmp(c, "texres", 6))
			{
				c += 6;
				move_past_line_whitespace(&c);
				if (!from_string(&i.resolution, c))
					break;

				move_next_word(&c);
				move_to_whitespace(&c);
			}

			else if (!_memicmp(c, "clamp", 5))
			{
				c += 5;
				move_past_line_whitespace(&c);
				i.blendv = !!_memicmp(c, "on", 2);
				c += i.blendv ? 2 : 3;
			}

			else if (!_memicmp(c, "imfchan", 7))
			{
				c += 7;
				move_past_line_whitespace(&c);
				i.imfchan = *c++;
			}

			else if (!_memicmp(c, "type", 4))
			{
				c += 4;
				move_past_line_whitespace(&c);

				if (!from_string(&i.type, c))
					break;
			}
		}
		else
		{
			move_past_line_whitespace(&c);
			i.path = c;
			return i;
		}
	}

	oThrow(std::errc::io_error, "error parsing obj texture info");
}

template<typename allocT>
static void parse_materials(std::vector<material_info, allocT>& materials, const path_t& mtl_path, const char* mtl_string)
{
	// NOTE: mtl_path is not used yet, but leave this in case we need
	// to report better errors.

	material_info i;

	float3* pColor = nullptr;
	char type = 0;
	char buf[_MAX_PATH];

	const char* r = mtl_string;
	while (*r)
	{
		move_past_line_whitespace(&r);
		switch (*r)
		{
			case 'n':
				r += 6; // "newmtl"
				move_past_line_whitespace(&r);
				sscanf_s(r, "%[^\r|^\n]", buf, (int)countof(buf));
				materials.resize(materials.size() + 1);
				materials.back().name = buf;
				break;
			case 'N':
				type = *(++r);
				sscanf_s(++r, "%f", type == 's' ? &materials.back().specularity : &materials.back().refraction_index);
				break;
			case 'T': // Tr
				type = *(++r);
				if (type == 'f')
				{
					float3* col = (float3*)&materials.back().transmission_color;
					sscanf_s(++r, "%f %f %f", &col->x, &col->y, &col->z);
				}
				else if (type == 'r') 
					sscanf_s(++r, "%f", &materials.back().transparency); // 'd' or 'Tr' are the same (transparency)
				break;
			case 'd':
				sscanf_s(++r, "%f", &materials.back().transparency);
				break;
			case 'K':
				if (r[1] == 'm') break; // I can't find Km documented anywhere.
				switch (r[1])
				{
					case 'a': pColor = (float3*)&materials.back().ambient_color; break;
					case 'e': pColor = (float3*)&materials.back().emissive_color; break;
					case 'd': pColor = (float3*)&materials.back().diffuse_color; break;
					case 's': pColor = (float3*)&materials.back().specular_color; break;
					default: break;
				}
				sscanf_s(r+2, "%f %f %f", &pColor->x, &pColor->y, &pColor->z);
				break;
			case 'm':
				sscanf_s(r + strcspn(r, "\t "), "%[^\r|^\n]", buf, (int)countof(buf));

				#define oOBJTEX(_Name) do \
				{	oCheck(parse_texture_info(buf + 1, &materials.back()._Name), std::errc::io_error, "Failed to parse \"%s\"", r); \
				} while(false)
				
				if (!_memicmp("map_Ka", r, 6)) materials.back().ambient = parse_texture_info(buf + 1);
				else if (!_memicmp("map_Kd", r, 6)) materials.back().diffuse = parse_texture_info(buf + 1);
				else if (!_memicmp("map_Ks", r, 6)) materials.back().specular = parse_texture_info(buf + 1);
				else if (!_memicmp("map_d", r, 5) || 0 == _memicmp("map_opacity", r, 11)) materials.back().alpha = parse_texture_info(buf + 1);
				else if (!_memicmp("map_Bump", r, 8) || 0 == _memicmp("bump", r, 4)) materials.back().bump = parse_texture_info(buf + 1);
				break;
			case 'i':
				r += 5; // "illum"
				move_past_line_whitespace(&r);
				sscanf_s(r, "%hhu", &materials.back().illum);
				break;
			default:
				break;
		}
		move_to_line_end(&r);
		move_past_newline(&r);
	}
}

struct material_lib_impl
{
	material_lib_impl(const path_t& mtl_path, const char* mtl_string, const allocator& alloc);
	material_lib_info info() const;

	std::vector<material_info, ouro_std_allocator<material_info>> materials_;
	path_t                                                        path_;
};

material_lib_impl::material_lib_impl(const path_t& mtl_path, const char* mtl_string, const allocator& alloc)
	: materials_(ouro_std_allocator<material_info>(alloc, "material_lib"))
{
	materials_.reserve(16);
	parse_materials(materials_, mtl_path, mtl_string);
}

material_lib_info material_lib_impl::info() const
{
	material_lib_info i;
	i.materials = materials_.data();
	i.num_materials = (uint32_t)materials_.size();
	i.mtl_path = path_;
	return i;
}

void material_lib::initialize(const path_t& mtl_path, const char* mtl_string, const allocator& alloc)
{
	impl_ = alloc.allocate(sizeof(material_lib_impl), "material_lib_impl");
	new (impl_) material_lib_impl(mtl_path, mtl_string, alloc);
}

void material_lib::deinitialize()
{
	if (impl_)
	{
		auto impl      = (material_lib_impl*)impl_;
		auto std_alloc = impl->materials_.get_allocator();
		std_alloc.alloc_.destroy(impl);
		impl_          = nullptr;
	}
}

material_lib_info material_lib::info() const
{
	auto impl = (material_lib_impl*)impl_;
	return impl->info();
}

}}}