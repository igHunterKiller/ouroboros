// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Runtime representation of vertex and index information

#pragma once
#include <oMemory/allocate.h>
#include <oMesh/mesh.h>
#include <array>

namespace ouro { namespace mesh {

class model
{
public:
	model();
	model(const info_t& info
		, const allocator& subsets_alloc = default_allocator
		, const allocator& mesh_alloc = default_allocator)
	{ initialize(info, subsets_alloc, mesh_alloc); }

	model(const info_t& info
		, const void* subset_data
		, const void* mesh_data
		, const allocator& subsets_alloc = noop_allocator
		, const allocator& mesh_alloc = noop_allocator)
	{ initialize(info, subset_data, mesh_data, subsets_alloc, mesh_alloc); }
	
	~model() { deinitialize(); }

	model(model&& that);
	model& operator=(model&& that);

	// create a buffer with uninitialized subset and mesh data
	void initialize(const info_t& info
		, const allocator& subsets_alloc = default_allocator
		, const allocator& mesh_alloc = default_allocator);

	// create a buffer using the specified pointer; the allocs will be used to manage the lifetime
	void initialize(const info_t& info
		, const void* subset_data
		, const void* mesh_data
		, const allocator& subsets_alloc = noop_allocator
		, const allocator& mesh_alloc = noop_allocator);

	void deinitialize();

	const info_t& info() const { return info_; }

	// direct access to subsets
	const subset_t* subsets() const { return subsets_; }
	subset_t* subsets() { return subsets_; }

	// direct access to indices
	const uint16_t* indices() const { return (const uint16_t*)((const uint8_t*)mesh_+indices_offset_); }
	uint16_t* indices() { return (uint16_t*)((uint8_t*)mesh_+indices_offset_); }
	
	// direct access to vertices by slot. Use mesh::layout_size and num_vertices to traverse the memory
	const void* vertices(uint32_t slot) const { return ((const void*)(mesh_+vertices_offsets_[slot])); }
	void* vertices(uint32_t slot) { return ((void*)(mesh_+vertices_offsets_[slot])); }

	uint32_t indices_offset() const { return indices_offset_; }
	uint32_t vertices_offset(uint32_t slot) const { return vertices_offsets_[slot]; }
	uint32_t vertex_stride(uint32_t slot) const { return vertex_strides_[slot]; }

	// frees the existing mesh data and assigns new elements
	inline void rebase(const void* new_base, deallocate_fn mesh_dealloc, uint32_t indices_offset, const std::array<uint32_t, max_num_slots>& vertices_offsets)
	{
		if (mesh_dealloc_ && mesh_)
			mesh_dealloc_(mesh_);
		mesh_ = (uint8_t*)new_base;
		mesh_dealloc_ = mesh_dealloc;

		indices_offset_ = indices_offset;
		vertices_offsets_ = vertices_offsets;
	}

	// modifies all vertices by tx
	void bake_transform(const float4x4& tx);

private:
	info_t info_;
	subset_t* subsets_;
	uint8_t* mesh_;
	deallocate_fn subset_dealloc_;
	deallocate_fn mesh_dealloc_;
	uint32_t indices_offset_;
	uint32_t padA;
	std::array<uint32_t, max_num_slots> vertices_offsets_;
	std::array<uint8_t, max_num_slots> vertex_strides_;

	void init_offsets_and_strides(uint32_t indices_bytes);
};

}}
