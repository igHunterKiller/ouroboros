// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// api for handling buffers of indices for irregular networks of points, 
// lines, indices and triangles.

#pragma once
//#include <oArch/arch.h>

namespace ouro { namespace mesh {

enum class primitive_type : uint8_t
{
	unknown,
	points,
	lines,
	line_strips,
	triangles,
	triangle_strips,
	lines_adjacency,
	line_strips_adjacency,
	triangles_adjacency,
	triangle_strips_adjacency,
	patches1, // # postfix is the # of control points per patch
	patches2,
	patches3,
	patches4,
	patches5,
	patches6,
	patches7,
	patches8,
	patches9,
	patches10,
	patches11,
	patches12,
	patches13,
	patches14,
	patches15,
	patches16,
	patches17,
	patches18,
	patches19,
	patches20,
	patches21,
	patches22,
	patches23,
	patches24,
	patches25,
	patches26,
	patches27,
	patches28,
	patches29,
	patches30,
	patches31,
	patches32,
	
	count,
};

enum class face_type : uint8_t
{
	unknown,
	front_ccw,
	front_cw,
	outline,

	count,
};

uint32_t num_primitives(const primitive_type& type, uint32_t num_indices, uint32_t num_vertices);

inline bool has_16bit_indices(uint32_t num_vertices) { return num_vertices <= 65535; }
inline uint32_t index_size(uint32_t num_vertices) { return has_16bit_indices(num_vertices) ? sizeof(uint16_t) : sizeof(uint32_t); }

// swaps indices to have a triangles in the list face the other way
void flip_winding_order(uint32_t base_index_index, uint16_t* indices, uint32_t num_indices);
void flip_winding_order(uint32_t base_index_index, uint32_t* indices, uint32_t num_indices);

// copies index buffers from one to another, properly converting from 16-bit to 32-bit and vice versa.
void copy_indices(void* oRESTRICT dst, uint32_t dst_pitch, const void* oRESTRICT src, uint32_t src_pitch, uint32_t num_indices);
void copy_indices(uint16_t* oRESTRICT dst, const uint32_t* oRESTRICT src, uint32_t num_indices);
void copy_indices(uint32_t* oRESTRICT dst, const uint16_t* oRESTRICT src, uint32_t num_indices);

// Adds offset to each index
void offset_indices(uint16_t* dst, uint32_t num_indices, int offset);
void offset_indices(uint32_t* dst, uint32_t num_indices, int offset);

// Finds the min/max index value used in the specified array. It walks the 
// specified array from start_index for num_indices. If indices is nullptr, then
// the min = 0, max = num_vertices - 1.
void min_max_indices(const uint32_t* oRESTRICT indices, uint32_t start_index, uint32_t num_indices, uint32_t num_vertices, uint32_t* oRESTRICT out_min_vertex, uint32_t* oRESTRICT out_max_vertex);
void min_max_indices(const uint16_t* oRESTRICT indices, uint32_t start_index, uint32_t num_indices, uint32_t num_vertices, uint16_t* oRESTRICT out_min_vertex, uint16_t* oRESTRICT out_max_vertex);

}}
