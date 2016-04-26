// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Library for handling buffers of irregular networks of points, lines, indices and triangles.

#pragma once
#include <oMesh/element.h>
#include <oMesh/face.h>
#include <oMath/quantize.h>
#include <oSurface/surface.h>
#include <array>

namespace ouro { namespace mesh {

// describes a part of a larger mesh that is associated with unique attributes/materials.
// this is usually what maps to one draw call.
struct subset_t
{
	enum flags : uint16_t
	{
		skinned  = 1<<0,
		atested  = 1<<1,
		blended  = 1<<2,
		dblsided = 1<<3,
		face_ccw = 1<<4,
	};

	uint32_t start_index;     // offset into an index buffer where the indices for this subset starts
	uint32_t num_indices;     // how many indices this runs for
	uint32_t start_vertex;    // offset into a vertex buffer where this subset starts
	uint16_t subset_flags;    // bitmask of the subset flags above
	uint16_t unused;          // available for future usage
	uint64_t material_id;     // hash/id for the material associated with this subset
};
static_assert(sizeof(subset_t) == 24, "size mismatch");

// describes the run of subsets for a particular use case
// meshes should be sorted by opaque, alpha, etc.
struct subset_range_t
{
	subset_range_t(uint16_t start = 0, uint16_t num = 0) : start_subset(start), num_subsets(num) {}

	uint16_t start_subset;
	uint16_t num_subsets;
};
static_assert(sizeof(subset_range_t) == 4, "size mismatch");

// describes all subsets
struct lod_t
{
	subset_range_t opaque_color;
	subset_range_t atest_color;
	subset_range_t blend_color;
	subset_range_t opaque_shadow;
	subset_range_t atest_shadow;
	subset_range_t blend_shadow;
	subset_range_t collision;
};
static_assert(sizeof(lod_t) == 28, "size mismatch");

// describes top-level data about a mesh
struct info_t
{
	info_t()
		: num_indices(0)
		, num_vertices(0)
		, num_subsets(0)
		, num_slots(0)
		, log2scale(0)
		, primitive_type(primitive_type::unknown)
		, face_type(face_type::unknown)
		, flags(0)
		, bounding_sphere(0.0f, 0.0f, 0.0f, 0.0f)
		, extents(0.0f, 0.0f, 0.0f)
		, avg_edge_length(0.0f)
		, lod_distances(10.0f, 50.0f, 125.0f, 300.0f)
		, avg_texel_density(0.0f, 0.0f)
	{}

