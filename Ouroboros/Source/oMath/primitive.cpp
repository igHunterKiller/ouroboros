// Copyright(c) 2015 Antony Arciuolo. See License.txt regarding use.

#include <oMath/primitive.h>
#include <oCore/byte.h>
#include <oCore/countof.h>
#include <algorithm>
#include <stdexcept>

#include <oCore/assert.h>

namespace ouro { namespace primitive {

static const float PI = 3.14159265358979323846f;
static inline void sincosf(float radians, float& out_sin, float& out_cos) { out_sin = sinf(radians); out_cos = cosf(radians); }
static inline uint16_t pow(uint16_t a, uint16_t e)  { uint16_t v = 1; for (uint16_t i = 0; i < e; i++) v *= a; return v; }
static float clampf(float val, float min_, float max_) { return std::min(std::max(val, min_), max_); }
static float saturatef(float val) { return clampf(val, 0.0f, 1.0f); }
static void clamp_facet(uint16_t& facet) { if (facet < 3) facet = 3; }

static void apply_cylindrical_texgen(uint16_t* indices, uint32_t nindices, float* positions, float* texcoords, uint16_t& inout_nvertices, bool fix_seam = true)
{
	static const float apex  =  1.0f;
	static const float nadir = -1.0f;

	float* p = positions;
	float* t = texcoords;

	for (int i = 0; i < inout_nvertices; i++, p += 3, t += 2)
		cylindrical_texgen(p[0], p[1], p[2], nadir, apex, t, t + 1);

	if (fix_seam)
	{
		// fix seams where 0.9-ish wraps back to 0: duplicate a vertex and set its texcoord to 1
		
		p = positions;
		t = texcoords;

		static const float max_u_delta = 0.5f;
		for (uint32_t i = 0; i < nindices; i += 3)
		{
			fix_cylindrical_seam(max_u_delta, indices[i+0], indices[i+1], p, t, inout_nvertices);
			fix_cylindrical_seam(max_u_delta, indices[i+0], indices[i+2], p, t, inout_nvertices);
			fix_cylindrical_seam(max_u_delta, indices[i+1], indices[i+2], p, t, inout_nvertices);
		}
	}
}

uint32_t info_t::total_bytes() const
{
	uint32_t n = sizeof(uint16_t) * nindices + 3 * sizeof(float) * nvertices;
	if (type == tessellation_type::textured)
		n += 2 * sizeof(float) * nvertices;
	return n;
}

mesh_t::mesh_t(const info_t& info, void* memory)
{
	positions   = (float*)memory;
	memory      = (uint8_t*)memory + 3 * sizeof(float) * info.nvertices;
	if (info.type == tessellation_type::textured)
	{
		texcoords = (float*)memory;
		memory    = (uint8_t*)memory + 2 * sizeof(float) * info.nvertices;
	}
	else
		texcoords = nullptr;
	indices     = (uint16_t*)memory;
}

const mesh_t& mesh_t::operator+=(const info_t& that)
{
	if (indices  ) indices   += that.nindices;
	if (positions) positions += that.nvertices * 3;
	if (texcoords) texcoords += that.nvertices * 2;
	return *this;
}
	
// _____________________________________________________________________________
// Starting out

static const float first_tri_positions[] = {

	-0.750f, -0.667f, 0.000f,
	 0.000f,  0.667f, 0.000f,
	 0.750f, -0.667f, 0.000f,
};

static const uint16_t first_tri_line_indices[] = 
{
	0, 1, 1, 2, 2, 0,
};

static const uint16_t first_tri_face_indices[] = 
{
	0, 1, 2,
};

info_t first_tri_info(const tessellation_type& type)
{
	// todo: add textured
	if (type == tessellation_type::textured || type == tessellation_type::cubemapped)
		oThrow(std::errc::invalid_argument, "textured and cubemapped are unsupported");

	info_t i;
	i.nindices  = type == tessellation_type::lines ? 6 : 3;
	i.nvertices = countof(first_tri_positions) / 3;
	i.maxdiv    = 7;
	i.type      = type;
	return i;
}

cmesh_t first_tri_mesh(const tessellation_type& type)
{
	if (type == tessellation_type::textured || type == tessellation_type::cubemapped)
		oThrow(std::errc::invalid_argument, "textured and cubemapped are unsupported");

	cmesh_t m;
	m.indices   = type == tessellation_type::lines ? first_tri_line_indices : first_tri_face_indices;
	m.positions = first_tri_positions;
	m.texcoords = nullptr;
	return m;
}

// _____________________________________________________________________________
// Platonic Solids

// 4 faces, 6 edges
static const uint16_t tetrahedron_indices[] = 
{
	0, 1, 2,
	0, 2, 3,
	0, 3, 1,
	1, 3, 2,
};

static const float tetrahedron_positions[] = 
{
	 0.000f,  0.000f,  1.000f,
	 0.943f,  0.000f, -0.333f,
	-0.471f,  0.816f, -0.333f,
	-0.471f, -0.816f, -0.333f,
};

// 6 faces, 12 edges
static const uint16_t hexahedron_indices[] = 
{
	7, 4, 5, 7, 5, 6,
	5, 1, 2, 5, 2, 6,
	7, 3, 4, 0, 4, 3,
	7, 2, 3, 7, 6, 2,
	0, 1, 4, 4, 1, 5,
	0, 3, 1, 1, 3, 2,
};

static const float hexahedron_positions[] = 
{
	-0.816497f, -0.471404f, -0.333333f,
	 0.000000f,  0.000000f, -1.000000f,
	 0.000000f,  0.942809f, -0.333333f,
	-0.816497f,  0.471404f,  0.333333f,
	 0.000000f, -0.942809f,  0.333333f,
	 0.816497f, -0.471404f, -0.333333f,
	 0.816497f,  0.471404f,  0.333333f,
	 0.000000f,  0.000000f,  1.000000f,
};

// 8 faces, 12 edges
const uint16_t octahedron_indices[] = 
{
	 0, 2, 1, // tfl
	 0, 3, 2, // tfr
	 0, 4, 3, // tbr
	 0, 1, 4, // tbl
	 5, 1, 2, // bfl
	 5, 2, 3, // bfr
	 5, 3, 4, // bbr
	 5, 4, 1, // bbl
};

static const float octahedron_positions[] = 
{
	 0.0f,  0.0f,  1.0f,
	-1.0f,  0.0f,  0.0f,
	 0.0f,  1.0f,  0.0f,
	 1.0f,  0.0f,  0.0f,
	 0.0f, -1.0f,  0.0f,
	 0.0f,  0.0f, -1.0f,
};

// 20 faces, 30 edges
static const uint16_t icosahedron_indices[] =
{
	0, 1, 2, 0, 2, 3, 0, 3, 4, 0, 4, 5, 0, 5, 1,     // top cap
	3, 2, 11, 2, 1, 10, 1, 5, 9, 5, 4, 8, 4, 3, 7,   // triangles with base in top cap, apex in bottom cap
	11, 7, 3, 10, 11, 2, 9, 10, 1, 8, 9, 5, 7, 8, 4, // triangles with base in bottom cap, apex in top cap
	6, 8, 7, 6, 9, 8, 6, 10, 9, 6, 11, 10, 6, 7, 11, // bottom cap
};

static const float icosahedron_positions[] =
{
	 0.000000f,  0.000000f,  1.000000f,
	 0.894427f,  0.000000f,  0.447214f,
	 0.276393f,  0.850651f,  0.447214f,
	-0.723607f,  0.525731f,  0.447214f,
	-0.723607f, -0.525731f,  0.447214f,
	 0.276393f, -0.850651f,  0.447214f,
	 0.000000f,  0.000000f, -1.000000f,
	-0.894427f, -0.000000f, -0.447214f,
	-0.276393f, -0.850651f, -0.447214f,
	 0.723607f, -0.525731f, -0.447214f,
	 0.723607f,  0.525731f, -0.447214f,
	-0.276393f,  0.850651f, -0.447214f,
};

// 12 faces, 30 edges
static const uint16_t dodecahedron_indices[] =
{
	4, 12, 13, 4, 13, 5, 4, 5, 9, 6, 14, 15, 6, 15, 7, 6, 7, 10, 
	1, 13, 12, 1, 12, 0, 1, 0, 8, 3, 15, 14, 3, 14, 2, 
	3, 2, 11, 8, 0, 16, 8, 16, 3, 8, 3, 11, 11, 2, 17, 
	11, 17, 1, 11, 1, 8, 10, 7, 19, 10, 19, 4, 10, 4, 9, 
	9, 5, 18, 9, 18, 6, 9, 6, 10, 6, 18, 17, 6, 17, 2, 
	6, 2, 14, 1, 17, 18, 1, 18, 5, 1, 5, 13, 3, 16, 19, 
	3, 19, 7, 3, 7, 15, 4, 19, 16, 4, 16, 0, 4, 0, 12, 
};

static const float dodecahedron_positions[] = 
{
	-0.816497f, -0.471404f, -0.333333f,
	 0.000000f,  0.000000f, -1.000000f,
	 0.000000f,  0.942809f, -0.333333f,
	-0.816497f,  0.471404f,  0.333333f,
	 0.000000f, -0.942809f,  0.333333f,
	 0.816497f, -0.471404f, -0.333333f,
	 0.816497f,  0.471404f,  0.333333f,
	 0.000000f,  0.000000f,  1.000000f,
	-0.660560f,  0.090030f, -0.745356f,
	 0.660560f, -0.672718f,  0.333333f,
	 0.660560f, -0.090030f,  0.745356f,
	-0.660560f,  0.672718f, -0.333333f,
	-0.252311f, -0.908420f, -0.333334f,
	 0.252311f, -0.617076f, -0.745356f,
	 0.252311f,  0.908420f,  0.333334f,
	-0.252311f,  0.617076f,  0.745356f,
	-0.912871f, -0.235702f,  0.333333f,
	 0.408248f,  0.527047f, -0.745356f,
	 0.912871f,  0.235702f, -0.333333f,
	-0.408248f, -0.527047f,  0.745356f,
};

#define PLATONIC(name, max_div, apex_, nadir_)                                                        \
  info_t name##_info(const tessellation_type& type)                                                   \
  {	info_t i;																	                                                        \
    i.nindices  = countof(name##_indices); 			                                                      \
    i.nvertices = countof(name##_positions) / 3;                                                      \
    i.maxdiv    = max_div;											                                                      \
    i.apex      = apex_;												                                                      \
    i.nadir     = nadir_;	                                                                            \
    i.type      = type;                                                                               \
    return i;                                                                                         \
  }                                                                                                   \
  cmesh_t name##_mesh()                                                                               \
  {	cmesh_t m;                                                                                        \
    m.indices   = name##_indices;                                                                     \
    m.positions = name##_positions;                                                                   \
    return m;                                                                                         \
	}                                                                                                   \
	void name##_tessellate(mesh_t* out_mesh, const tessellation_type& type)                             \
	{ if (type == tessellation_type::lines)                                                             \
			oThrow(std::errc::invalid_argument, "lines not supported for " #name);                                  \
		if (out_mesh->indices) memcpy(out_mesh->indices, name##_indices, sizeof(name##_indices));         \
		if (out_mesh->positions) memcpy(out_mesh->positions, name##_positions, sizeof(name##_positions)); \
		if (out_mesh->texcoords && (type == tessellation_type::textured || type == tessellation_type::cubemapped)) \
		{	info_t   info   = name##_info(tessellation_type::solid);                                        \
			uint16_t nverts = info.nvertices;                                                               \
			apply_cylindrical_texgen(out_mesh->indices, info.nindices, out_mesh->positions, out_mesh->texcoords, nverts, false); \
			oTrace("Actual verts: %u", nverts);                                                             \
		}                                                                                                 \
	}

PLATONIC(tetrahedron,  6, 0, 0xffff)
PLATONIC(hexahedron,   5, 7, 1)
PLATONIC(octahedron,   5, 0, 5)
PLATONIC(icosahedron,  5, 0, 6)
PLATONIC(dodecahedron, 4, 7, 1)

// _____________________________________________________________________________
// Cube

static const float cube_creased_positions[] = {
	-1.0f,  1.0f, -1.0f, // LTN
	-1.0f,  1.0f,  1.0f, // LTF
	-1.0f, -1.0f, -1.0f, // LBN
	-1.0f, -1.0f,  1.0f, // LBF
	 1.0f,  1.0f, -1.0f, // RTN
	 1.0f,  1.0f,  1.0f, // RTF
	 1.0f, -1.0f, -1.0f, // RBN
	 1.0f, -1.0f,  1.0f, // RBF

	-1.0f,  1.0f, -1.0f, // LTN  // repeat verts for creased cube support
	-1.0f,  1.0f,  1.0f, // LTF
	-1.0f, -1.0f, -1.0f, // LBN
	-1.0f, -1.0f,  1.0f, // LBF
	 1.0f,  1.0f, -1.0f, // RTN
	 1.0f,  1.0f,  1.0f, // RTF
	 1.0f, -1.0f, -1.0f, // RBN
	 1.0f, -1.0f,  1.0f, // RBF

	-1.0f,  1.0f, -1.0f, // LTN
	-1.0f,  1.0f,  1.0f, // LTF
	-1.0f, -1.0f, -1.0f, // LBN
	-1.0f, -1.0f,  1.0f, // LBF
	 1.0f,  1.0f, -1.0f, // RTN
	 1.0f,  1.0f,  1.0f, // RTF
	 1.0f, -1.0f, -1.0f, // RBN
	 1.0f, -1.0f,  1.0f, // RBF
};

static const uint16_t cube_line_indices[] = 
{
	0, 1, 1, 3, 3, 2, 2, 0, // Left
	4, 5, 5, 7, 7, 6, 6, 4, // Right
	0, 4, 1, 5, // Top
	2, 6, 3, 7, // Bottom
};

static const uint16_t cube_face_indices[] = 
{
	0, 2, 1, 1, 2, 3, // Left
	4, 5, 7, 7, 6, 4, // Right
	0, 1, 5, 5, 4, 0, // Top
	3, 2, 7, 7, 2, 6, // Bottom
	0, 4, 6, 6, 2, 0, // Near
	1, 3, 7, 7, 5, 1, // Far
};

static const uint16_t cube_creased_face_indices[] = 
{
	0, 2, 1, 1, 2, 3, // Left
	4, 5, 7, 7, 6, 4, // Right

	0+8, 1+8, 5+8, 5+8, 4+8, 0+8, // Top
	3+8, 2+8, 7+8, 7+8, 2+8, 6+8, // Bottom
	
	0+16, 4+16, 6+16, 6+16, 2+16, 0+16, // Near
	1+16, 3+16, 7+16, 7+16, 5+16, 1+16, // Far
};

static uint16_t cube_edge_indices[] =
{
	0, 1, 1, 3, 
	3, 2, 2, 0,
	4, 5, 5, 7, 
	7, 6, 6, 4,
	0, 4, 1, 5,
	2, 6, 3, 7,
};

static uint16_t cube_face_adjacency_indices[] = 
{
	0, 2, 0, 5, 
	0, 3, 0, 4,
	1, 2, 1, 5, 
	1, 3, 1, 4,
	2, 4, 2, 5,
	3, 4, 3, 5,
};

static const float cube_creased_texcoords[] = 
{
	// Left/Right
	0.0f, 1.0f, // LTN
	1.0f, 1.0f, // LTF
	0.0f, 0.0f, // LBN
	1.0f, 0.0f, // LBF
	1.0f, 1.0f, // RTN
	0.0f, 1.0f, // RTF
	1.0f, 0.0f, // RBN
	0.0f, 0.0f, // RBF

	// Top/Bottom
	0.0f, 1.0f, // LTN
	0.0f, 0.0f, // LTF
	0.0f, 0.0f, // LBN
	0.0f, 1.0f, // LBF
	1.0f, 1.0f, // RTN
	1.0f, 0.0f, // RTF
	1.0f, 0.0f, // RBN
	1.0f, 1.0f, // RBF

	// Near/Far
	1.0f, 1.0f, // LTN
	0.0f, 1.0f, // LTF
	1.0f, 0.0f, // LBN
	0.0f, 0.0f, // LBF
	0.0f, 1.0f, // RTN
	1.0f, 1.0f, // RTF
	0.0f, 0.0f, // RBN
	1.0f, 0.0f, // RBF
};

info_t cube_info(const tessellation_type& type)
{
	info_t i;
	i.nindices  = type == tessellation_type::lines ? countof(cube_line_indices) : countof(cube_face_indices);
	i.nvertices = countof(cube_creased_positions) / 3;
	i.maxdiv    = 6;
	i.type      = type;

	// the array describes replicated vertices, so only use the first set
	if (type == tessellation_type::lines || type == tessellation_type::solid)
		i.nvertices /= 3;

	return i;
}

void cube_tessellate(mesh_t* out_mesh, const tessellation_type& type)
{
	switch (type)
	{
		case tessellation_type::lines:
			if (out_mesh->indices)   memcpy(out_mesh->indices,   cube_line_indices,      sizeof(cube_line_indices));
			if (out_mesh->positions) memcpy(out_mesh->positions, cube_creased_positions, sizeof(cube_creased_positions) / 3);
			break;

		case tessellation_type::solid:
			if (out_mesh->indices)   memcpy(out_mesh->indices,   cube_face_indices,      sizeof(cube_face_indices));
			if (out_mesh->positions) memcpy(out_mesh->positions, cube_creased_positions, sizeof(cube_creased_positions) / 3);
			break;

		// todo: reverse texcoords for textured version
		case tessellation_type::textured:
		case tessellation_type::cubemapped:
			if (out_mesh->indices)   memcpy(out_mesh->indices,   cube_creased_face_indices, sizeof(cube_creased_face_indices));
			if (out_mesh->positions) memcpy(out_mesh->positions, cube_creased_positions,    sizeof(cube_creased_positions));
			if (out_mesh->texcoords) memcpy(out_mesh->texcoords, cube_creased_texcoords,    sizeof(cube_creased_texcoords));
			break;
	}
}

const uint16_t* cube_edges()
{
	return cube_edge_indices;
}

const uint16_t* cube_face_adjacencies()
{
	return cube_face_adjacency_indices;
}

// _____________________________________________________________________________
// Rounded

info_t circle_info(const tessellation_type& type, uint16_t facet)
{
	clamp_facet(facet);

	info_t i;
	i.nindices  = type == tessellation_type::lines ? (2 * facet) : (3 * (facet - 2));
	i.nvertices = facet;
	i.type      = type;
	return i;
}

static info_t cyltube_info(const tessellation_type& type, uint16_t facet)
{
	if (type == tessellation_type::cubemapped)
		oThrow(std::errc::invalid_argument, "cubemapped texcoords not supported for cylinder");

	clamp_facet(facet);

	info_t i;
	i.nindices  = type == tessellation_type::lines ? (2 * facet) : (6 * facet);
	i.nvertices = 2 * facet;
	i.type      = type;

	// duplicate verts along the seam to fix up texcoords
	if (type == tessellation_type::textured || type == tessellation_type::cubemapped)
		i.nvertices += 3;

	return i;
}

info_t cone_info(const tessellation_type& type, uint16_t facet)
{
	clamp_facet(facet);

	if (type == tessellation_type::textured || type == tessellation_type::cubemapped)
	{
		if (type == tessellation_type::cubemapped)
			oThrow(std::errc::invalid_argument, "cubemapped texcoords not supported for cone");

		auto cyl = cyltube_info(type, facet);
		auto cir = circle_info (type, facet);

		info_t i;
		i.nindices  = cyl.nindices  + cir.nindices;
		i.nvertices = cyl.nvertices + cir.nvertices;
		i.type      = type;
		return i;
	}

	info_t i;
	i.nindices  = type == tessellation_type::lines ? (2 * facet) : (6 * (facet - 1));
	i.nvertices = facet + 1;
	i.type      = type;
	return i;
}

info_t cylinder_info(const tessellation_type& type, uint16_t facet)
{
	if (type == tessellation_type::cubemapped)
		oThrow(std::errc::invalid_argument, "cubemapped texcoords not supported for cylinder");

	clamp_facet(facet);

	info_t cyl = cyltube_info(type, facet);
	info_t cir = circle_info (type, facet);

	info_t i;
	i.nindices  = cyl.nindices  + 2 * cir.nindices;
	i.nvertices = cyl.nvertices + 2 * cir.nvertices;
	i.type      = type;
	return i;
}

info_t torus_info(const tessellation_type& type, uint16_t facet, uint16_t divide)
{
	if (type == tessellation_type::cubemapped)
		oThrow(std::errc::invalid_argument, "cubemapped texcoords not supported for torus");

	if (type == tessellation_type::lines)
	{
		auto cir = circle_info(type, facet);

		info_t i;
		i.nindices  = cir.nindices  * 4;
		i.nvertices = cir.nvertices * 4;
		i.type      = type;
		return i;
	}

	clamp_facet(facet);

	uint16_t n  = divide * (facet + 1) * 2;

	info_t i;
	i.nindices  = type == tessellation_type::lines ? 0 : (3 * (n - 2));
	i.nvertices = n;
	i.type      = type;
	return i;
}

static void circle_tessellate1(mesh_t* out_mesh, const tessellation_type& type, uint16_t facet, float z, float radius, uint16_t base_index)
{
	if (out_mesh->indices)
	{
		if (type == tessellation_type::lines)
		{
			uint16_t* line = out_mesh->indices;
			uint16_t j = facet - 1;
			for (uint16_t i = 0; i < facet; j = i++)
			{
				*line++ = base_index + j;
				*line++ = base_index + i;
			}
		}

		else
		{
			const uint16_t nfaces = facet - 2;

			// define a strip across the polygon
			uint16_t  i     = 1;
			uint16_t  j     = facet - 1;
			uint16_t  n     = 1;
			uint16_t* strip = (uint16_t*)alloca(sizeof(uint16_t) * facet);
			strip[0]        = 0;

			while (i <= j)
			{
				if (n & 0x1) strip[n] = i++;
				else         strip[n] = j--;
				n++;
			}

			// unstrip
			uint16_t* face = out_mesh->indices;
			for (uint16_t f = 0; f < nfaces; f++)
			{
				uint16_t a = f & 0x1;
				*face++ = base_index + strip[f+0];
				*face++ = base_index + strip[f+1+a];
				*face++ = base_index + strip[f+2-a];
			}
		}
	}

	if (out_mesh->positions)
	{
		float* position    = out_mesh->positions;
		float* texcoord    = out_mesh->texcoords;
		const float step   = (2.0f * PI) / (float)facet;
		float s            = 0.0f;
		float inv_diameter = 1.0f / (2.0f * radius);

		for (uint16_t i = 0; i < facet; i++, s += step)
		{
			float ss, cs;
			sincosf(s, ss, cs);

			float x = radius * cs;
			float y = radius * ss;

			*position++ = x;
			*position++ = y;
			*position++ = z;

			if (texcoord)
			{
				*texcoord++ = (x + radius) * inv_diameter;
				*texcoord++ = (y + radius) * inv_diameter;
			}
		}
	}
}

void circle_tessellate(mesh_t* out_mesh, const tessellation_type& type, uint16_t facet, float radius, uint16_t base_index)
{
	clamp_facet(facet);
	circle_tessellate1(out_mesh, type, facet, 0.0f, saturatef(radius), base_index);
}

static void cyltube_tessellate(mesh_t* out_mesh, const tessellation_type& type, uint16_t facet, float apex_radius, uint16_t base_index)
{
	const info_t cir = circle_info(type, facet);

	if (out_mesh->indices)
	{
		if (type == tessellation_type::lines)
		{
			uint16_t* line = out_mesh->indices;
			const uint16_t n = cir.nvertices;
			const uint16_t step = n >= 10 ? 4 : 1; // don't have so many lines
			for (uint16_t v = 0; v < n; v += step)
			{
				*line++ = base_index + v;
				*line++ = base_index + v + n;
			}
		}

		else
		{
			uint16_t*     face = out_mesh->indices;
			const info_t cyl  = cyltube_info(type, facet);
		
			uint16_t prev_base_bot = cir.nvertices - 1;
			for (uint16_t seg = 0; seg < cir.nvertices; seg++)
			{
				uint16_t base_bot      = seg;
				uint16_t prev_base_top = prev_base_bot + cir.nvertices;
				uint16_t base_top      = base_bot      + cir.nvertices;

				*face++ = prev_base_bot; *face++ = prev_base_top; *face++ = base_bot;
				*face++ = base_bot;      *face++ = prev_base_top; *face++ = base_top;

				prev_base_bot = base_bot;
			}
		}
	}

	if (out_mesh->positions)
	{
		mesh_t cir_mesh;
		cir_mesh.positions  = out_mesh->positions;
		circle_tessellate1(&cir_mesh, type, facet, 1.0f, apex_radius, 0);
		cir_mesh.positions += 3 * cir.nvertices;
		circle_tessellate1(&cir_mesh, type, facet, -1.0f, 1.0f, 0);
	}

	if (out_mesh->texcoords && out_mesh->indices)
	{
		uint16_t nverts = 2 * cir.nvertices;
		auto     cyl    = cyltube_info(type, facet);
		apply_cylindrical_texgen(out_mesh->indices, cyl.nindices, out_mesh->positions, out_mesh->texcoords, nverts);
	}
}

void cone_tessellate(mesh_t* out_mesh, const tessellation_type& type, uint16_t facet, uint16_t base_index)
{
	clamp_facet(facet);

	const info_t cir = circle_info(type, facet);

	if (out_mesh->texcoords || type == tessellation_type::textured || type == tessellation_type::cubemapped)
	{
		mesh_t m = *out_mesh;
		cyltube_tessellate(&m, type, facet, 0.00001f, base_index);
		auto cyl = cyltube_info(type, facet);
		m       += cyl;
		circle_tessellate1(&m, type, facet, -1.0f, 1.0f, base_index + cyl.nvertices);

		for (uint32_t i = 0; i < cir.nindices; i += 3)
			std::swap(m.indices[i+1], m.indices[i+2]);

		return;
	}

	mesh_t cir_mesh;

	if (out_mesh->indices)
	{
		if (type == tessellation_type::lines)
		{
			uint16_t* line = out_mesh->indices;

			const uint16_t step = cir.nvertices >= 10 ? 4 : 1; // don't have so many lines

			for (uint16_t i = 1; i <= cir.nvertices; i += step)
			{
				*line++ = base_index;
				*line++ = base_index + i;
			}
		}

		else
		{
			uint16_t* face = out_mesh->indices;

			uint16_t j = cir.nvertices;
			for (uint16_t i = 1; i <= cir.nvertices; j = i++)
			{
				*face++ = base_index + 0;
				*face++ = base_index + j;
				*face++ = base_index + i;
			}

			cir_mesh.indices = face;
		}
	}

	if (out_mesh->positions)
	{
		float* position = out_mesh->positions;
		*position++ = 0.0f;
		*position++ = 0.0f;
		*position++ = 1.0f;

		cir_mesh.positions = position;
	}

	circle_tessellate1(&cir_mesh, type, facet, -1.0f, 1.0f, 1);
	
	if (cir_mesh.indices && type != tessellation_type::lines)
	{
		for (uint32_t i = 0; i < cir.nindices; i += 3)
			std::swap(cir_mesh.indices[i+1], cir_mesh.indices[i+2]);
	}
}

void cylinder_tessellate(mesh_t* out_mesh, const tessellation_type& type, uint16_t facet, float apex_radius, uint16_t base_index)
{
	apex_radius = saturatef(apex_radius);

	cyltube_tessellate(out_mesh, type, facet, apex_radius, base_index);

	info_t cyl = cyltube_info(type, facet);
	info_t cir = circle_info (type, facet);

	mesh_t cap = *out_mesh;

	cap += cyl;
	circle_tessellate1(&cap, type, facet, -1.0f, 1.0f, cyl.nvertices);
	
	if (cap.indices && type != tessellation_type::lines)
		for (uint32_t i = 0; i < cir.nindices; i += 3)
			std::swap(cap.indices[i+1], cap.indices[i+2]);

	cap += cir;
	circle_tessellate1(&cap, type, facet, 1.0f, apex_radius, cyl.nvertices + cir.nvertices);
}

void torus_tessellate(mesh_t* out_mesh, const tessellation_type& type, uint16_t facet, uint16_t divide, float inner_radius, uint16_t base_index)
{
	clamp_facet(facet);
	inner_radius = saturatef(inner_radius);

	if (type == tessellation_type::lines)
	{
		// @tony add divide indicator

		float tube_radius   = (1.0f - inner_radius) * 0.5f;
		float center_radius = (1.0f + inner_radius) * 0.5f;

		mesh_t m = *out_mesh;

		auto cir = circle_info(type, facet);
		circle_tessellate1(&m, type, facet, 0.0f,         inner_radius,  cir.nvertices * 0); m += cir;
		circle_tessellate1(&m, type, facet, 0.0f,         1.0f,          cir.nvertices * 1); m += cir;
		circle_tessellate1(&m, type, facet,  tube_radius, center_radius, cir.nvertices * 2); m += cir;
		circle_tessellate1(&m, type, facet, -tube_radius, center_radius, cir.nvertices * 3);

		return;
	}

	if (out_mesh->indices)
	{
		const info_t tor = torus_info(type, facet, divide);
		uint16_t* face = out_mesh->indices;

		const uint16_t maxi = tor.nvertices - 2;
		for (uint16_t i = 0; i < maxi; i++)
		{
			uint16_t a = 1 + (i & 1);
			uint16_t b = 2 - (i & 1);
		
			*face++ = base_index + i;
			*face++ = base_index + i + a;
			*face++ = base_index + i + b;
		}
	}

	if (out_mesh->positions)
	{
		float* position = out_mesh->positions;
		float* texcoord = out_mesh->texcoords;

		const float outer_radius = 1.0f;

		// Revisit inner and outer
		// http://mathworld.wolfram.com/Torus.html
		const float center_radius = (inner_radius + outer_radius) * 0.5f;
		const float range_radius  =  outer_radius - center_radius;
		const float inv_divide    = 1.0f / static_cast<float>(divide);
		const float inv_facet     = 1.0f / static_cast<float>(facet);
		const float dstep         = (2.0f * PI) * inv_divide;
		const float fstep         = (2.0f * PI) * inv_facet;

		for (uint16_t i = 0; i < divide; i++)
		{
			float S[2], C[2];
			sincosf(dstep *  i,      S[0], C[0]);
			sincosf(dstep * (i + 1), S[1], C[1]);

			for (uint16_t j = 0; j <= facet; j++)
			{
				float fs, fc;
				sincosf((j % facet) * fstep, fs, fc);

				for (uint16_t k = 0; k < 2; k++)
				{
					*position++ = (center_radius + range_radius * fc) * C[k];
					*position++ = (center_radius + range_radius * fc) * S[k];
					*position++ = range_radius * fs;

					if (texcoord)
					{
						*texcoord++ = inv_divide * (i + k);
						*texcoord++ = inv_facet  * j;
					}
				}
			}
		}
	}
}

void deindex(mesh_t* out_mesh, const mesh_t& src_mesh, const info_t& src_info)
{
	float*          position      = out_mesh->positions;
	float*          texcoord      = out_mesh->texcoords;
	const uint16_t* src_indices   = src_mesh.indices;
	const float*    src_positions = src_mesh.positions;
	const float*    src_texcoords = src_mesh.texcoords;

	if (out_mesh->indices)
		oThrow(std::errc::invalid_argument, "deindexed output should not have indices");

	if (!src_indices)
		oThrow(std::errc::invalid_argument, "source must have indices");

	if (src_info.type == tessellation_type::lines)
	{
		for (uint16_t i = 0; i < src_info.nindices; i++, src_indices++)
		{
			*position++ = src_positions[*src_indices * 3 + 0];
			*position++ = src_positions[*src_indices * 3 + 1];
			*position++ = src_positions[*src_indices * 3 + 2];

			if (texcoord && src_texcoords)
			{
				*texcoord++ = src_texcoords[*src_indices * 2 + 0];
				*texcoord++ = src_texcoords[*src_indices * 2 + 1];
			}
		}
	}

	else
	{
		for (uint16_t i = 0; i < src_info.nindices; i++, src_indices++)
		{
			*position++ = src_positions[*src_indices * 3 + 0];
			*position++ = src_positions[*src_indices * 3 + 1];
			*position++ = src_positions[*src_indices * 3 + 2];
			
			if (texcoord && src_texcoords)
			{
				*texcoord++ = src_texcoords[*src_indices * 2 + 0];
				*texcoord++ = src_texcoords[*src_indices * 2 + 1];
			}
		}
	}
}

void cylindrical_texgen(float x, float y, float z, float min_z, float max_z, float* out_u, float* out_v)
{
	*out_u = 1.0f - (atan2(x, y) + PI) / (2.0f * PI);
	*out_v = (z - min_z) / (max_z - min_z);
}

void toroidal_texgen(float x, float y, float z, float outer_radius, float* out_u, float* out_v)
{
//v = arccos (Y/R)/2p
//u = [arccos ((X/(R + r*cos(2 pv))]2p

	static const float TWO_PI = 2.0f * PI;

	*out_u = 1.0f - (atan2(x, y) + PI) / TWO_PI;

	float len = sqrt(x * x + y * y);
	float xx  = len - outer_radius;
	*out_v    = (atan2(z, xx) + PI) / TWO_PI;
}

static uint16_t duplicate_vertex(uint16_t src_index, float* positions, float* texcoords, uint16_t& inout_nvertices)
{
	// returns index of newly duplicated vertex (source at src_index)
	positions[3 * inout_nvertices + 0] = positions[3 * src_index + 0];
	positions[3 * inout_nvertices + 1] = positions[3 * src_index + 1];
	positions[3 * inout_nvertices + 2] = positions[3 * src_index + 2];

	if (texcoords)
	{
		texcoords[2 * inout_nvertices + 0] = texcoords[2 * src_index + 0];
		texcoords[2 * inout_nvertices + 1] = texcoords[2 * src_index + 1];
	}

	uint16_t duped_index = inout_nvertices++;

	return duped_index;
}

void fix_polar_seam(uint16_t pole, uint16_t* indices, uint16_t nindices, float* positions, float* texcoords, uint16_t& inout_nvertices)
{
	for (uint16_t i = 0; i < nindices; i += 3)
	{
		uint16_t& a = indices[i+0];
		uint16_t& b = indices[i+1];
		uint16_t& c = indices[i+2];

		if (a != pole && b != pole && c != pole)
			continue;

		// rotate until a has apex
		while (a != pole)
		{
			uint16_t tmp = a;
			a = b;
			b = c;
			c = tmp;
		}

		uint16_t new_index = duplicate_vertex(a, positions, texcoords, inout_nvertices);
		a = new_index;

		      float* auv = texcoords + 2 * a;
		const float* buv = texcoords + 2 * b;
		const float* cuv = texcoords + 2 * c;

		auv[0] = (buv[0] + cuv[0]) * 0.5f;
	}
}

void fix_cylindrical_seam(float max_u_delta, uint16_t& a, uint16_t& b, float* positions, float* texcoords, uint16_t& inout_nvertices)
{
	const float& u0 = texcoords[2 * a];
	const float& u1 = texcoords[2 * b];

	if (fabs(u0 - u1) > max_u_delta)
	{
		uint16_t& min_idx = u0 < u1 ? a : b;
		min_idx = duplicate_vertex(min_idx, positions, texcoords, inout_nvertices);
		texcoords[2 * min_idx] += 1.0f;
	}
}

}}
