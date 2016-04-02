// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Primitives are always tessellated to fit inside a unit cube ([-1,1] in each axis)
// All platonics have an single apex and nadir along the Z axis.
// All rounded primitives are radially symmetric in XY plane and extruded along the Z axis.
// To minimize artifacts textured or cubemapped primitives will have additional vertices: 
// don't assume the same index/vertex count between lines, solids, textured and cubemapped.

#pragma once
#include <cstdint>
#include <oMath/cube_topology.h>

namespace ouro { namespace primitive {

enum class tessellation_type : uint8_t
{
	lines,      // positions only, indices are line pairs
	solid,      // positions only, indices are triangle triplets
	textured,   // positions & texcoords, indices are triangle triplets
	cubemapped, // positions & texcoords assigned as viewed from inside rather than outside, indices are triangle triplets

	count,
};

struct info_t
{
	info_t() : nindices(0), nvertices(0), maxdiv(0xffff), apex(0xffff), nadir(0xffff), type(tessellation_type::solid) {} 
	uint32_t total_bytes() const; // total memory to allocate to pass to the mesh_t ctor
	
	uint16_t          nindices;  // either nlines * 2 or ntris * 3
	uint16_t          nvertices; // number of positions, texcoords
	uint16_t          maxdiv;    // maximum edge-midpoint subdivision such that vertex count remains less than 65536
	uint16_t          apex;      // index of vertex at (0, 0, 1)
	uint16_t          nadir;     // index of vertex at (0, 0,-1)
	tessellation_type type;
};

struct mesh_t
{
	mesh_t() : indices(nullptr), positions(nullptr), texcoords(nullptr) {}
	mesh_t(const info_t& info, void* memory);
	const mesh_t& operator+=(const info_t& that); // increment by an info, useful for writing multiple primitives into one allocation

	uint16_t* indices;   // pairs for lines, triplets for triangles
	float*    positions; // xyz triplets (nvertices * 3)
	float*    texcoords; // uv pairs     (nvertices * 2)
};

struct cmesh_t
{
	cmesh_t() : indices(nullptr), positions(nullptr), texcoords(nullptr) {} 
	
	const uint16_t* indices;   // pairs for lines, triplets for triangles
	const float*    positions; // xyz triplets (nvertices * 3)
	const float*    texcoords; // uv pairs     (nvertices * 2)
};

// _____________________________________________________________________________
// Starting out

// Use this when bringing up a renderer. This is meant to be rendered with 
// identity view/projection.

info_t  first_tri_info(const tessellation_type& type);
cmesh_t first_tri_mesh(const tessellation_type& type);

// _____________________________________________________________________________
// Platonic Solids

// Apex and nadir are always at extreme Z values, so the hexahedron is a cube
// rotated to be consistent with other platonics. For frusta or pyramids use
// the cube API rather than hexahedron. Only solid tessellation is supported.

// Note: textured platonics when subdivided will propagate texcoords assigned
// at the low-resolution. If using these to subdivide into a sphere, choose
// solid, subdivide, apply cylindrical texcoords, then fix seams for proper
// results.

info_t tetrahedron_info (const tessellation_type& type);
info_t hexahedron_info  (const tessellation_type& type);
info_t octahedron_info  (const tessellation_type& type);
info_t icosahedron_info (const tessellation_type& type);
info_t dodecahedron_info(const tessellation_type& type);

cmesh_t tetrahedron_mesh();
cmesh_t hexahedron_mesh();
cmesh_t octahedron_mesh();
cmesh_t icosahedron_mesh();
cmesh_t dodecahedron_mesh();

void tetrahedron_tessellate (mesh_t* out_mesh, const tessellation_type& type);
void hexahedron_tessellate  (mesh_t* out_mesh, const tessellation_type& type);
void octahedron_tessellate  (mesh_t* out_mesh, const tessellation_type& type);
void icosahedron_tessellate (mesh_t* out_mesh, const tessellation_type& type);
void dodecahedron_tessellate(mesh_t* out_mesh, const tessellation_type& type);

// _____________________________________________________________________________
// Cube

// Unit cube. This has extended information that is valid for frusta and pyramids.
// Textured cubes have each corner vertex replicated so texture coordinates are
// planar mapped to each face consistent with D3D cubemap order.

info_t cube_info(const tessellation_type& type);

void cube_tessellate(mesh_t* out_mesh, const tessellation_type& type = tessellation_type::solid);

// 12 pair (24) indices representing the edges of a cube
const uint16_t* cube_edges();

// 12 pair (24) face indices (oCUBE_LEFT, oCUBE_TOP, etc.), same order as 
// cube_edges() described faces adjacent to those edges.
const uint16_t* cube_face_adjacencies();

// _____________________________________________________________________________
// Rounded

// Base radius is always 1.0. Radius parameters are clamped to 1.0, do scaling
// separately.

info_t circle_info  (const tessellation_type& type, uint16_t facet);
info_t cone_info    (const tessellation_type& type, uint16_t facet);
info_t cylinder_info(const tessellation_type& type, uint16_t facet);
info_t torus_info   (const tessellation_type& type, uint16_t facet, uint16_t divide);

void circle_tessellate  (mesh_t* out_mesh, const tessellation_type& type, uint16_t facet,                   float radius = 1.0f, uint16_t base_index = 0);
void cone_tessellate    (mesh_t* out_mesh, const tessellation_type& type, uint16_t facet,                                        uint16_t base_index = 0);
void cylinder_tessellate(mesh_t* out_mesh, const tessellation_type& type, uint16_t facet,                   float apex_radius,   uint16_t base_index = 0);
void torus_tessellate   (mesh_t* out_mesh, const tessellation_type& type, uint16_t facet,  uint16_t divide, float inner_radius,  uint16_t base_index = 0);

// _____________________________________________________________________________
// Utility

// Create a triangle-only mesh: ensure enough positions/texcoords memory is allocated.
void deindex(mesh_t* out_mesh, const mesh_t& src_mesh, const info_t& src_info);

// Projects UVs assuming a cylinder with circular faces on the Z axis and radial
// symmetry in the XY plane.
void cylindrical_texgen(float x, float y, float z, float min_z, float max_z, float* out_u, float* out_v);

// Platonics have singluar poles, which receive one texcoord. Duplicate the vertex and 
// average associated triangle UVs to reduce the disparity in texture mapping.
void fix_polar_seam(uint16_t pole, uint16_t* indices, uint16_t nindices, float* positions, float* texcoords, uint16_t& inout_nvertices);

// Using cylindrical texgen can leave a seam where u >= 0.9 and then wraps back around
// to zero: the HW interprets the texcoords going from 0.9 to 0.5 to 0.0 (i.e. backwards)
// rather than wrapping as expected. This function duplicates such vertices and increments
// the texcoords by U += 1 which provides the proper interpretation.
void fix_cylindrical_seam(float max_u_delta, uint16_t& a, uint16_t& b, float* positions, float* texcoords, uint16_t& inout_nvertices);

}}
