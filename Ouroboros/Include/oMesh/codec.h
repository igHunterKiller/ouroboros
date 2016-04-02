// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Facade for encoding and decoding several mesh formats

#pragma once
#include <oString/path.h>
#include <oMesh/mesh.h>
#include <oMesh/model.h>

namespace ouro { namespace mesh {

enum class file_format : uint8_t
{
	unknown,
	obj,
	omdl,
};

// checks the file extension
file_format get_file_format(const char* path);

// checks the first few bytes/header info
file_format get_file_format(const void* buffer, size_t size);
inline file_format get_file_format(const blob& buffer) { return get_file_format(buffer, buffer.size()); }

// returns a buffer ready to be written to disk in the specified format.
// this may use temp_alloc to do conversions.
blob encode(const model& mdl
	, const file_format& fmt
	, const allocator& file_alloc = default_allocator
	, const allocator& temp_alloc = default_allocator);

// Parses the in-memory formatted buffer into a model. temp_alloc may be used
// for conversions. The path is available as pass-through, nothing is loaded.
model decode(const path_t& path
	, const void* buffer, size_t size
	, const layout_t& desired_layout
	, const allocator& subsets_alloc = default_allocator
	, const allocator& mesh_alloc = default_allocator
	, const allocator& temp_alloc = default_allocator);

inline model decode(const path_t& path
	, const blob& buffer
	, const layout_t& desired_layout
	, const allocator& subsets_alloc = default_allocator
	, const allocator& mesh_alloc = default_allocator
	, const allocator& temp_alloc = default_allocator)
	{ return decode(path, buffer, buffer.size(), desired_layout, subsets_alloc, mesh_alloc, temp_alloc); }

#if 0
// converts the first few bytes of a supported format into a surface::info
// if not recognized the returned format will be surface::unknown
info_t get_info(const void* buffer, size_t size);
inline info_t get_info(const blob& buffer) { return get_info(buffer, buffer.size()); }
#endif

}}
