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

static const float3 ZERO3(0.0f, 0.0f, 0.0f);

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

	std::vector<float3> positions;
	std::vector<float3> normals;
	std::vector<float3> texcoords;
	
	// these are assigned after negative indices have been handled, but are 
	// otherwise directly-from-file values.
	std::vector<face> faces;

	std::vector<group_t> groups;
	std::vector<subset_t> subsets;

	path_string mtl_path;
};

// Given a string that starts with the letter 'v', parse as a line of vector
// values and push_back into the appropriate vector.
static const char* parse_vline(const char* vline
	, bool flip_handedness
	, std::vector<float3>& positions
	, std::vector<float3>& normals
	, std::vector<float3>& texcoords
	, float3& min_position
	, float3& max_positions)
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
			max_positions = max(max_positions, temp);
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
static const char* parse_fline(const char* fline
	, uint32_t num_positions
	, uint32_t num_normals
	, uint32_t num_texcoords
	, uint32_t num_groups
	, std::vector<face>& faces)
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
			uint32_t ZeroBasedIndexFromFile = 0;

			// Negative indices implies the max indexable vertex OBJ will be INT_MAX, 
			// not UINT_MAX.
			if (*fline == '-')
			{
				int NegativeIndex = atoi(fline);
				uint32_t NumElements = 0;
				switch (semantic)
				{
					case face::position: NumElements = num_positions; break;
					case face::normal: NumElements = num_normals; break;
					case face::texcoord: NumElements = num_texcoords; break;
				}
				ZeroBasedIndexFromFile = NumElements + NegativeIndex;
			}
			else
				ZeroBasedIndexFromFile = atoi(fline) - 1;

			f.index[f.num_indices][semantic] = ZeroBasedIndexFromFile;

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
static void parse_elements(const char* obj_string, bool flip_handedness, vertex_data* _pElements)
{
	uint32_t NumGroups = 0;
	group_t group;

	const char* line = obj_string;
	while (*line)
	{
		move_past_line_whitespace(&line);
		switch (*line)
		{
			case 'v':
				line = parse_vline(line, flip_handedness, _pElements->positions, _pElements->normals, _pElements->texcoords, _pElements->aabb_min, _pElements->aabb_max);
				break;
			case 'f':
				line = parse_fline(line
					, (uint32_t)_pElements->positions.size()
					, (uint32_t)_pElements->normals.size()
					, (uint32_t)_pElements->texcoords.size()
					, (uint32_t)_pElements->groups.size()
					, _pElements->faces);
				break;
			case 'g':
				// close out previous group
				if (NumGroups)
					_pElements->groups.push_back(group); 
				NumGroups++;
				parse_string(group.group_name, line);
				break;
			case 'u':
				parse_string(group.material_name, line);
				break;
			case 'm':
				parse_string(_pElements->mtl_path, line);
				break;
			default:
				break;
		}
		move_to_line_end(&line);
		move_past_newline(&line);
	}

	// close out a remaining group one last time
	// NOTE: start prim / num prims are handled later after vertices have been reduced
	if (NumGroups)
		_pElements->groups.push_back(group);

	else
	{
		group.group_name = "Default Group";
		_pElements->groups.push_back(group);
	}
}

// Using config data and an index hash, reduce all duplicate/face data into 
// unique indexed data. This must be called after ALL SourceElements have been
// populated.
static void reduce_elements(const init_t& init
	, index_map_t* index_map
	, const vertex_data& src_elements
	, std::vector<uint32_t>* indices
	, vertex_data* singly_indexed_elements
	, std::vector<uint32_t>* degenerate_normals
	, std::vector<uint32_t>* degenerate_texcoords)
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
						singly_indexed_elements->normals.push_back(ZERO3);
					}
				}

				if (!src_elements.texcoords.empty())
				{
					if(f.index[p][face::texcoord] != invalid_index)
						singly_indexed_elements->texcoords.push_back(src_elements.texcoords[f.index[p][face::texcoord]]);
					else
					{
						degenerate_texcoords->push_back(NewIndex);
						singly_indexed_elements->texcoords.push_back(ZERO3);
					}
				}
			
				(*index_map)[hash] = resolvedIndices[p] = NewIndex;
			}
		
			else
				resolvedIndices[p] = (*it).second;
		} // for each face element

		// Now that the index has either been added or reset to a pre-existing index, 
		// add the face definition to the index list

		static const uint32_t CCWindices[6] = { 0, 2, 1, 2, 0, 3, };
		static const uint32_t CWindices[6] = { 0, 1, 2, 2, 3, 0, };
	
		bool UseCCW = init.counter_clockwise_faces;
		if (init.flip_handedness)
			UseCCW = !UseCCW;
	
		const uint32_t* pOrder = UseCCW ? CCWindices : CWindices;

		indices->push_back(resolvedIndices[pOrder[0]]);
		indices->push_back(resolvedIndices[pOrder[1]]);
		indices->push_back(resolvedIndices[pOrder[2]]);

		// Add another triangle for the rest of the quad
		if (f.num_indices == 4)
		{
			indices->push_back(resolvedIndices[pOrder[3]]);
			indices->push_back(resolvedIndices[pOrder[4]]);
			indices->push_back(resolvedIndices[pOrder[5]]);
		}
	} // for each face

	// close out last group
	last_range_index = src_elements.faces.back().group_range_index;
	auto& s = singly_indexed_elements->subsets[last_range_index];
	s.num_indices = (uint32_t)indices->size() - s.start_index;
}

