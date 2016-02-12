// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oSurface/subresource.h>

namespace ouro { namespace surface {

subresource_info_t subresourceinfo(const info_t& info, uint32_t subresource)
{
	subresource_info_t subresource_info;
	int nMips = num_mips(info.mip_layout, info.dimensions);
	decode_subresource(subresource, nMips, info.array_size, &subresource_info.mip_level, &subresource_info.array_slice, &subresource_info.subsurface);

	if (info.array_size && subresource_info.array_slice >= info.safe_array_size())
		throw std::out_of_range("array slice index is out of range");

	if (subresource_info.subsurface >= num_subformats(info.format))
		throw std::out_of_range("subsurface index is out of range for the specified surface");

	subresource_info.dimensions = dimensions_npot(info.format, info.dimensions, subresource_info.mip_level, subresource_info.subsurface);
	subresource_info.format = info.format;
	return subresource_info;
}

uint32_t subresource_size(const subresource_info_t& subresource_info)
{
	return mip_size(subresource_info.format, subresource_info.dimensions, subresource_info.subsurface);
}

uint32_t subresource_offset(const info_t& info, uint32_t subresource, uint32_t depth)
{
	if (depth >= info.dimensions.z)
		throw std::out_of_range("Depth index is out of range");

	auto subresource_info = subresourceinfo(info, subresource);
	uint32_t off = offset(info, subresource_info.mip_level, subresource_info.subsurface);
	if (depth)
		off += depth_pitch(info, subresource_info.mip_level, subresource_info.subsurface) * depth;
	else if (subresource_info.array_slice > 0)
		off += slice_pitch(info, subresource_info.subsurface) * subresource_info.array_slice;
	return off;
}

const_mapped_subresource map_const_subresourced(const info_t& info, uint32_t subresource, uint32_t depth, const void* surface_bytes, uint2* out_byte_dimensions)
{
	auto subresource_info =subresourceinfo(info, subresource);

	const_mapped_subresource mapped;
	mapped.row_pitch = row_pitch(info, subresource_info.mip_level, subresource_info.subsurface);
	mapped.depth_pitch = depth_pitch(info, subresource_info.mip_level, subresource_info.subsurface);
	mapped.data = (uint8_t*)surface_bytes + subresource_offset(info, subresource, depth);

	if (out_byte_dimensions)
		*out_byte_dimensions = byte_dimensions(info.format, subresource_info.dimensions.xy(), subresource_info.subsurface);
	return mapped;
}

mapped_subresource map_subresource(const info_t& info, uint32_t subresource, uint32_t depth, void* surface_bytes, uint2* out_byte_dimensions)
{
	const_mapped_subresource msr = map_const_subresourced(info, subresource, depth, surface_bytes, out_byte_dimensions);
	return (mapped_subresource&)msr;
}

const_mapped_subresource map_const_subresource(const info_t& info, uint32_t subresource, const void* surface_bytes, uint2* out_byte_dimensions)
{
	auto subresource_info = subresourceinfo(info, subresource);

	const_mapped_subresource mapped;
	mapped.row_pitch = row_pitch(info, subresource_info.mip_level, subresource_info.subsurface);
	mapped.depth_pitch = depth_pitch(info, subresource_info.mip_level, subresource_info.subsurface);
	mapped.data = (uint8_t*)surface_bytes + subresource_offset(info, subresource, 0);

	if (out_byte_dimensions)
		*out_byte_dimensions = byte_dimensions(info.format, subresource_info.dimensions.xy(), subresource_info.subsurface);
	return mapped;
}

mapped_subresource map_subresource(const info_t& info, uint32_t subresource, void* surface_bytes, uint2* out_byte_dimensions)
{
	const_mapped_subresource msr = map_const_subresource(info, subresource, surface_bytes, out_byte_dimensions);
	return (mapped_subresource&)msr;
}

}}
