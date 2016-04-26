// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oMesh/mesh.h>
#include <oCore/bit.h>
#include <oMath/hlslx.h>
#include <oMemory/memory.h>
#include <oSurface/convert.h>
#include "mesh_template.h"

namespace ouro { namespace mesh {

void copy_vertices(void* oRESTRICT* oRESTRICT dst, const layout_t& dst_elements, const void* oRESTRICT* oRESTRICT src, const layout_t& src_elements, uint32_t num_vertices)
{
	for (uint32_t di = 0; di < (uint32_t)dst_elements.size(); di++)
	{
		const element_t& e = dst_elements[di];
		if (e.semantic == mesh::element_semantic::unknown)
			continue;

		const uint32_t dslot = e.slot;

		if (!dst[dslot])
			continue;

		const uint32_t doff = element_offset(dst_elements, di);
		const uint32_t dstride = layout_size(dst_elements, dslot);

		bool copied = false;
		for (uint32_t si = 0; si < src_elements.size(); si++)
		{
			const element_t& se = src_elements[si];
			const uint32_t sslot = se.slot;
			if (e.semantic == se.semantic && e.index == se.index && src[sslot])
			{
				const uint32_t soff = element_offset(src_elements, si);
				const uint32_t sstride = layout_size(src_elements, sslot);

				void* oRESTRICT data = byte_add(dst[dslot], doff);

				surface::convert_structured(data, dstride, e.format, src[sslot], sstride, se.format, num_vertices);
				copied = true;
				break;
			}
		}

		if (!copied)
			memset2d4(byte_add(dst[dslot], doff), dstride, 0, surface::element_size(e.format), num_vertices);
	}
}

void transform_points(const float4x4& matrix, float3* oRESTRICT dst, uint32_t dst_stride, const float3* oRESTRICT src, uint32_t src_stride, uint32_t num_points)
{
	const float3* oRESTRICT end = byte_add(dst, dst_stride * num_points);
	while (dst < end)
	{
		*dst = mul(matrix, float4(*src, 1.0f)).xyz();
		dst = byte_add(dst, dst_stride);
		src = byte_add(src, src_stride);
	}
}

void transform_vectors(const float4x4& matrix, float3* oRESTRICT dst, uint32_t dst_stride, const float3* oRESTRICT src, uint32_t src_stride, uint32_t num_vectors)
{
	const float3x3 m_(matrix[0].xyz(), matrix[1].xyz(), matrix[2].xyz());
	const float3* oRESTRICT end = byte_add(dst, dst_stride * num_vectors);
	while (dst < end)
	{
		*dst = mul(m_, *src);
		dst = byte_add(dst, dst_stride);
		src = byte_add(src, src_stride);
	}
}

void calc_aabb(const float3* oRESTRICT vertices, uint32_t vertex_stride, uint32_t num_vertices, float3* oRESTRICT out_min, float3* oRESTRICT out_max)
{
	float3 mn = FLT_MAX;
	float3 mx = -FLT_MAX;

	const float3* oRESTRICT end = byte_add(vertices, vertex_stride * num_vertices);
	while (vertices < end)
	{
		mn = min(mn, *vertices);
		mx = max(mx, *vertices);
		vertices = byte_add(vertices, vertex_stride);
	}

	*out_min = mn;
	*out_max = mx;
}

float4 calc_sphere(const float3* vertices, uint32_t vertex_stride, uint32_t num_vertices)
{
	// https://en.wikipedia.org/wiki/Bounding_sphere#Ritter.27s_bounding_sphere
	// Replace this with Bouncing Bubble

	float3 mn, mx;
	calc_aabb(vertices, vertex_stride, num_vertices, &mn, &mx);

	float4 s;
	s.xyz() = (mx + mn) * 0.5f;
	s.w = distance(mn, mx) * 0.5f;
	return s;
}

uint8_t calc_log2scale(const float3& aabb_extents)
{
	float    max_extent   = max(aabb_extents);
	uint32_t max_extent_u = (uint32_t)floor(max_extent + 0.5f);
	uint32_t max_extent_2 = nextpow2(max_extent_u);
	return (uint8_t)log2i(max_extent_2);
}

void minmax_index(const uint16_t* oRESTRICT indices, uint32_t num_indices, uint32_t* oRESTRICT out_min_index, uint32_t* oRESTRICT out_max_index)
{
	return detail::minmax_index(indices, num_indices, out_min_index, out_max_index);
}

void minmax_index(const uint32_t* oRESTRICT indices, uint32_t num_indices, uint32_t* oRESTRICT out_min_index, uint32_t* oRESTRICT out_max_index)
{
	return detail::minmax_index(indices, num_indices, out_min_index, out_max_index);
}

void calc_face_normals(float3* oRESTRICT face_normals, const uint32_t* oRESTRICT indices, uint32_t num_indices, const float3* oRESTRICT positions, uint32_t num_positions, bool ccw)
{
	detail::calc_face_normals(face_normals, indices, num_indices, positions, num_positions, ccw);
}

void calc_face_normals(float3* oRESTRICT face_normals, const uint16_t* oRESTRICT indices, uint32_t num_indices, const float3* oRESTRICT positions, uint32_t num_positions, bool ccw)
{
	detail::calc_face_normals(face_normals, indices, num_indices, positions, num_positions, ccw);
}

void calc_vertex_normals(float3* vertex_normals, const uint32_t* indices, uint32_t num_indices, const float3* positions, uint32_t num_positions, bool ccw, bool overwrite_all)
{
	detail::calc_vertex_normals(vertex_normals, indices, num_indices, positions, num_positions, ccw, overwrite_all);
}

void calc_vertex_normals(float3* vertex_normals, const uint16_t* indices, uint32_t num_indices, const float3* positions, uint32_t num_positions, bool ccw, bool overwrite_all)
{
	detail::calc_vertex_normals(vertex_normals, indices, num_indices, positions, num_positions, ccw, overwrite_all);
}

void calc_vertex_tangents(float4* tangents, const uint32_t* indices, uint32_t num_indices, const float3* positions, const float3* normals, const float3* texcoords, uint32_t num_vertices)
{
	detail::calc_vertex_tangents(tangents, indices, num_indices, positions, normals, texcoords, num_vertices);
}

void calc_vertex_tangents(float4* tangents, const uint32_t* indices, uint32_t num_indices, const float3* positions, const float3* normals, const float2* texcoords, uint32_t num_vertices)
{
	detail::calc_vertex_tangents(tangents, indices, num_indices, positions, normals, texcoords, num_vertices);
}

void calc_vertex_tangents(float4* tangents, const uint16_t* indices, uint32_t num_indices, const float3* positions, const float3* normals, const float3* texcoords, uint32_t num_vertices)
{
	detail::calc_vertex_tangents(tangents, indices, num_indices, positions, normals, texcoords, num_vertices);
}

void calc_vertex_tangents(float4* tangents, const uint16_t* indices, uint32_t num_indices, const float3* positions, const float3* normals, const float2* texcoords, uint32_t num_vertices)
{
	detail::calc_vertex_tangents(tangents, indices, num_indices, positions, normals, texcoords, num_vertices);
}

void calc_texcoords(const float3& aabb_min, const float3& aabb_max, const uint32_t* indices, uint32_t num_indices, const float3* positions, float2* out_texcoords, uint32_t num_vertices, double* out_solve_time)
{
	detail::calc_texcoords(aabb_min, aabb_max, indices, num_indices, positions, out_texcoords, num_vertices, out_solve_time);
}

void calc_texcoords(const float3& aabb_min, const float3& aabb_max, const uint32_t* indices, uint32_t num_indices, const float3* positions, float3* out_texcoords, uint32_t num_vertices, double* out_solve_time)
{
	detail::calc_texcoords(aabb_min, aabb_max, indices, num_indices, positions, out_texcoords, num_vertices, out_solve_time);
}

void calc_texcoords(const float3& aabb_min, const float3& aabb_max, const uint16_t* indices, uint32_t num_indices, const float3* positions, float2* out_texcoords, uint32_t num_vertices, double* out_solve_time)
{
	detail::calc_texcoords(aabb_min, aabb_max, indices, num_indices, positions, out_texcoords, num_vertices, out_solve_time);
}

void calc_texcoords(const float3& aabb_min, const float3& aabb_max, const uint16_t* indices, uint32_t num_indices, const float3* positions, float3* out_texcoords, uint32_t num_vertices, double* out_solve_time)
{
	detail::calc_texcoords(aabb_min, aabb_max, indices, num_indices, positions, out_texcoords, num_vertices, out_solve_time);
}

namespace TerathonEdges {

