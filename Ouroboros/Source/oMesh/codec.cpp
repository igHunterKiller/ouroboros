// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oMesh/codec.h>
#include <oString/string.h>

namespace ouro { namespace mesh {

// add format extension to this list and it will propagate to all the apis below
#define FOREACH_EXT(macro) macro(omdl) macro(obj)

// _____________________________________________________________________________
// Boilerplate (don't use directly, they're registered with the functions below)

#define DECLARE_CODEC(ext) \
	bool is_##ext(const void* buffer, size_t size); \
	blob encode_##ext(const model& mdl, const allocator& file_alloc, const allocator& temp_alloc); \
	model decode_##ext(const void* buffer, size_t size, const layout_t& desired_layout, const allocator& subsets_alloc, const allocator& mesh_alloc, const allocator& temp_alloc);

	/// The challenge with get_info() is that often it's not known before a full parse
	//info_t get_info_##ext(const void* buffer, size_t size);

	/// Required input isn't enough information to generate an encoding, so this doesn't make sense
	//format required_input_##ext(const format& stored);

#define GET_FILE_FORMAT_EXT(ext) if (!_stricmp(extension, "." #ext)) return file_format::##ext;
#define GET_FILE_FORMAT_HEADER(ext) if (is_##ext(buffer, size)) return file_format::##ext;
#define ENCODE(ext) case file_format::##ext: return encode_##ext(mdl, file_alloc, temp_alloc);
#define DECODE(ext) case file_format::##ext: decoded = decode_##ext(buffer, size, desired_layout, subsets_alloc, mesh_alloc, temp_alloc); break;
#define AS_STRING(ext) case mesh::file_format::##ext: return #ext;
//#define GET_INFO(ext) case file_format::##ext: return get_info_##ext(buffer, size);
//#define GET_REQ_INPUT(ext) case file_format::##ext: return required_input_##ext(stored_format);

FOREACH_EXT(DECLARE_CODEC)

file_format get_file_format(const char* path)
{
	const char* extension = rstrstr(path, ".");
	FOREACH_EXT(GET_FILE_FORMAT_EXT)
	return file_format::unknown;
}

file_format get_file_format(const void* buffer, size_t size)
{
	FOREACH_EXT(GET_FILE_FORMAT_HEADER)
	return file_format::unknown;
}

blob encode(const model& mdl
	, const file_format& fmt
	, const allocator& file_alloc
	, const allocator& temp_alloc)
{
	switch (fmt)
	{ FOREACH_EXT(ENCODE)
		default: break;
	}
	throw std::exception("unknown mesh encoding");
}

model decode(const void* buffer, size_t size
	, const layout_t& desired_layout
	, const allocator& subsets_alloc
	, const allocator& mesh_alloc
	, const allocator& temp_alloc)
{
	model decoded;
	switch (get_file_format(buffer, size))
	{ FOREACH_EXT(DECODE)
		default: throw std::exception("unknown mesh encoding");
	}

	return decoded;
}

#if 0
info_t get_info(const void* buffer, size_t size)
{
	switch (get_file_format(buffer, size))
	{	FOREACH_EXT(GET_INFO)
		default: break;
	}
	throw std::exception("unknown image encoding");
}
	
format required_input(const file_format& file_format, const format& stored_format)
{
	switch (file_format)
	{
		FOREACH_EXT(GET_REQ_INPUT)
		default: break;
	}

	return format::unknown;
}
#endif
	}

template<> const char* as_string(const mesh::file_format& ff)
{
	switch (ff)
	{
		FOREACH_EXT(AS_STRING)
		default: break;
	}
	return "?";
}

}
