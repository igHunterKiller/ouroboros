// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oMemory/fnv1a.h>
#include <oMesh/codec.h>
#include <oMesh/face.h>
#include <oMesh/obj.h>
#include <oCore/bit.h>
#include <oString/fixed_string.h>
#include <oString/string.h>

#include <oBase/scoped_timer.h>
#include <oSystem/filesystem.h>

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
	init.mesh_alloc = temp_alloc;
	init.temp_alloc = temp_alloc;

	obj obj(init, path.c_str(), buffer, size);

	const auto    nidx        = obj.num_indices();
	const auto    nvtx        = obj.num_vertices();
	const auto    ngrp        = obj.num_groups();
	const auto    indices     = obj.indices();
	const float3* positions   = obj.positions();
	      float3* texcoords   = obj.texcoords();
	      float3* normals     = obj.normals();
	const auto    groups      = obj.groups();
	const auto    ccw         = obj.ccw_faces();
	const float4* tangents    = nullptr;

	// do a quick scan if the whole file needs normals (typical case)
	uint32_t ntexcoords       = 0;
	uint32_t nnormals         = 0;
	for (uint32_t i = 0; i < ngrp; i++)
	{
		const auto& group = groups[i];
		ntexcoords       += (int)group.has_texcoords & 0x1;
		nnormals         += (int)group.has_normals   & 0x1;
	}

	if (indices && positions)
	{
		if (normals && nnormals != ngrp)
			calc_vertex_normals(normals, indices, nidx, positions, nvtx, obj.ccw_faces(), nnormals == 0);

		if (texcoords && ntexcoords != ngrp)
		{
		#if 0
			double solve_time = 0.0;
			calc_texcoords(obj.aabb_min(), obj.aabb_max(), indices, nidx, positions, texcoords, nvtx, &solve_time);
		#else
			oTrace("[obj] requires texcoords: %s", path.c_str());
		#endif
		}
	}

	// define subsets
	// keep this before info because there may be a time where more subsets
	// need to be added because of 32-bit indices -> 16-bit indices.
	typedef std::vector<subset_t, ouro_std_allocator<subset_t>> subset_vector;
	subset_vector subsets(temp_alloc);
	{
		subsets.reserve(obj.num_groups() * 2);
		subsets.resize (obj.num_groups());

		const bool split_groups = nvtx > 65535; // break up any larger than 65535 so there's only one path of 16-bit indices beyond this point
		uint16_t   subset_flags = ccw ? subset_t::face_ccw : 0;

		for (uint32_t i = 0; i < ngrp; i++)
		{
			auto& sub = subsets[i];
			auto& grp = groups[i];

			sub.start_index  = grp.start_index;
			sub.num_indices  = grp.num_indices;
			sub.subset_flags = subset_flags;
			sub.unused       = 0;
			sub.material_id  = fnv1a<uint64_t>(grp.material);
			uint32_t min_idx = 0;

			if (split_groups)
			{
				uint32_t max_idx;
				min_max_indices(indices, 0, nidx, nvtx, &min_idx, &max_idx);

				// @tony: how to fix a triangle that is 65536 196608 786432?
				// have to retriangulate/reproduce vertices within 64k of 
				// each other.

				if ((max_idx - min_idx) > 65535)
					throw std::invalid_argument("can't handle a group with double-16-bit sizing yet");

				uint32_t ngrpvtx = grp.start_index + grp.num_indices;

				if (max_idx > 65535)
				{
					uint32_t* writable_indices = (uint32_t*)indices;
					for (uint32_t j = grp.start_index; j < ngrpvtx; j++)
						writable_indices[j] -= min_idx;
				}
			}

			sub.start_vertex = min_idx;
		}
	}

	mesh::info_t info;
	{
		auto extents           = (obj.aabb_max() - obj.aabb_min()) * 0.5f;
		auto nsubsets          = (uint16_t)ngrp;
		info.num_indices       = nidx;
		info.num_vertices      = nvtx;
		info.num_subsets       = nsubsets;
		info.num_slots         = (uint8_t)layout_slots(desired_layout);
		info.log2scale         = calc_log2scale(extents);
		info.primitive_type    = primitive_type::triangles;
		info.face_type         = ccw ? face_type::front_ccw : face_type::front_cw;
		info.flags             = 0;
		info.bounding_sphere   = positions ? calc_sphere(positions, sizeof(float3), nvtx) : float4(0.0f, 0.0f, 0.0f, 0.0f);
		info.extents           = extents;
		info.avg_edge_length   = 1.0f;
		info.avg_texel_density = float2(1.0f, 1.0f);
		info.layout            = desired_layout;
	
		lod_t lod;
		lod.opaque_color.start_subset  = 0; lod.opaque_color.num_subsets  = nsubsets;
		lod.opaque_shadow.start_subset = 0; lod.opaque_shadow.num_subsets = nsubsets;
		lod.collision.start_subset     = 0; lod.collision.num_subsets     = nsubsets;

		info.lods.fill(lod);
	}

	model mdl(info, subsets_alloc, mesh_alloc);
	memcpy(mdl.subsets(), subsets.data(), subsets.size() * sizeof(subset_t));
	copy_indices(mdl.indices(), indices, nidx);

	// set up vertex copy
	const void*    obj_slots[4] = { positions, texcoords, normals, tangents };
	const uint32_t dst_nslots   = info.num_slots;
	void**         dst_slots    = (void**)alloca(dst_nslots * sizeof(void*));
	for (uint32_t slot = 0; slot < dst_nslots; slot++)
		dst_slots[slot] = mdl.vertices(slot);

	// swizzle into the proper format
	layout_t src_layout = mesh::layout(mesh::basic::wavefront_obj);
	src_layout[3] = { element_semantic::tangent,  0, surface::format::r32g32b32a32_float, 3 };

	copy_vertices(dst_slots, info.layout, obj_slots, src_layout, nvtx);

	return mdl;
}

}}