	/** <citation
	usage="Implementation" 
	reason="tangents can be derived, and this is how to do it" 
	author="Eric Lengyel"
	description="http://www.terathon.com/code/edges.html"
	license="*** Assumed Public Domain ***"
	licenseurl="http://www.terathon.com/code/edges.html"
	modification="Minor changes to not limit algo to 65536 indices and some fixes to get it compiling"
	/>*/

	// $(CitedCodeBegin)

	// Building an Edge List for an Arbitrary Mesh
	// The following code builds a list of edges for an arbitrary triangle 
	// mesh and has O(n) running time in the number of triangles n in the 
	// pGeometry-> The edgeArray parameter must point to a previously allocated 
	// array of Edge structures large enough to hold all of the mesh's 
	// edges, which in the worst possible case is 3 times the number of 
	// triangles in the pGeometry->

	// An edge list is useful for many geometric algorithms in computer 
	// graphics. In particular, an edge list is necessary for stencil 
	// shadows.

	struct Edge
	{
		unsigned int      vertexIndex[2]; 
		unsigned int      faceIndex[2];
	};


	struct Triangle
	{
		unsigned int      index[3];
	};


	long BuildEdges(long vertexCount, long triangleCount,
		const Triangle *triangleArray, Edge *edgeArray)
	{
		long maxEdgeCount = triangleCount * 3;
		unsigned int *firstEdge = new unsigned int[vertexCount + maxEdgeCount];
		unsigned int *nextEdge = firstEdge + vertexCount;

		for (long a = 0; a < vertexCount; a++) firstEdge[a] = 0xFFFFFFFF;

		// First pass over all triangles. This finds all the edges satisfying the
		// condition that the first vertex index is less than the second vertex index
		// when the direction from the first vertex to the second vertex represents
		// a counterclockwise winding around the triangle to which the edge belongs.
		// For each edge found, the edge index is stored in a linked list of edges
		// belonging to the lower-numbered vertex index i. This allows us to quickly
		// find an edge in the second pass whose higher-numbered vertex index is i.

		long edgeCount = 0;
		const Triangle *triangle = triangleArray;
		for (long a = 0; a < triangleCount; a++)
		{
			long i1 = triangle->index[2];
			for (long b = 0; b < 3; b++)
			{
				long i2 = triangle->index[b];
				if (i1 < i2)
				{
					Edge *edge = &edgeArray[edgeCount];

					edge->vertexIndex[0] = (unsigned int) i1;
					edge->vertexIndex[1] = (unsigned int) i2;
					edge->faceIndex[0] = (unsigned int) a;
					edge->faceIndex[1] = (unsigned int) a;

					long edgeIndex = firstEdge[i1];
					if (edgeIndex == 0xFFFFFFFF)
					{
						firstEdge[i1] = edgeCount;
					}
					else
					{
						for (;;)
						{
							long index = nextEdge[edgeIndex];
							if (index == 0xFFFFFFFF)
							{
								nextEdge[edgeIndex] = edgeCount;
								break;
							}

							edgeIndex = index;
						}
					}

					nextEdge[edgeCount] = 0xFFFFFFFF;
					edgeCount++;
				}

				i1 = i2;
			}

			triangle++;
		}

		// Second pass over all triangles. This finds all the edges satisfying the
		// condition that the first vertex index is greater than the second vertex index
		// when the direction from the first vertex to the second vertex represents
		// a counterclockwise winding around the triangle to which the edge belongs.
		// For each of these edges, the same edge should have already been found in
		// the first pass for a different triangle. So we search the list of edges
		// for the higher-numbered vertex index for the matching edge and fill in the
		// second triangle index. The maximum number of comparisons in this search for
		// any vertex is the number of edges having that vertex as an endpoint.

		triangle = triangleArray;
		for (long a = 0; a < triangleCount; a++)
		{
			long i1 = triangle->index[2];
			for (long b = 0; b < 3; b++)
			{
				long i2 = triangle->index[b];
				if (i1 > i2)
				{
					for (long edgeIndex = firstEdge[i2]; edgeIndex != 0xFFFFFFFF;
						edgeIndex = nextEdge[edgeIndex])
					{
						Edge *edge = &edgeArray[edgeIndex];
						if ((edge->vertexIndex[1] == (unsigned int)i1) &&
							(edge->faceIndex[0] == edge->faceIndex[1]))
						{
							edge->faceIndex[1] = (unsigned int) a;
							break;
						}
					}
				}

				i1 = i2;
			}

			triangle++;
		}

		delete[] firstEdge;
		return (edgeCount);
	}

