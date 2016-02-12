// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Algorithms for working with surfaces

#pragma once
#include <oSurface/subresource.h>
#include <functional>

namespace ouro { namespace surface {

// Single-pixel get/put API. Only use this for debug/non-performant cases. This
// currently only supports common r rgb and rgba cases.
void put(const subresource_info_t& subresource_info, const mapped_subresource& dst, const uint2& coord, uint32_t argb);
uint32_t get(const subresource_info_t& subresource_info, const const_mapped_subresource& src, const uint2& coord);

// A suggestion on whether or not you should use large memory pages. A little 
// bit arbitrary but initially always returns false if your image is smaller 
// than half a large page to ensure your not wasting too much memory. After 
// that this will return true if each tile won't copy at least half a small page 
// worth before encountering a tlb miss.
bool use_large_pages(const info_t& info, const uint2& tile_dimensions, uint32_t small_page_size_bytes, uint32_t large_page_size_bytes);

// Calls the specified function on each pixel.
void enumerate_pixels(const uint2& dimensions, const format& format
	, const const_mapped_subresource& mapped
	, const std::function<void(const void* pixel)>& enumerator);

void enumerate_pixels(const uint2& dimensions, const format& format
	, const mapped_subresource& mapped
	, const std::function<void(void* pixel)>& enumerator);

// Calls the specified function on each pixel of two same-formatted surfaces.
void enumerate_pixels(const uint2& dimensions, const format& format
	, const const_mapped_subresource& mapped1
	, const const_mapped_subresource& mapped2
	, const std::function<void(const void* oRESTRICT pixel1, const void* oRESTRICT pixel2)>& enumerator);

// Calls the specified function on each pixel of two same-formatted surfaces.
// and writes to a 3rd other-format surface.
void enumerate_pixels(const info_t& input_info
	, const const_mapped_subresource& mappedInput1
	, const const_mapped_subresource& mappedInput2
	, const info_t& output_info
	, mapped_subresource& mapped_output
	, const std::function<void(const void* oRESTRICT pixel1, const void* oRESTRICT pixel2, void* oRESTRICT out_pixel)>& enumerator);

// Returns the root mean square of the difference between the two surfaces.
float calc_rms(const info_t& info
	, const const_mapped_subresource& mapped1
	, const const_mapped_subresource& mapped2);

// Fills the output surface with abs(Input1 - Input2) for each pixel and returns 
// the root mean square. Currently the only supportedoutput image is r8_unorm.
// Inputs can be r8_unorm, b8g8r8_unorm, b8g8r8a8_unorm
float calc_rms(const info_t& input_info
	, const const_mapped_subresource& mappedInput1
	, const const_mapped_subresource& mappedInput2
	, const info_t& output_info
	, mapped_subresource& mapped_output);

// Fills the specified array with the count of pixels at each luminance value.
// (lum [0,1] mapped to [0,255] or [0,65536]).
void histogram8(const info_t& info, const const_mapped_subresource& mapped, uint32_t histogram[256]);
void histogram16(const info_t& info, const const_mapped_subresource& mapped, uint32_t histogram[65536]);

}}
