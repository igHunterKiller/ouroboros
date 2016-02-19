// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.
#include <oMesh/model.h>
#include <oMesh/mesh.h>
#include <oMemory/fnv1a.h>

namespace ouro { namespace mesh {

model::model()
	: subsets_(nullptr)
	, mesh_(nullptr)
	, subset_dealloc_(nullptr)
	, mesh_dealloc_(nullptr)
	, indices_offset_(0)
	, padA(0)
{
	vertices_offsets_.fill(0);
	vertex_strides_.fill(0);
}

model::model(model&& that)
	: info_(std::move(that.info_))
	, subsets_(that.subsets_)
	, mesh_(that.mesh_)
	, subset_dealloc_(that.subset_dealloc_)
	, mesh_dealloc_(that.mesh_dealloc_)
	, vertices_offsets_(std::move(that.vertices_offsets_))
	, vertex_strides_(std::move(that.vertex_strides_))
	, indices_offset_(that.indices_offset_)
	, padA(that.padA)
{
	that.subsets_ = nullptr;
	that.mesh_ = nullptr;
	that.subset_dealloc_ = nullptr;
	that.mesh_dealloc_ = nullptr;
	that.indices_offset_ = 0;
	that.padA = 0;
	that.vertices_offsets_.fill(0);
	that.vertex_strides_.fill(0);
}

model& model::operator=(model&& that)
{
	if (this != &that)
	{
		deinitialize();

		info_ = std::move(that.info_);
		subsets_ = that.subsets_; that.subsets_ = nullptr;
		mesh_ = that.mesh_; that.mesh_ = nullptr;
		subset_dealloc_ = that.subset_dealloc_; that.subset_dealloc_ = nullptr;
		mesh_dealloc_ = that.mesh_dealloc_; that.mesh_dealloc_ = nullptr;
		vertices_offsets_ = std::move(that.vertices_offsets_); that.vertices_offsets_.fill(0);
		vertex_strides_ = std::move(that.vertex_strides_); that.vertex_strides_.fill(0);
		indices_offset_ = that.indices_offset_; that.indices_offset_ = 0;
		padA = that.padA; that.padA = 0;
	}

	return *this;
}

void model::init_offsets_and_strides(uint32_t indices_bytes)
{
	indices_offset_ = 0;
	uint32_t nslots = info_.num_slots;

	for (uint32_t slot = 0, offset = indices_bytes; slot < max_num_slots; slot++)
	{
		if (slot < nslots)
		{
			const uint32_t stride = layout_size(info_.layout, slot);
			const uint32_t vertices_size = stride * info_.num_vertices;
			vertices_offsets_[slot] = offset;
			vertex_strides_[slot] = (uint8_t)stride;
			offset += vertices_size;
		}

		else
		{
			vertices_offsets_[slot] = 0;
			vertex_strides_[slot] = 0;
		}
	}
}

void model::initialize(const info_t& info
	, const allocator& subsets_alloc
	, const allocator& mesh_alloc)
{
	const uint32_t subsets_bytes = sizeof(subset_t) * info.num_subsets;
	const uint32_t vertex_bytes = layout_size(info.layout);
	const uint32_t vertices_bytes = vertex_bytes * info.num_vertices;
	const uint32_t indices_bytes = align((uint32_t)sizeof(uint16_t) * info.num_indices, sizeof(uint32_t));
	const uint32_t mesh_bytes = indices_bytes + vertices_bytes;

	info_ = info;

	subsets_ = (subset_t*)subsets_alloc.allocate(subsets_bytes, "model subsets");
	if (!subsets_)
		throw std::bad_alloc();

	mesh_ = (uint8_t*)mesh_alloc.allocate(mesh_bytes, "model mesh data");
	if (!mesh_)
		throw std::bad_alloc();

	subset_dealloc_ = subsets_alloc.deallocator();
	mesh_dealloc_ = mesh_alloc.deallocator();

	padA = 0;

	init_offsets_and_strides(indices_bytes);

#ifdef oDEBUG
	memset(subsets(), 0, subsets_bytes);

	// debug-init index data for debugging
	memset(indices(), 0xff, indices_bytes);

	// debug-init vertex data for debugging
	for (uint32_t slot = 0; slot < info.num_slots; slot++)
	{
		const uint32_t stride = vertex_stride(slot);
		void* vertex_data = vertices(slot);
		memset(vertex_data, slot, stride * info.num_vertices);
	}
#else
	// init index padding
	if ((sizeof(uint16_t) * info.num_indices) != indices_bytes)
		*(indices() + info.num_indices) = 0xff;
#endif
}

void model::initialize(const info_t& info
	, const void* subset_data
	, const void* mesh_data
	, const allocator& subsets_alloc
	, const allocator& mesh_alloc)
{
	info_ = info;

	subsets_ = (subset_t*)subset_data;
	mesh_ = (uint8_t*)mesh_data;
	subset_dealloc_ = subsets_alloc.deallocator();
	mesh_dealloc_ = mesh_alloc.deallocator();

	const uint32_t indices_bytes = align((uint32_t)sizeof(uint16_t) * info.num_indices, sizeof(uint32_t));
	init_offsets_and_strides(indices_bytes);
}

void model::deinitialize()
{
	bool reinit = false;

	if (subsets_ && subset_dealloc_)
	{
		subset_dealloc_(subsets_);
		subsets_ = nullptr;
		reinit = true;
	}

	if (mesh_ && mesh_dealloc_)
	{
		mesh_dealloc_(mesh_);
		mesh_ = nullptr;
		reinit = true;
	}

	if (reinit)
		*this = model();
}

void model::bake_transform(const float4x4& tx)
{
	const auto& layout = info_.layout;
	uint32_t i = 0;
	while (layout[i].semantic != mesh::element_semantic::unknown)
	{
		if (layout[i].semantic == mesh::element_semantic::position)
		{
			auto offset = element_offset(layout, i);
			auto slot = layout[i].slot;
			auto stride = vertex_stride(slot);
			float3* positions = (float3*)((uint8_t*)vertices(slot) + offset);
			float3* end = (float3*)((uint8_t*)positions + stride * info_.num_vertices);

			while (positions < end)
			{
				*positions = mul(tx, float4(*positions, 1.0f)).xyz();
				positions = (float3*)((uint8_t*)positions + stride);
			}

			return;
		}

		i++;
	}

	oThrow(std::errc::invalid_argument, "no position semantic found");
}

}}
