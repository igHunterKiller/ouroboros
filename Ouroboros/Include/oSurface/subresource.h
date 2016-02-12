// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// API for working with subresources. These describe a depth slice of a surface.
// Here is an example of a subresource:
//
// |-A-|   A: row pitch
// +---+-  B: depth pitch
// | 0 |B
// +---+-  Subresources can be identified by an ID that represents a consistent 
// | 1 |   subresource for a constant surface topology. So given an info_t and a 
// +---+   subresource id, the subresource itself can be calculated.
// | 2 |
// +---+
// |...|
// +---+

#pragma once
#include <oSurface/surface.h>

namespace ouro { namespace surface {

struct subresource_info_t
{
	subresource_info_t()
		: dimensions(0, 0, 0)
		, mip_level(0)
		, array_slice(0)
		, subsurface(0)
		, format(format::unknown)
	{}

	uint3 dimensions;
	uint32_t mip_level;
	uint32_t array_slice;
	uint32_t subsurface;
	format format;
};

struct mapped_subresource
{
	mapped_subresource() : data(nullptr), row_pitch(0), depth_pitch(0) {}

	void* data;
	uint32_t row_pitch;
	uint32_t depth_pitch;
};

struct const_mapped_subresource
{
	const_mapped_subresource() : data(nullptr), row_pitch(0), depth_pitch(0) {}
	const_mapped_subresource(const mapped_subresource& that) { operator=(that); }
	const_mapped_subresource(const const_mapped_subresource& that) { operator=(that); }

	// allow assignment from a mapped_subresource
	const const_mapped_subresource& operator=(const mapped_subresource& that)
		{ data = that.data; row_pitch = that.row_pitch; depth_pitch = that.depth_pitch; return *this; }

	const const_mapped_subresource& operator=(const const_mapped_subresource& that)
		{ data = that.data; row_pitch = that.row_pitch; depth_pitch = that.depth_pitch; return *this; }

	const void* data;
	uint32_t row_pitch;
	uint32_t depth_pitch;
};

inline uint32_t calc_subresource(uint32_t mip, uint32_t array_slice, uint32_t subsurface, uint32_t num_mips, uint32_t num_slices){ uint32_t nmips = ::max(1u, num_mips); return mip + (array_slice * nmips) + (subsurface * nmips * ::max(1u, num_slices)); }
inline void decode_subresource(uint32_t subresource, uint32_t num_mips, uint32_t num_slices, uint32_t* out_mip, uint32_t* out_array_slice, uint32_t* out_subsurface) { uint32_t nmips = max(1u, num_mips); uint32_t as = max(1u, num_slices); *out_mip = subresource % nmips; *out_array_slice = (subresource / nmips) % as; *out_subsurface = subresource / (nmips * as); }

inline uint32_t num_subresources(const info_t& info) { return max(1u, num_mips(info.mip_layout, info.dimensions)) * info.safe_array_size(); }

subresource_info_t subresourceinfo(const info_t& info, uint32_t subresource);

uint32_t subresource_size(const subresource_info_t& subresource_info); // byte size of subresource
inline uint32_t subresource_size(const info_t& info, uint32_t subresource) { return subresource_size(subresourceinfo(info, subresource)); }

uint32_t subresource_offset(const info_t& info, uint32_t subresource, uint32_t depth = 0);

inline uint2 byte_dimensions(const subresource_info_t& subresource_info) { return uint2(row_size(subresource_info.format, subresource_info.dimensions.xy(), subresource_info.subsurface), num_rows(subresource_info.format, subresource_info.dimensions.xy(), subresource_info.subsurface)); }
inline uint2 byte_dimensions(const info_t& info, uint32_t subresource) { return byte_dimensions(subresourceinfo(info, subresource)); }

const_mapped_subresource map_const_subresource(const info_t& info, uint32_t subresource, const void* surface_bytes, uint2* out_byte_dimensions = nullptr);
mapped_subresource map_subresource(const info_t& info, uint32_t subresource, void* surface_bytes, uint2* out_byte_dimensions = nullptr);

const_mapped_subresource map_const_subresource(const info_t& info, uint32_t subresource, uint32_t depth, const void* surface_bytes, uint2* out_byte_dimensions = nullptr);
mapped_subresource map_subresource(const info_t& info, uint32_t subresource, uint32_t depth, void* surface_bytes, uint2* out_byte_dimensions = nullptr);

}}
