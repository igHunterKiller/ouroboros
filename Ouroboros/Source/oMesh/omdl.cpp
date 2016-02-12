// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/file_format.h>
#include <oCore/fourcc.h>
#include <oMesh/codec.h>

namespace ouro { namespace mesh {

static const fourcc_t omdl_signature = oFOURCC('o','m','d','l');
static const fourcc_t omdl_info_signature = oFOURCC('i','n','f','o');
static const fourcc_t omdl_subsets_signature = oFOURCC('s','b','s','t');
static const fourcc_t omdl_indices_signature = oFOURCC('i','n','d','x');
static const fourcc_t omdl_vertex_slot_signature = oFOURCC('s','l','o','t');

bool is_omdl(const void* buffer, size_t size)
{
	return size >= sizeof(file_header) && ((const file_header*)buffer)->fourcc == omdl_signature;
}

blob encode_omdl(const model& mdl
	, const allocator& file_alloc
	, const allocator& temp_alloc)
{
	const auto& info = mdl.info();

	uint32_t nslots = info.num_slots;
	const auto subsets_bytes = info.num_subsets * (uint32_t)sizeof(subset_t);
	const auto indices_bytes = info.num_indices * (uint32_t)sizeof(uint16_t);
	const auto vertices_bytes = info.num_vertices * layout_size(info.layout);

	const auto bytes = sizeof(file_header) 
		+ sizeof(file_chunk) + sizeof(info_t) 
		+ sizeof(file_chunk) + subsets_bytes
		+ sizeof(file_chunk) + indices_bytes
		+ sizeof(file_chunk) * nslots + vertices_bytes;

	auto mem = file_alloc.scoped_allocate(bytes, "encoded model");
	auto hdr = (file_header*)mem;

	hdr->fourcc = omdl_signature;
	hdr->num_chunks = 4;
	hdr->compression = compression_type::none; // not yet implemented, though here's where it gets done
	hdr->reserved = 0;
	hdr->version_hash = 0; // not yet implemented

	auto chk = hdr->first_chunk();
	chk->fourcc = omdl_info_signature;
	chk->chunk_bytes = sizeof(info_t);
	chk->uncompressed_bytes = chk->chunk_bytes;
	memcpy(chk->data<info_t>(), &info, sizeof(info_t));

	chk = chk->next();
	chk->fourcc = omdl_subsets_signature;
	chk->chunk_bytes = subsets_bytes;
	chk->uncompressed_bytes = chk->chunk_bytes;
	memcpy(chk->data<subset_t>(), mdl.subsets(), chk->chunk_bytes);

	chk = chk->next();
	chk->fourcc = omdl_indices_signature;
	chk->chunk_bytes = indices_bytes;
	chk->uncompressed_bytes = chk->chunk_bytes;
	memcpy(chk->data<uint16_t>(), mdl.indices(), chk->chunk_bytes);

	for (uint32_t slot = 0; slot < nslots; slot++)
	{
		chk = chk->next();
		chk->fourcc = omdl_vertex_slot_signature;
		chk->chunk_bytes = vertices_bytes;
		chk->uncompressed_bytes = chk->chunk_bytes;
		memcpy(chk->data<void>(), mdl.vertices(slot), chk->chunk_bytes);
	}

	return mem;
}

model decode_omdl(const void* buffer, size_t size
	, const layout_t& desired_layout
	, const allocator& subsets_alloc
	, const allocator& mesh_alloc
	, const allocator& temp_alloc)
{
	if (!is_omdl(buffer, size))
		return model();

	auto hdr = (const file_header*)buffer;
	auto chk = hdr->find_chunk(omdl_info_signature);

	if (!chk)
		throw std::invalid_argument("invalid omdl: no info section");

	auto info = *chk->data<info_t>();
	
	model mdl(info, subsets_alloc, mesh_alloc);

	chk = chk->next();
	
	if (chk->fourcc != omdl_subsets_signature)
		throw std::invalid_argument("invalid omdl: no subsets section");

	memcpy(mdl.subsets(), chk->data<subset_t>(), chk->uncompressed_bytes);

	chk = chk->next();

	if (chk->fourcc != omdl_indices_signature)
		throw std::invalid_argument("invalid omdl: no indices section");

	memcpy(mdl.indices(), chk->data<uint16_t>(), chk->uncompressed_bytes);

	const uint32_t nslots = info.num_slots;
	for (uint32_t slot = 0; slot < nslots; slot++)
	{
		chk = chk->next();

		if (chk->fourcc != omdl_vertex_slot_signature)
			throw std::invalid_argument("invalid omdl: no vertices slot section");

		memcpy(mdl.vertices(slot), chk->data<void>(), chk->uncompressed_bytes);
	}

	return mdl;
}

}}