class mesh_impl : public mesh
{
public:
	mesh_impl(const init_t& init, const path_t& obj_path, const char* obj_string);
	info_t info() const override;

private:
	vertex_data data_;
	std::vector<uint32_t> indices_;
	path_t path_;
	bool counter_clockwise_faces_;
};

mesh_impl::mesh_impl(const init_t& init, const path_t& obj_path, const char* obj_string)
	: path_(obj_path)
	, counter_clockwise_faces_(init.counter_clockwise_faces)
{
	const size_t kInitialReserve = init.est_num_indices * sizeof(uint32_t);
	size_t IndexMapMallocBytes = 0;
	
	std::vector<uint32_t> DegenerateNormals, DegenerateTexcoords;
	DegenerateNormals.reserve(1000);
	DegenerateTexcoords.reserve(1000);

	// Scope memory usage... more might be used below when calculating normals, 
	// etc. so free this stuff up rather than leaving it all around.
	
	{
		void* pArena = malloc(kInitialReserve);
		oFinally { if (pArena) free(pArena); };
		linear_allocator Allocator(pArena, kInitialReserve);
		index_map_t IndexMap(0, index_map_t::hasher(), index_map_t::key_equal(), std::less<key_t>()
			, allocator_type(&Allocator, &IndexMapMallocBytes));

		// OBJ files don't contain a same-sized vertex streams for each elements, 
		// and indexing occurs uniquely between vertex elements. Eventually we will 
		// create same-sized vertex data - even by replication of data - so that a 
		// single index buffer can be used. That's the this->VertexElements, but 
		// first to get the raw vertex data off disk, use this.
		vertex_data OffDiskElements;
		OffDiskElements.reserve(init.est_num_vertices, init.est_num_indices / 3);
		parse_elements(obj_string, init.flip_handedness, &OffDiskElements);

		const uint32_t kEstMaxVertexElements = (uint32_t)max(max(OffDiskElements.positions.size(), OffDiskElements.normals.size()), OffDiskElements.texcoords.size());
		data_.reserve(kEstMaxVertexElements, init.est_num_indices / 3);
		indices_.reserve(init.est_num_indices);

		reduce_elements(init, &IndexMap, OffDiskElements, &indices_, &data_, &DegenerateNormals, &DegenerateTexcoords);

	} // End of life for from-disk vertex elements and index hash

	#ifdef _DEBUG
		if (IndexMapMallocBytes)
		{
			mstring reserved, additional;
			const char* n = oSAFESTRN(obj_path); // passing the macro directly causes win32 to throw an exception
			oTrace("obj: %s index map allocated %s additional indices beyond the initial est_num_vertices=%s", n, format_commas(additional, (uint32_t)(IndexMapMallocBytes / sizeof(uint32_t))), format_commas(reserved, init.est_num_vertices));
		}
	#endif

	if (init.calc_normals_on_error)
	{
		bool CalcNormals = false;
		if (data_.normals.empty())
		{
			oTrace("obj: No normals found in %s...", obj_path);
			CalcNormals = true;
		}

		else if (!DegenerateNormals.empty())
		{
			// @tony: Is there a way to calculate only the degenerates?
			oTrace("oOBJ: %u degenerate normals in %s...", DegenerateNormals.size(), obj_path);
			CalcNormals = true;
		}

		if (CalcNormals)
		{
			oTrace("Calculating vertex normals... (%s)", oSAFESTRN(obj_path));
			sstring StrTime;
			timer t;
			data_.normals.resize(data_.positions.size());
			calc_vertex_normals(data_.normals.data(), indices_.data(), (uint32_t)indices_.size(), data_.positions.data(), (uint32_t)data_.positions.size(), init.counter_clockwise_faces, true);
			format_duration(StrTime, t.seconds(), true, true);
			oTrace("Calculating vertex normals done in %s. (%s)", StrTime.c_str(), oSAFESTRN(obj_path));
		}
	}

	if (init.calc_texcoords_on_error)
	{
		bool CalcTexcoords = false;
		if (data_.texcoords.empty())
		{
			oTrace("oOBJ: No texcoords found in %s...", oSAFESTRN(obj_path));
			CalcTexcoords = true;
		}

		else if (!DegenerateTexcoords.empty())
		{
			oTrace("oOBJ: %u degenerate texcoords in %s...", DegenerateTexcoords.size(), oSAFESTRN(obj_path));
			CalcTexcoords = true;
		}

		if (CalcTexcoords)
		{
			data_.texcoords.resize(data_.positions.size());
			sstring StrTime;
			double SolverTime = 0.0;
			oTrace("Calculating texture coordinates... (%s)", oSAFESTRN(obj_path));
			try { calc_texcoords(data_.aabb_min, data_.aabb_max, indices_.data(), (uint32_t)indices_.size(), data_.positions.data(), data_.texcoords.data(), (uint32_t)data_.texcoords.size(), &SolverTime); }
			catch (std::exception& e)
			{
				e;
				data_.texcoords.clear();
				oTraceA("Calculating texture coordinates failed. %s (%s)", obj_path, e.what());
			}

			format_duration(StrTime, SolverTime, true, true);
			oTrace("Calculating texture coordinates done in %s. (%s)", StrTime.c_str(), oSAFESTRN(obj_path));
		}
	}
}

