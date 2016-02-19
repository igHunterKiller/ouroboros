// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oMesh/face.h>
#include <oMemory/memory.h>

#define STR_SUPPORT(_T) oDEFINE_ENUM_TO_STRING(_T) oDEFINE_ENUM_FROM_STRING(_T)

#define oFACE_CHECK(expr, format, ...) do { if (!(expr)) oThrow(std::errc::invalid_argument, format, ## __VA_ARGS__); } while(false)

namespace ouro {

const char* as_string(const mesh::primitive_type& type)
{
	switch (type)
	{
		case mesh::primitive_type::unknown: return "unknown";
		case mesh::primitive_type::points: return "points";
		case mesh::primitive_type::lines: return "lines";
		case mesh::primitive_type::line_strips: return "line_strips";
		case mesh::primitive_type::triangles: return "triangles";
		case mesh::primitive_type::triangle_strips: return "triangle_strips";
		case mesh::primitive_type::lines_adjacency: return "lines_adjacency";
		case mesh::primitive_type::line_strips_adjacency: return "line_strips_adjacency";
		case mesh::primitive_type::triangles_adjacency: return "triangles_adjacency";
		case mesh::primitive_type::triangle_strips_adjacency: return "triangle_strips_adjacency";
		case mesh::primitive_type::patches1: return "patches1";
		case mesh::primitive_type::patches2: return "patches2";
		case mesh::primitive_type::patches3: return "patches3";
		case mesh::primitive_type::patches4: return "patches4";
		case mesh::primitive_type::patches5: return "patches5";
		case mesh::primitive_type::patches6: return "patches6";
		case mesh::primitive_type::patches7: return "patches7";
		case mesh::primitive_type::patches8: return "patches8";
		case mesh::primitive_type::patches9: return "patches9";
		case mesh::primitive_type::patches10: return "patches10";
		case mesh::primitive_type::patches11: return "patches11";
		case mesh::primitive_type::patches12: return "patches12";
		case mesh::primitive_type::patches13: return "patches13";
		case mesh::primitive_type::patches14: return "patches14";
		case mesh::primitive_type::patches15: return "patches15";
		case mesh::primitive_type::patches16: return "patches16";
		case mesh::primitive_type::patches17: return "patches17";
		case mesh::primitive_type::patches18: return "patches18";
		case mesh::primitive_type::patches19: return "patches19";
		case mesh::primitive_type::patches20: return "patches20";
		case mesh::primitive_type::patches21: return "patches21";
		case mesh::primitive_type::patches22: return "patches22";
		case mesh::primitive_type::patches23: return "patches23";
		case mesh::primitive_type::patches24: return "patches24";
		case mesh::primitive_type::patches25: return "patches25";
		case mesh::primitive_type::patches26: return "patches26";
		case mesh::primitive_type::patches27: return "patches27";
		case mesh::primitive_type::patches28: return "patches28";
		case mesh::primitive_type::patches29: return "patches29";
		case mesh::primitive_type::patches30: return "patches30";
		case mesh::primitive_type::patches31: return "patches31";
		case mesh::primitive_type::patches32: return "patches32";

		default: break;
	}
	return "?";
}

STR_SUPPORT(mesh::primitive_type);

const char* as_string(const mesh::face_type& type)
{
	switch (type)
	{
		case mesh::face_type::unknown: return "unknown";
		case mesh::face_type::front_ccw: return "front_ccw";
		case mesh::face_type::front_cw: return "front_cw";
		case mesh::face_type::outline: return "outline";
		default: break;
	}
	return "?";
}
		
STR_SUPPORT(mesh::face_type);

namespace mesh {

uint32_t num_primitives(const primitive_type& type, uint32_t num_indices, uint32_t num_vertices)
{
	uint32_t n = num_indices;
	switch (type)
	{
		case primitive_type::points: n = num_vertices; break;
		case primitive_type::lines: n /= 2; break;
		case primitive_type::line_strips: n--; break;
		case primitive_type::triangles: n /= 3; break;
		case primitive_type::triangle_strips: n -= 2; break;
		default: oThrow(std::errc::invalid_argument, "unsupported primitive type");
	}
	return n;
}

void flip_winding_order(uint32_t base_index_index, uint16_t* indices, uint32_t num_indices)
{
	oFACE_CHECK((base_index_index % 3) == 0, "Indices is not divisible by 3, so thus is not triangles and cannot be re-winded");
	oFACE_CHECK((num_indices % 3) == 0, "Indices is not divisible by 3, so thus is not triangles and cannot be re-winded");
	for (uint32_t i = base_index_index; i < num_indices; i += 3)
		std::swap(indices[i+1], indices[i+2]);
}

void flip_winding_order(uint32_t base_index_index, uint32_t* indices, uint32_t num_indices)
{
	oFACE_CHECK((base_index_index % 3) == 0, "Indices is not divisible by 3, so thus is not triangles and cannot be re-winded");
	oFACE_CHECK((num_indices % 3) == 0, "Indices is not divisible by 3, so thus is not triangles and cannot be re-winded");
	for (uint32_t i = base_index_index; i < num_indices; i += 3)
		std::swap(indices[i+1], indices[i+2]);
}

void copy_indices(void* oRESTRICT dst, uint32_t dst_pitch, const void* oRESTRICT src, uint32_t src_pitch, uint32_t num_indices)
{
	if (dst_pitch == sizeof(uint32_t) && src_pitch == sizeof(uint16_t))
		copy_indices((uint32_t*)dst, (const uint16_t*)src, num_indices);
	else if (dst_pitch == sizeof(uint16_t) && src_pitch == sizeof(uint32_t))
		copy_indices((uint16_t*)dst, (const uint32_t*)src, num_indices);
	else
		oThrow(std::errc::invalid_argument, "unsupported index pitches (src=%d, dst=%d)", src_pitch, dst_pitch);
}

void copy_indices(uint16_t* oRESTRICT dst, const uint32_t* oRESTRICT src, uint32_t num_indices)
{
	const uint32_t* end = &src[num_indices];
	while (src < end)
	{
		oCheck(*src <= 65535, std::errc::invalid_argument, "truncating a uint32_t (%d) to a uint16_t in a way that will change its value.", *src);
		*dst++ = (*src++) & 0xffff;
	}
}

void copy_indices(uint32_t* oRESTRICT dst, const uint16_t* oRESTRICT src, uint32_t num_indices)
{
	const uint16_t* end = &src[num_indices];
	while (src < end)
		*dst++ = *src++;
}

void offset_indices(uint16_t* oRESTRICT dst, uint32_t num_indices, int offset)
{
	const uint16_t* end = dst + num_indices;
	while (dst < end)
	{
		uint32_t i = *dst + offset;
		if ( i & 0xffff0000)
			throw std::out_of_range("indices with offset push it out of range");
		*dst++ = static_cast<uint16_t>(i);
	}
}

void offset_indices(uint32_t* oRESTRICT dst, uint32_t num_indices, int offset)
{
	const uint32_t* end = dst + num_indices;
	while (dst < end)
	{
		uint64_t i = *dst + offset;
		if ( i & 0xffffffff00000000)
			throw std::out_of_range("indices with offset push it out of range");
		*dst++ = static_cast<uint32_t>(i);
	}
}

template<typename T> void min_max_indicesT(const T* oRESTRICT indices
	, uint32_t start_index, uint32_t num_indices, uint32_t num_vertices
	, T* oRESTRICT out_min_vertex, T* oRESTRICT out_max_vertex)
{
	if (indices)
	{
		*out_min_vertex = T(-1);
		*out_max_vertex = 0;

		const uint32_t end = start_index + num_indices;
		for (uint32_t i = start_index; i < end; i++)
		{
			*out_min_vertex = min(*out_min_vertex, indices[i]);
			*out_max_vertex = max(*out_max_vertex, indices[i]);
		}
	}

	else
	{
		*out_min_vertex = 0;
		*out_max_vertex = static_cast<T>(num_vertices - 1);
	}
}

void min_max_indices(const uint32_t* oRESTRICT indices, uint32_t start_index, uint32_t num_indices, uint32_t num_vertices, uint32_t* oRESTRICT out_min_vertex, uint32_t* oRESTRICT out_max_vertex)
{
	min_max_indicesT(indices, start_index, num_indices, num_vertices, out_min_vertex, out_max_vertex);
}

void min_max_indices(const uint16_t* oRESTRICT indices, uint32_t start_index, uint32_t num_indices, uint32_t num_vertices, uint16_t* oRESTRICT out_min_vertex, uint16_t* oRESTRICT out_max_vertex)
{
	min_max_indicesT(indices, start_index, num_indices, num_vertices, out_min_vertex, out_max_vertex);
}

}}
