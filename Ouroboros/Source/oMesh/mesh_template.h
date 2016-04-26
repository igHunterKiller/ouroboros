// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.
#pragma once
#ifndef oMesh_mesh_template
#define oMesh_mesh_template

#include <oMesh/mesh.h>
#include <oCore/algorithm.h>
#include <oCore/byte.h>
#include <oConcurrency/concurrency.h>
#include <oMath/hlsl.h>
#include <vector>

namespace ouro { namespace mesh { namespace detail {

template<typename T> void minmax_index(const T* oRESTRICT indices, uint32_t num_indices, uint32_t* oRESTRICT out_min_index, uint32_t* oRESTRICT out_max_index)
{
	uint32_t min_index = UINT_MAX;
	uint32_t max_index = 0;
	for (uint32_t i = 0; i < num_indices; i++)
	{
		min_index = std::min(min_index, (uint32_t)indices[i]);
		max_index = std::max(max_index, (uint32_t)indices[i]);
	}
	
	*out_min_index = min_index;
	*out_max_index = max_index;
}

template<typename T, typename IndexT> void remove_degenerates(const oHLSL3<T>* oRESTRICT positions, uint32_t num_positions, IndexT* oRESTRICT indices, uint32_t num_indices, uint32_t* oRESTRICT out_new_num_indices)
{
	if ((num_indices % 3) != 0)
		oThrow(std::errc::invalid_argument, "num_indices must be a multiple of 3");

	for (uint32_t i = 0; i < num_indices / 3; i++)
	{
		uint32_t I = i * 3;
		uint32_t J = i * 3 + 1;
		uint32_t K = i * 3 + 2;

		if (indices[I] >= num_positions || indices[J] >= num_positions || indices[K] >= num_positions)
			oThrow(std::errc::invalid_argument, "an index value indexes outside the range of vertices specified");

		const oHLSL3<T>& a = positions[indices[I]];
		const oHLSL3<T>& b = positions[indices[J]];
		const oHLSL3<T>& c = positions[indices[K]];

		if (equal(cross(a - b, a - c), oHLSL3<T>(T(0), T(0), T(0))))
		{
			indices[I] = IndexT(-1);
			indices[J] = IndexT(-1);
			indices[K] = IndexT(-1);
		}
	}

	*out_new_num_indices = num_indices;
	for (uint32_t i = 0; i < *out_new_num_indices; i++)
	{
		if (indices[i] == ~0u)
		{
			memcpy(&indices[i], &indices[i+1], sizeof(uint32_t) * (num_indices - i - 1));
			i--;
			(*out_new_num_indices)--;
		}
	}
}

template<typename T, typename IndexT> void calc_face_normals_task(size_t index, oHLSL3<T>* oRESTRICT face_normals, const IndexT* oRESTRICT indices, uint32_t num_indices, const oHLSL3<T>* oRESTRICT positions, uint32_t num_positions, T ccwMultiplier, bool* oRESTRICT _pSuccess)
{
	size_t I = index * 3;
	size_t J = index * 3 + 1;
	size_t K = index * 3 + 2;

	if (indices[I] >= num_positions || indices[J] >= num_positions || indices[K] >= num_positions)
	{
		*_pSuccess = false;
		return;
	}

	const oHLSL3<T>& a = positions[indices[I]];
	const oHLSL3<T>& b = positions[indices[J]];
	const oHLSL3<T>& c = positions[indices[K]];

	// gracefully put in a zero vector for degenerate faces
	oHLSL3<T> cr = cross(a - b, a - c);
	const oHLSL3<T> Zero3(T(0), T(0), T(0));
	face_normals[index] = equal(cr, Zero3) ? Zero3 : normalize(cr) * ccwMultiplier;
}

template<typename T, typename IndexT> void calc_face_normals(oHLSL3<T>* oRESTRICT face_normals, const IndexT* oRESTRICT indices, uint32_t num_indices, const oHLSL3<T>* oRESTRICT positions, uint32_t num_positions, bool ccw)
{
	if ((num_indices % 3) != 0)
		oThrow(std::errc::invalid_argument, "num_indices must be a multiple of 3");

	bool success = true;
	const T s = ccw ? T(-1) : T(1);
	parallel_for( 0, num_indices / 3, std::bind( calc_face_normals_task<T, IndexT>, std::placeholders::_1
		, face_normals, indices, num_indices, positions, num_positions, s, &success));
	if (!success)
		oThrow(std::errc::invalid_argument, "an index value indexes outside the range of vertices specified");
}

template<typename InnerContainerT, typename VecT> void average_face_normals(
	const std::vector<InnerContainerT>& container, const std::vector<oHLSL3<VecT>>& face_normals
	, oHLSL3<VecT>* normals, uint32_t num_vertex_normals, bool overwrite_all)
{
	// Now go through the list and average the normals
	for (uint32_t i = 0; i < num_vertex_normals; i++)
	{
		// If there is length on the data already, leave it alone
		if (!overwrite_all && equal(normals[i], oHLSL3<VecT>(VecT(0), VecT(0), VecT(0))))
			continue;

		oHLSL3<VecT> N(VecT(0), VecT(0), VecT(0));
		const InnerContainerT& TrianglesUsed = container[i];
		for (size_t t = 0; t < TrianglesUsed.size(); t++)
		{
			uint32_t faceIndex = TrianglesUsed[t];
			if (!equal(dot(face_normals[faceIndex], face_normals[faceIndex]), VecT(0)))
				N += face_normals[faceIndex];
		}

		normals[i] = normalize(N);
	}
}

template<typename T, typename IndexT> void calc_vertex_normals(oHLSL3<T>* oRESTRICT vertex_normals, const IndexT* oRESTRICT indices, uint32_t num_indices
	, const oHLSL3<T>* oRESTRICT positions, uint32_t num_positions, bool ccw = false, bool overwrite_all = true)
{
	std::vector<oHLSL3<T>> faceNormals(num_indices / 3, oHLSL3<T>(T(0), T(0), T(0)));

	calc_face_normals(faceNormals.data(), indices, num_indices, positions, num_positions, ccw);

	const uint32_t nFaces = (uint32_t)num_indices / 3;
	const uint32_t REASONABLE_MAX_FACES_PER_VERTEX = 32;
	std::vector<fixed_vector<uint32_t, REASONABLE_MAX_FACES_PER_VERTEX>> trianglesUsedByVertexA(num_positions);
	std::vector<std::vector<uint32_t>> trianglesUsedByVertex;
	bool UseVecVec = false;

	// Try with a less-memory-intensive method first. If that overflows, fall back
	// to the alloc-y one. This is to avoid a 5x slowdown when std::vector gets
	// released due to secure CRT over-zealousness in this case.
	// for each vertex, store a list of the faces to which it contributes
	for (uint32_t i = 0; i < nFaces; i++)
	{
		fixed_vector<uint32_t, REASONABLE_MAX_FACES_PER_VERTEX>& a = trianglesUsedByVertexA[indices[i*3]];
		fixed_vector<uint32_t, REASONABLE_MAX_FACES_PER_VERTEX>& b = trianglesUsedByVertexA[indices[i*3+1]];
		fixed_vector<uint32_t, REASONABLE_MAX_FACES_PER_VERTEX>& c = trianglesUsedByVertexA[indices[i*3+2]];

		// maybe fixed_vector should throw? and catch it here instead of this?
		if (a.size() == a.capacity() || b.size() == b.capacity() || c.size() == c.capacity())
		{
			UseVecVec = true;
			break;
		}

		push_back_unique(a, i);
		push_back_unique(b, i);
		push_back_unique(c, i);
	}

	if (UseVecVec)
	{
		// free memory
		trianglesUsedByVertexA.clear();
		trianglesUsedByVertexA.shrink_to_fit();
		trianglesUsedByVertex.resize(num_positions);

		for (uint32_t i = 0; i < nFaces; i++)
		{
			push_back_unique(trianglesUsedByVertex[indices[i*3]], i);
			push_back_unique(trianglesUsedByVertex[indices[i*3+1]], i);
			push_back_unique(trianglesUsedByVertex[indices[i*3+2]], i);
		}

		average_face_normals(trianglesUsedByVertex, faceNormals, vertex_normals, num_positions, overwrite_all);

		uint32_t MaxValence = 0;
		// print out why we ended up in this path...
		for (uint32_t i = 0; i < num_positions; i++)
			MaxValence = max(MaxValence, (uint32_t)trianglesUsedByVertex[i].size());
		oTrace("debug-slow path in normals caused by reasonable max valence (%u) being exceeded. Actual valence: %u", REASONABLE_MAX_FACES_PER_VERTEX, MaxValence);
	}

	else
		average_face_normals(trianglesUsedByVertexA, faceNormals, vertex_normals, num_positions, overwrite_all);
}

template<typename T, typename IndexT, typename TexCoordTupleT> void calc_vertex_tangents(oHLSL4<T>* oRESTRICT tangents, const IndexT* oRESTRICT indices, uint32_t num_indices
 , const oHLSL3<T>* oRESTRICT positions, const oHLSL3<T>* oRESTRICT normals, const TexCoordTupleT* oRESTRICT texcoords, uint32_t num_vertices)
{
	/** <citation
		usage="Implementation" 
		reason="tangents can be derived, and this is how to do it" 
		author="Eric Lengyel"
		description="http://www.terathon.com/code/tangent.html"
		license="*** Assumed Public Domain ***"
		licenseurl="http://www.terathon.com/code/tangent.html"
		modification="Changes types to oMath types"
	/>*/

	// $(CitedCodeBegin)

	std::vector<oHLSL3<T>> tan1(num_vertices, oHLSL3<T>(T(0), T(0), T(0)));
	std::vector<oHLSL3<T>> tan2(num_vertices, oHLSL3<T>(T(0), T(0), T(0)));

	for (uint32_t i = 0; i < num_indices; i++)
	{
		const uint32_t a = indices[i++];
		const uint32_t b = indices[i++];
		const uint32_t c = indices[i];

		const auto& Pa = positions[a];
		const auto& Pb = positions[b];
		const auto& Pc = positions[c];

		const auto x1 = Pb - Pa;
		const auto x2 = Pc - Pa;

		const auto& TCa = texcoords[a];
		const auto& TCb = texcoords[b];
		const auto& TCc = texcoords[c];

		const auto st1 = TCb.xy() - TCa.xy();
		const auto st2 = TCc.xy() - TCa.xy();

		const auto denom = st1.x * st2.y - st2.x * st1.y;
		if (!equal(denom, 0.0f))
		{
			auto r = T(1) / denom;

			oHLSL3<T> s((st2.y * x1.x - st1.y * x2.x) * r, (st2.y * x1.y - st1.y * x2.y) * r, (st2.y * x1.z - st1.y * x2.z) * r);
			oHLSL3<T> t((st1.x * x2.x - st2.x * x1.x) * r, (st1.x * x2.y - st2.x * x1.y) * r, (st1.x * x2.z - st2.x * x1.z) * r);

			tan1[a] += s;
			tan1[b] += s;
			tan1[c] += s;

			tan2[a] += t;
			tan2[b] += t;
			tan2[c] += t;
		}
	}

	parallel_for(0, num_vertices, [&](size_t idx)
	{
		// Gram-Schmidt orthogonalize + handedness
		const oHLSL3<T>& n = normals[idx];
		const oHLSL3<T>& t = tan1[idx];
		tangents[idx] = oHLSL4<T>(normalize(t - n * dot(n, t)), (dot(cross(n, t), tan2[idx]) < T(0)) ? T(-1) : T(1));
	});

	// $(CitedCodeEnd)
}

template<typename T, typename IndexT, typename UV0T>
void calc_texcoords(const oHLSL3<T>& aabb_min, const oHLSL3<T>& aabb_max, const IndexT* indices, uint32_t num_indices, const oHLSL3<T>* positions, UV0T* out_texcoords, uint32_t num_vertices, double* out_solve_time)
{
	oThrow(std::errc::operation_not_supported, "calc_texcoords not implemented");
}

template<typename IndexT> void prune_indices(const std::vector<bool>& refed, IndexT* indices, uint32_t num_indices)
{
	std::vector<IndexT> sub(refed.size(), 0);
	for (size_t i = 0; i < refed.size(); i++)
		if (!refed[i])
			for (size_t j = i; j < sub.size(); j++)
				(sub[j])++;

	for (uint32_t i = 0; i < num_indices; i++)
		indices[i] -= sub[indices[i]];
}

template<typename T> uint32_t prune_stream(const std::vector<bool>& refed, T* _pStream, uint32_t _NumberOfElements)
{
	if (!_pStream)
		return 0;

	std::vector<bool>::const_iterator itRefed = refed.begin();
	T* r = _pStream, *w = _pStream;
	while (itRefed != refed.end())
	{
		if (*itRefed++)
			*w++ = *r++;
		else
			++r;
	}

	return static_cast<uint32_t>(_NumberOfElements - (r - w));
}

template<typename T, typename IndexT, typename UV0VecT, typename UV1VecT> 
void prune_unindexed_vertices(const IndexT* oRESTRICT indices, uint32_t num_indices
	, oHLSL3<T>* oRESTRICT positions, oHLSL3<T>* oRESTRICT normals, oHLSL4<T>* oRESTRICT tangents, UV0VecT* oRESTRICT texcoords0, UV1VecT* oRESTRICT texcoords1, uint32_t* oRESTRICT colors
	, uint32_t num_vertices, uint32_t* oRESTRICT out_new_num_vertices)
{
	std::vector<bool> refed;
	refed.assign(num_vertices, false);
	for (size_t i = 0; i < num_indices; i++)
		refed[indices[i]] = true;
	uint32_t newNumVertices = 0;

	static const uint32_t kNumVertexWhereParallelismHelps = 2000;

	if (num_vertices < kNumVertexWhereParallelismHelps)
	{
		newNumVertices = prune_stream(refed, positions, num_vertices);
		prune_stream(refed, normals, num_vertices);
		prune_stream(refed, tangents, num_vertices);
		prune_stream(refed, texcoords0, num_vertices);
		prune_stream(refed, texcoords1, num_vertices);
		prune_stream(refed, colors, num_vertices);
	}

	else
	{
		task_group* g = new_task_group();
		g->run([&] { newNumVertices = prune_stream(refed, positions, num_vertices); });
		g->run([&] { prune_stream(refed, normals, num_vertices); });
		g->run([&] { prune_stream(refed, tangents, num_vertices); });
		g->run([&] { prune_stream(refed, texcoords0, num_vertices); });
		g->run([&] { prune_stream(refed, texcoords1, num_vertices); });
		g->run([&] { prune_stream(refed, colors, num_vertices); });
		g->wait();
		delete_task_group(g);
	}

	*out_new_num_vertices = newNumVertices;
}

}}}

#endif