std::shared_ptr<mesh> mesh::make(const init_t& init, const path_t& obj_path, const char* obj_string)
{
	return std::make_shared<mesh_impl>(init, obj_path, obj_string);
}

info_t mesh_impl::info() const
{
	info_t i;
	i.obj_path = path_;
	i.mtl_path = data_.mtl_path;
	i.groups = data_.groups.data();
	i.subsets = data_.subsets.data();
	i.indices = indices_.data();
	i.positions = data_.positions.empty() ? nullptr : data_.positions.data();
	i.normals = data_.normals.empty() ? nullptr : data_.normals.data();
	i.texcoords = data_.texcoords.empty() ? nullptr : data_.texcoords.data();

	i.mesh_info.num_indices = (uint32_t)indices_.size();
	i.mesh_info.num_vertices = (uint32_t)data_.positions.size();
	i.mesh_info.num_subsets = (uint16_t)data_.groups.size();
	i.mesh_info.num_slots = 3; // each element is put in its own slots: obj supports pos, nrm, tan
	i.mesh_info.log2scale = 0;
	i.mesh_info.primitive_type = primitive_type::triangles;
	i.mesh_info.face_type = counter_clockwise_faces_ ? face_type::front_cw : face_type::front_ccw;
	i.mesh_info.flags = 0;

	i.mesh_info.bounding_sphere = i.positions ? calc_sphere(i.positions, sizeof(float3), i.mesh_info.num_vertices) : float4(0.0f, 0.0f, 0.0f, 0.0f);
	i.mesh_info.extents = (data_.aabb_max - data_.aabb_min) * 0.5f;
	i.mesh_info.avg_edge_length = 1.0f;
	i.mesh_info.avg_texel_density = float2(1.0f, 1.0f);
	i.mesh_info.layout[0] = celement_t(element_semantic::position, 0, surface::format::r32g32b32_float,    0);
	i.mesh_info.layout[1] = celement_t(element_semantic::normal,   0, surface::format::r32g32b32_float,    1);
	i.mesh_info.layout[2] = celement_t(element_semantic::tangent,  0, surface::format::r32g32b32a32_float, 2);
	
	lod_t lod;
	lod.opaque_color.start_subset = 0;
	lod.opaque_color.num_subsets = i.mesh_info.num_subsets;
	lod.opaque_shadow.start_subset = 0;
	lod.opaque_shadow.num_subsets = i.mesh_info.num_subsets;
	lod.collision.start_subset = 0;
	lod.collision.num_subsets = i.mesh_info.num_subsets;
	i.mesh_info.lods.fill(lod);

	return i;
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

static void parse_materials(std::vector<material_info>& materials, const path_t& mtl_path, const char* mtl_string)
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

class material_lib_impl : public material_lib
{
public:
	material_lib_impl(const path_t& mtl_path, const char* mtl_string);
	material_lib_info get_info() const override;
private:
	std::vector<material_info> Materials;
	path_t Path;
};

material_lib_impl::material_lib_impl(const path_t& mtl_path, const char* mtl_string)
{
	Materials.reserve(16);
	parse_materials(Materials, mtl_path, mtl_string);
}

std::shared_ptr<material_lib> material_lib::make(const path_t& mtl_path, const char* mtl_string)
{
	return std::make_shared<material_lib_impl>(mtl_path, mtl_string);
}

material_lib_info material_lib_impl::get_info() const
{
	material_lib_info i;
	i.materials = Materials.data();
	i.num_materials = (uint32_t)Materials.size();
	i.mtl_path = Path;
	return i;
}

}}}