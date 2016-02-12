// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Facade for various conversion formats. Not all are supported but as 
// easy to add over time.

#pragma once
#include <oSurface/subresource.h>

namespace ouro { namespace surface {

// Copies surface texel data from one buffer to another, one format to another.
// There's a fast-path for same-formatted buffers so this can be used in most cases.
// If dst_format is a block-compressed format then the row pitches of src and dst 
// must match the width * format size for each respectively. Otherwise row pitches 
// can differ so that copying a subsurface out of a larger surface or copying into 
// a part of a surface can be done.
void convert_formatted(const mapped_subresource& dst, const format& dst_format
	, const const_mapped_subresource& src, const format& src_format
	, const uint3& dimensions, const copy_option& option = copy_option::none);

// Copies elements from one buffer to another, one format/layout to another. Instead 
// of rows and depth slices this concentrates on element pitch enabling homogenous 
// sources to be copied to interleaved buffers by setting up the src/dst pointers 
// correctly and stepping through each with the size of the overall struct, all while 
// converting the data format. This is useful for copying separate streams of uncompressed 
// vertex data into struct-style vertex definitions.
void convert_structured(void* oRESTRICT dst, uint32_t dst_elem_pitch, const format& dst_format
	, const void* oRESTRICT src, uint32_t src_elem_pitch, const format& src_format, uint32_t num_elements);

// This is a conversion in-place for RGB v. BGR and similar permutations.
void swizzle_formatted(const info_t& info, const format& new_format, const mapped_subresource& mapped);

}}
