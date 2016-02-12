// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#pragma once

#include <oMemory/object_pool.h>

#include <oGfx/pivot.h>

namespace ouro { namespace gfx {

struct scene_init_t
{
	uint32_t max_pivots;
};

class scene_t
{
public:
	scene_t() {}
	~scene_t() { deinitialize(); }

	void initialize(const scene_init_t& init);
	void deinitialize();

// _____________________________________________________________________________
// ...

	pivot_t* new_pivot(const void* src);

	pivot_t* new_pivot(const pivot_t& pivot);

	// add criteria, cull volume, flags
	size_t select(pivot_t** out_pivots, size_t max_num_pivots);
	template<size_t size> size_t select(pivot_t* (&out_pivots)[size]) { return select(out_pivots, size); }

	size_t select_by_segment(const float3& WSseg0, const float3& WSseg1, pivot_t** out_pivots, size_t max_num_pivots);

	float4 calc_bounding_sphere() const;
	
private:
	scene_t(const scene_t&);
	const scene_t& operator=(const scene_t&);

	void del_pivot(pivot_t* p);

private:
	scene_init_t init_;

	// consider using small block allocator for the varying object sizes here.
	object_pool<pivot_t> pivot_pool_;

	pivot_t** pivots_;
	uint32_t num_pivots_;
};

}}
