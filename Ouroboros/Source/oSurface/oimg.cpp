// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/file_format.h>
#include <oCore/fourcc.h>
#include <oSurface/codec.h>

namespace ouro { namespace surface {

static const fourcc_t oimg_signature = oFOURCC('o','i','m','g');
static const fourcc_t oimg_info_signature = oFOURCC('i','n','f','o');
static const fourcc_t oimg_bits_signature = oFOURCC('b','i','t','s');

bool is_oimg(const void* buffer, size_t size)
{
	return size >= sizeof(file_header) && ((const file_header*)buffer)->fourcc == oimg_signature;
}

format required_input_oimg(const format& stored)
{
	return stored;
}

info_t get_info_oimg(const void* buffer, size_t size)
{
	if (size < sizeof(file_header))
		return info_t();

	auto hdr = (const file_header*)buffer;

	auto chk = hdr->first_chunk();
	for (uint32_t i = 0; i < hdr->num_chunks; i++, chk = chk->next())
	{
		if (chk->in_range(buffer, size) && chk->fourcc == oimg_info_signature)
		{
			auto info = chk->data<info_t>();
			return *info;
		}
	}

	return info_t();
}

blob encode_oimg(const image& img, const allocator& file_alloc, const allocator& temp_alloc, const compression& compression)
{
	const auto& info = img.info();
	const auto bit_bytes = surface::total_size(info);
	const auto bytes = sizeof(file_header) + sizeof(file_chunk) + sizeof(info_t) + sizeof(file_chunk) + bit_bytes;

	auto mem = file_alloc.allocate(bytes, "encoded image");
	auto hdr = (file_header*)mem;

	hdr->fourcc = oimg_signature;
	hdr->num_chunks = 2;
	hdr->compression = compression_type::none; // not yet implemented, though here's where it gets done
	hdr->reserved = 0;
	hdr->version_hash = 0; // not yet implemented

	auto chk = hdr->first_chunk();
	chk->fourcc = oimg_info_signature;
	chk->chunk_bytes = sizeof(info_t);
	chk->uncompressed_bytes = chk->chunk_bytes;
	memcpy(chk->data<info_t>(), &info, sizeof(info_t));

	chk = chk->next();
	chk->fourcc = oimg_bits_signature;
	chk->chunk_bytes = bit_bytes;
	chk->uncompressed_bytes = chk->chunk_bytes;
	
	const_mapped_subresource mapped;
	img.map_const(0, &mapped);
	memcpy(chk->data<void>(), mapped.data, bit_bytes); // note: there may need to be a method of zeroing padding bits 
	img.unmap_const(0);

	return blob(mem, bytes, (blob::deleter_fn)file_alloc.deallocator());
}

image decode_oimg(const void* buffer, size_t size, const allocator& texel_alloc, const allocator& temp_alloc, const mip_layout& layout)
{
	info_t info = get_info_oimg(buffer, size);
	
	if (info.format == format::unknown)
		throw std::invalid_argument("invalid oimg buffer");

	image img(info, texel_alloc);

	auto hdr = (const file_header*)buffer;
	auto chk = hdr->find_chunk(oimg_bits_signature);
	if (!chk)
		throw std::invalid_argument("invalid oimg: no bits section");

	auto bytes = total_size(info);
	mapped_subresource mapped;
	img.map(0, &mapped);
	memcpy(mapped.data, chk->data<void>(), bytes);
	img.unmap(0);

	return img;
}

}}