	uint32_t             num_indices;
	uint32_t             num_vertices;
	uint16_t             num_subsets;
	uint8_t              num_slots;         // number of slots used in layout
	uint8_t              log2scale;         // for ushort4 compressed verts: [-32768,32767] -> [-1,1] * 2^n where n is this number.
	primitive_type       primitive_type;
	face_type            face_type;
	uint16_t             flags;
	float4               bounding_sphere;   // inscribed in the aabb
	float3               extents;           // forms an aabb from the sphere's center
	float                avg_edge_length;
	float4               lod_distances;
	float2               avg_texel_density;
	layout_t             layout;
	std::array<lod_t, 5> lods;
};
static_assert(sizeof(info_t) == (72 + sizeof(layout_t) + 5*sizeof(lod_t)), "size mismatch");

// _____________________________________________________________________________
// Copy/convert

// Copies from the src to the dst + dst_byte_offset and does any appropriate format conversion.
// If no copy can be done the values are set to zero. This returns dst_byte_offset + size of dst_type.
uint32_t copy_element(uint32_t dst_byte_offset, void* oRESTRICT dst, uint32_t dst_stride, const surface::format& dst_format,
									 const void* oRESTRICT src, uint32_t src_stride, const surface::format& src_format, uint32_t num_vertices);

// Uses copy_element to find a src for each dst and copies it in (or sets to zero).
void copy_vertices(void* oRESTRICT* oRESTRICT dst, const layout_t& dst_elements, const void* oRESTRICT* oRESTRICT src, const layout_t& src_elements, uint32_t num_vertices);

// _____________________________________________________________________________
// Transform

void transform_points(const float4x4& matrix, float3* oRESTRICT dst, uint32_t dst_stride, const float3* oRESTRICT src, uint32_t src_stride, uint32_t num_points);
void transform_vectors(const float4x4& matrix, float3* oRESTRICT dst, uint32_t dst_stride, const float3* oRESTRICT src, uint32_t src_stride, uint32_t num_vectors);

// _____________________________________________________________________________
// Calculation

inline uint32_t num_primitives(const info_t& info) { return num_primitives(info.primitive_type, info.num_indices, info.num_vertices); }

void calc_aabb(const float3* oRESTRICT vertices, uint32_t vertex_stride, uint32_t num_vertices, float3* oRESTRICT out_min, float3* oRESTRICT out_max);
float4 calc_sphere(const float3* vertices, uint32_t vertex_stride, uint32_t num_vertices);

uint8_t calc_log2scale(const float3& aabb_extents);

// scan an index list for its lowest value
void minmax_index(const uint16_t* oRESTRICT indices, uint32_t num_indices, uint32_t* oRESTRICT out_min_index, uint32_t* oRESTRICT out_max_index);
void minmax_index(const uint32_t* oRESTRICT indices, uint32_t num_indices, uint32_t* oRESTRICT out_min_index, uint32_t* oRESTRICT out_max_index);

// Calculate the face normals from the following inputs:
// face_normals: output, array to fill with normals. This should be at least as
//                large as the number of faces in the specified mesh (num_indices/3)
// indices: array of triangles (every 3 specifies a triangle)
// num_indices: The number of indices in the indices array
// positions: list of XYZ positions for the mesh that are indexed by the index array
// num_positions: The number of vertices in the positions array
// ccw: If true triangles are assumed to have their front-face be specified by the counter-
//       clockwise order of vertices in a triangle. This affects which way a normal points.
void calc_face_normals(float3* oRESTRICT face_normals, const uint32_t* oRESTRICT indices, uint32_t num_indices, const float3* oRESTRICT positions, uint32_t num_positions, bool ccw = false);
void calc_face_normals(float3* oRESTRICT face_normals, const uint16_t* oRESTRICT indices, uint32_t num_indices, const float3* oRESTRICT positions, uint32_t num_positions, bool ccw = false);

// Calculates the vertex normals by averaging face normals from the following 
// inputs:
// vertex_normals: output, array to fill with normals. This should be at least 
//                 as large as the number of vertices in the specified mesh
//                 (_NumberOfVertices).
// indices: array of triangles (every 3 specifies a triangle)
// num_indices: The number of indices in the indices array
// positions: list of XYZ positions for the mesh that are indexed by the index 
//            array
// num_positions: The number of vertices in the positions array
// ccw: If true, triangles are assumed to have their front-face be specified by
//       the counter-clockwise order of vertices in a triangle. This affects 
//       which way a normal points.
// overwrite_all: Overwrites any pre-existing data in the array. If this is 
// false, any zero-length vector will be overwritten. Any length-having vector
// will not be touched.
// This can return EINVAL if a parameters isn't something that can be used.
void calc_vertex_normals(float3* vertex_normals, const uint32_t* indices, uint32_t num_indices, const float3* positions, uint32_t num_positions, bool ccw = false, bool overwrite_all = true);
void calc_vertex_normals(float3* vertex_normals, const uint16_t* indices, uint32_t num_indices, const float3* positions, uint32_t num_positions, bool ccw = false, bool overwrite_all = true);

// Calculates the tangent space vector and its handedness from the following
// inputs:
// tangents: output, array to fill with tangents. This should be at least as large as the number of vertices 
//           in the specified mesh (num_vertices)
// indices: array of triangles (every 3 specifies a triangle)
// num_indices: The number of indices in the indices array
// positions: list of XYZ positions for the mesh that are indexed by the index array
// normals: list of normalized normals for the mesh that are indexed by the index array
// texcoords: list of texture coordinates for the mesh that are indexed by the index array
// num_vertices: The number of vertices in the positions, normals and texcoords arrays
void calc_vertex_tangents(float4* tangents, const uint32_t* indices, uint32_t num_indices, const float3* positions, const float3* normals, const float3* texcoords, uint32_t num_vertices);
void calc_vertex_tangents(float4* tangents, const uint32_t* indices, uint32_t num_indices, const float3* positions, const float3* normals, const float2* texcoords, uint32_t num_vertices);
void calc_vertex_tangents(float4* tangents, const uint16_t* indices, uint32_t num_indices, const float3* positions, const float3* normals, const float3* texcoords, uint32_t num_vertices);
void calc_vertex_tangents(float4* tangents, const uint16_t* indices, uint32_t num_indices, const float3* positions, const float3* normals, const float2* texcoords, uint32_t num_vertices);

// Fills out_texcoords with texture coordinates calculated using LCSM. The 
// pointer should be allocated to have at least num_vertices elements. If 
// out_solve_time is specified the number of seconds to calculate texcoords will 
// be returned.

// NOTE: No LCSM code or middleware has been integrated, so these will only throw
// operation_not_supported right now.
void calc_texcoords(const float3& aabb_min, const float3& aabb_max, const uint32_t* indices, uint32_t num_indices, const float3* positions, float2* out_texcoords, uint32_t num_vertices, double* out_solve_time);
void calc_texcoords(const float3& aabb_min, const float3& aabb_max, const uint32_t* indices, uint32_t num_indices, const float3* positions, float3* out_texcoords, uint32_t num_vertices, double* out_solve_time);
void calc_texcoords(const float3& aabb_min, const float3& aabb_max, const uint16_t* indices, uint32_t num_indices, const float3* positions, float2* out_texcoords, uint32_t num_vertices, double* out_solve_time);
void calc_texcoords(const float3& aabb_min, const float3& aabb_max, const uint16_t* indices, uint32_t num_indices, const float3* positions, float3* out_texcoords, uint32_t num_vertices, double* out_solve_time);

// Allocates and fills an edge list for the mesh described by the specified indices:
// num_vertices: The number of vertices the index array indexes
// indices: array of triangles (every 3 specifies a triangle)
// num_indices: The number of indices in the indices array
// _ppEdges: a pointer to receive an allocation and be filled with index pairs 
//           describing an edge. Use oFreeEdgeList() to free memory the edge 
//           list allocation. So every two uints in *_ppEdges represents an edge.
// out_num_edges: a pointer to receive the number of edge pairs returned
void calc_edges(uint32_t num_vertices, const uint32_t* indices, uint32_t num_indices, uint32_t** _ppEdges, uint32_t* out_num_edges);

// Frees the buffer allocated by calc_edges
void free_edge_list(uint32_t* edges);

// _____________________________________________________________________________
// Pruning

// Removes indices for degenerate triangles. After calling this function use prune_unindexed_vertices() to clean up extra vertices.
// positions: list of XYZ positions indexed by the index array
// num_vertices: The number of vertices in the positions array
// indices: array of triangles (every 3 specifies a triangle)
// num_indices: The number of indices in the indices array
// out_new_num_indices: The new number of indices as a result of removed degenerages
void prune_degenerates(const float3* oRESTRICT positions, uint32_t num_positions, uint32_t* oRESTRICT indices, uint32_t num_indices, uint32_t* oRESTRICT out_new_num_indices);
void prune_degenerates(const float3* oRESTRICT positions, uint32_t num_positions, uint16_t* oRESTRICT indices, uint32_t num_indices, uint32_t* oRESTRICT out_new_num_indices);

#define PUV_PARAMS(IndexT, UV0T, UV1T) const IndexT* oRESTRICT indices, uint32_t num_indices \
	, float3* oRESTRICT positions, float3* oRESTRICT normals, float4* oRESTRICT tangents \
	, UV0T* oRESTRICT texcoords0, UV1T* oRESTRICT texcoords1, uint32_t* oRESTRICT colors \
	, uint32_t num_vertices, uint32_t* oRESTRICT out_new_num_vertices

// Given the parameters as described in the above macro, contract the vertex element arrays
// to remove any vertex not indexed by the specified indices. Call this after remove_degenerates.
void prune_unindexed_vertices(PUV_PARAMS(uint32_t, float2, float2));
void prune_unindexed_vertices(PUV_PARAMS(uint32_t, float2, float3));
void prune_unindexed_vertices(PUV_PARAMS(uint32_t, float3, float2));
void prune_unindexed_vertices(PUV_PARAMS(uint32_t, float3, float3));
void prune_unindexed_vertices(PUV_PARAMS(uint16_t, float2, float2));
void prune_unindexed_vertices(PUV_PARAMS(uint16_t, float2, float3));
void prune_unindexed_vertices(PUV_PARAMS(uint16_t, float3, float2));
void prune_unindexed_vertices(PUV_PARAMS(uint16_t, float3, float3));

// _____________________________________________________________________________
// Interpolation

inline float3   lerp_positions(const float3& a, const float3& b, float s) { return lerp(a, b, s); }
inline float3   lerp_normals(const float3& a, const float3& b, float s) { return normalize(lerp(a, b, s)); }
inline float4   lerp_tangents(const float4& a, const float4& b, float s) { return float4(normalize(lerp(a.xyz(), b.xyz(), s)), a.w); }
inline float2   lerp_texcoords(const float2& a, const float2& b, float s) { return lerp(a, b, s); }
inline float3   lerp_texcoords(const float3& a, const float3& b, float s) { return lerp(a, b, s); }
inline uint32_t lerp_colors(const uint32_t& a, const uint32_t& b, float s) { float4 aa = truetofloat4(a); float4 bb = truetofloat4(b); float4 lerped = lerp(aa, bb, s); return float4totrue(lerped); }

}}
