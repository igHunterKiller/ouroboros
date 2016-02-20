// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oCore/assert.h>
#include <oMesh/face.h>
#include <oMemory/memory.h>

namespace ouro {

template<> const char* as_string(const mesh::primitive_type& type)
{
	static const char* s_names[] = 
	{
		"unknown",
		"points",
		"lines",
		"line_strips",
		"triangles",
		"triangle_strips",
		"lines_adjacency",
		"line_strips_adjacency",
		"triangles_adjacency",
		"triangle_strips_adjacency",
		"patches1",
		"patches2",
		"patches3",
		"patches4",
		"patches5",
		"patches6",
		"patches7",
		"patches8",
		"patches9",
		"patches10",
		"patches11",
		"patches12",
		"patches13",
		"patches14",
		"patches15",
		"patches16",
		"patches17",
		"patches18",
		"patches19",
		"patches20",
		"patches21",
		"patches22",
		"patches23",
		"patches24",
		"patches25",
		"patches26",
		"patches27",
		"patches28",
		"patches29",
		"patches30",
		"patches31",
		"patches32",
	};
	return as_string(type, s_names);
}

oDEFINE_TO_FROM_STRING(mesh::primitive_type);

template<> const char* as_string(const mesh::face_type& type)
{
	static const char* s_names[] = 
	{
		"unknown",
		"front_ccw",
		"front_cw",
		"outline",
	};
	return as_string(type, s_names);
}
		
oDEFINE_TO_FROM_STRING(mesh::face_type);

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
	oCheck((base_index_index % 3) == 0, std::errc::invalid_argument, "Indices is not divisible by 3, so thus is not triangles and cannot be re-winded");
	oCheck((num_indices % 3) == 0, std::errc::invalid_argument, "Indices is not divisible by 3, so thus is not triangles and cannot be re-winded");
	for (uint32_t i = base_index_index; i < num_indices; i += 3)
		std::swap(indices[i+1], indices[i+2]);
}

void flip_winding_order(uint32_t base_index_index, uint32_t* indices, uint32_t num_indices)
{
	oCheck((base_index_index % 3) == 0, std::errc::invalid_argument, "Indices is not divisible by 3, so thus is not triangles and cannot be re-winded");
	oCheck((num_indices % 3) == 0, std::errc::invalid_argument, "Indices is not divisible by 3, so thus is not triangles and cannot be re-winded");
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