	// $(CitedCodeEnd)

} // namespace TerathonEdges

void calc_edges(uint32_t num_vertices, const uint32_t* indices, uint32_t num_indices, uint32_t** _ppEdges, uint32_t* out_num_edges)
{
	const uint32_t numTriangles = num_indices / 3;
	if ((uint32_t)((long)num_vertices) != num_vertices)
		throw std::out_of_range("num_vertices is out of range");
	
	if ((uint32_t)((long)numTriangles) != numTriangles)
		throw std::out_of_range("numTriangles is out of range");

	TerathonEdges::Edge* edgeArray = new TerathonEdges::Edge[3 * numTriangles];

	uint32_t numEdges = static_cast<uint32_t>(TerathonEdges::BuildEdges(static_cast<long>(num_vertices), static_cast<long>(numTriangles), (const TerathonEdges::Triangle *)indices, edgeArray));

	// @tony: Should the allocator be exposed?
	*_ppEdges = new uint32_t[numEdges * 2];

	for (size_t i = 0; i < numEdges; i++)
	{
		(*_ppEdges)[i*2] = edgeArray[i].vertexIndex[0];
		(*_ppEdges)[i*2+1] = edgeArray[i].vertexIndex[1];
	}

	*out_num_edges = numEdges;

	delete [] edgeArray;
}

void free_edge_list(uint32_t* edges)
{
	delete [] edges;
}

void remove_degenerates(const float3* oRESTRICT positions, uint32_t num_positions, uint32_t* oRESTRICT indices, uint32_t num_indices, uint32_t* oRESTRICT out_new_num_indices)
{
	detail::remove_degenerates(positions, num_positions, indices, num_indices, out_new_num_indices);
}

void remove_degenerates(const float3* oRESTRICT positions, uint32_t num_positions, uint16_t* oRESTRICT indices, uint32_t num_indices, uint32_t* oRESTRICT out_new_num_indices)
{
	detail::remove_degenerates(positions, num_positions, indices, num_indices, out_new_num_indices);
}

#define PUV(IndexT, UV0T, UV1T) \
void prune_unindexed_vertices(const IndexT* oRESTRICT indices, uint32_t num_indices \
	, float3* oRESTRICT positions, float3* oRESTRICT normals, float4* oRESTRICT tangents, UV0T* oRESTRICT texcoords0, UV1T* oRESTRICT texcoords1, uint32_t* oRESTRICT colors \
	, uint32_t num_vertices, uint32_t* oRESTRICT out_new_num_vertices) \
{ detail::prune_unindexed_vertices(indices, num_indices, positions, normals, tangents, texcoords0, texcoords1, colors, num_vertices, out_new_num_vertices); }

PUV(uint32_t, float2, float2)
PUV(uint32_t, float2, float3)
PUV(uint32_t, float3, float2)
PUV(uint32_t, float3, float3)
PUV(uint16_t, float2, float2)
PUV(uint16_t, float2, float3)
PUV(uint16_t, float3, float2)
PUV(uint16_t, float3, float3)

}}
