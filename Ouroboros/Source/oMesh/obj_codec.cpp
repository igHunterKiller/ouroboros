// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oMesh/codec.h>
#include <oMesh/obj.h>
#include <oString/string.h>

namespace ouro { namespace mesh {

bool is_obj(const void* buffer, size_t size)
{
	return strnistr((const char*)buffer, "obj file", size) || strnistr((const char*)buffer, "mtllib", size);
}

blob encode_obj(const model& mdl
	, const allocator& file_alloc
	, const allocator& temp_alloc)
{
	throw std::exception("encode obj not implemented");
}

model decode_obj(const path_t& path
	, const void* buffer, size_t size
	, const layout_t& desired_layout
	, const allocator& subsets_alloc
	, const allocator& mesh_alloc
	, const allocator& temp_alloc)
{
	if (!is_obj(buffer, size))
		return model();

	obj::init_t init;
	init.counter_clockwise_faces = false;
	auto mesh = obj::mesh::make(init, path, (const char*)buffer); // buffer must be nul-terminated
	auto obj_info = mesh->info();
	auto model_info = obj_info.mesh_info;
	model_info.layout = desired_layout;
	model_info.num_slots = (uint8_t)layout_slots(desired_layout);

	model mdl(model_info, subsets_alloc, mesh_alloc);
	memcpy(mdl.subsets(), obj_info.subsets, model_info.num_subsets * sizeof(subset_t));
	copy_indices(mdl.indices(), obj_info.indices, model_info.num_indices);

	const uint32_t dst_nslots = model_info.num_slots;
	void** dst_slots = (void**)alloca(dst_nslots * sizeof(void*));
	for (uint32_t slot = 0; slot < dst_nslots; slot++)
		dst_slots[slot] = mdl.vertices(slot);

	const void* src_slots[3] = { obj_info.positions, obj_info.normals, obj_info.texcoords };
	
	copy_vertices(dst_slots, model_info.layout, src_slots, obj_info.mesh_info.layout, model_info.num_vertices);

	return mdl;
}

}}
