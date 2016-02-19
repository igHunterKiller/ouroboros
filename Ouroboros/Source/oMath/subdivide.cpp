// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oMath/subdivide.h>
#include <oBase/growable_hash_map.h> // @tony uh oh, dependency wonkiness: oMath should not rely on oBase

namespace ouro {

template<typename T, typename U> inline U powi(T a, U e)  { U v = 1; for (U i = 0; i < e; i++) v *= U(a); return v; }

uint16_t calc_max_indices(uint16_t base_num_indices, uint16_t divide)
{
	return base_num_indices * powi(4, divide);
}

uint16_t calc_max_vertices(uint16_t base_num_vertices, uint16_t divide)
{
	uint16_t n = 1 + powi(2, divide);
	return base_num_vertices * (n * (n + 1) / 2);
}

uint16_t midpoint(growable_hash_map<uint64_t, uint16_t>& cache, uint16_t a, uint16_t b, float* vertices, uint16_t& inout_num_vertices, void (*new_vertex)(uint16_t new_index, uint16_t a, uint16_t b, void* user), void* user)
{
	// checks cache for a pre-existing vertex. If none found, appends a new vertex
	// half-way between the positions at a and b.

	// create a unique id: the edge between two vertices
	const uint64_t key = 1 + ((a < b) ? ((uint64_t(a) << 32) | uint64_t(b)) : ((uint64_t(b) << 32) | uint64_t(a)));

	// check cache
	uint16_t idx = uint16_t(-1);
	if (cache.get(key, &idx))
		return idx;

	// add a new index to the cache
	idx = inout_num_vertices;
	if (!cache.add(key, idx))
		oThrow(std::errc::invalid_argument, "failed to add new index to cache");

	// add a new vertex midway between the vertices
	float* vo = vertices + 3 * inout_num_vertices;
	float* va = vertices + 3 * a;
	float* vb = vertices + 3 * b;

	vo[0] = (va[0] + vb[0]) * 0.5f;
	vo[1] = (va[1] + vb[1]) * 0.5f;
	vo[2] = (va[2] + vb[2]) * 0.5f;
	inout_num_vertices++;
	
	// and any extra attributes
	if (new_vertex)
		new_vertex(idx, a, b, user);

	return idx;
}

void subdivide(uint16_t divide, uint16_t* indices, uint32_t& inout_num_indices, float* vertices, uint16_t& inout_num_vertices, void (*new_vertex)(uint16_t new_index, uint16_t a, uint16_t b, void* user), void* user)
{
	if (divide > 6)
		oThrow(std::errc::invalid_argument, "large subdivide would take too long to calculate");

	uint16_t pow2_divide = 1 << (divide    );
	uint16_t pow4_divide = 1 << (divide * 2);

	const uint32_t max_verts32 = (uint32_t)inout_num_vertices * pow2_divide;
	if (max_verts32 > 65535)
		oThrow(std::errc::invalid_argument, "subdivision would require 32-bit indices");

	const uint16_t max_verts = (uint16_t)max_verts32;
	const uint32_t max_indices = inout_num_indices * pow4_divide;

	if (!indices && !vertices)
	{
		inout_num_indices  = max_indices;
		inout_num_vertices = max_verts;
		return;
	}

	growable_hash_map<uint64_t, uint16_t> cache(max_verts, "subdivide vertex cache", default_allocator);

	for (uint32_t div = 0; div < divide; div++)
	{
		const uint32_t nindices = inout_num_indices;
	
		for (uint32_t i = 0; i < nindices; i += 3)
		{
			const uint16_t a = indices[i+0];
			const uint16_t b = indices[i+1];
			const uint16_t c = indices[i+2];

			const uint16_t ab_mid = midpoint(cache, b, a, vertices, inout_num_vertices, new_vertex, user);
			const uint16_t bc_mid = midpoint(cache, c, b, vertices, inout_num_vertices, new_vertex, user);
			const uint16_t ca_mid = midpoint(cache, a, c, vertices, inout_num_vertices, new_vertex, user);

			// overwrite old triangle
			indices[i+0] = a;
			indices[i+1] = ab_mid;
			indices[i+2] = ca_mid;

			// and add 3 more
			indices[inout_num_indices++] = ca_mid; indices[inout_num_indices++] = ab_mid; indices[inout_num_indices++] = bc_mid;
			indices[inout_num_indices++] = ca_mid; indices[inout_num_indices++] = bc_mid; indices[inout_num_indices++] = c;
			indices[inout_num_indices++] = ab_mid; indices[inout_num_indices++] = b;      indices[inout_num_indices++] = bc_mid;
		}
	}
}

}
