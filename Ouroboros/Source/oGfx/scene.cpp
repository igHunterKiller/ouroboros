// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oGfx/scene.h>
#include <oMath/seg_v_obb.h>

namespace ouro { namespace gfx {

void scene_t::initialize(const scene_init_t& init)
{
	init_       = init;
	num_pivots_ = 0;

	const uint32_t ptr_bytes   = init.max_pivots * sizeof(pivot_t*);
	const uint32_t pool_bytes  = pivot_pool_.calc_size(init.max_pivots);
	const uint32_t scene_bytes = ptr_bytes + pool_bytes;
	void* scene_arena          = default_allocate(scene_bytes, "scene");
	void* pool_arena           = scene_arena;
	pivots_                    = (gfx::pivot_t**)((uint8_t*)pool_arena + pool_bytes);

	pivot_pool_.initialize(pool_arena, pool_bytes);
}

void scene_t::deinitialize()
{
	void* scene_arena = pivot_pool_.deinitialize();

	default_deallocate(scene_arena);
}

pivot_t* scene_t::new_pivot(const pivot_t& pivot)
{
	auto p = pivot_pool_.create(pivot);

	if (p)
	{
		pivots_[num_pivots_++] = p;
	}

	return p;
}

void scene_t::del_pivot(pivot_t* p)
{
	for (uint32_t i = 0; i < num_pivots_; i++)
	{
		if (pivots_[i] == p)
		{
			std::swap(pivots_[i], pivots_[--num_pivots_]);
			break;
		}
	}

	pivot_pool_.destroy(p);
}

size_t scene_t::select(pivot_t** out_pivots, size_t max_num_pivots)
{
	const size_t n = min(max_num_pivots, (size_t)num_pivots_);
	for (size_t i = 0; i < n; i++)
	{
		out_pivots[i] = pivots_[i];
	}

	return n;
}

size_t scene_t::select_by_segment(const float3& WSseg0, const float3& WSseg1, pivot_t** out_pivots, size_t max_num_pivots)
{
	size_t n = 0;
	for (size_t i = 0; i < num_pivots_; i++)
	{
		const auto& pivot = pivots_[i];

		float3x3 r;
		float3 p, e;
		float t0, t1;
		pivot->obb(&r, &p, &e);
		if (seg_vs_obb(WSseg0, WSseg1, r, p, e, &t0, &t1))
		{
			out_pivots[n++] = pivots_[i];
			if (n >= max_num_pivots)
				break;
		}
	}

	return n;
}

float4 scene_t::calc_bounding_sphere() const
{
	//for (size_t i = 0; i < num_pivots_; i++)
	//{
	//	const auto& pivot = pivots_[i];
	//}

	return float4(0.0f, 0.0f, 0.0f, 34.0f);
}

}}
