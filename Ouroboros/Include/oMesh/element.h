// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// api for working with vertex streams and their layout

#pragma once
#include <oSurface/surface.h>
#include <array>

namespace ouro { namespace mesh {

static const uint32_t max_num_slots = 8;
static const uint32_t max_num_elements = 16;

enum class element_semantic : uint8_t { unknown, position, normal, tangent, texcoord, color, misc };

struct element_t
{
	element_semantic semantic;
	uint8_t index;
	surface::format format;
	uint8_t slot;
};
static_assert(sizeof(element_t) == 4, "size mismatch");

struct celement_t : element_t
{
	celement_t(const element_semantic& semantic = element_semantic::unknown, uint8_t index = 0, const surface::format& format = surface::format::unknown, uint8_t slot = 0)
	{
		this->semantic = semantic;
		this->index = index;
		this->format = format;
		this->slot = slot;
	}
};

// bytes from base where element data begins
uint32_t element_offset(const element_t* elements, size_t num_elements, uint32_t element_index);

// number of valid slots
uint32_t layout_slots(const element_t* elements, size_t num_elements);

// total size of all elements of the specified slot
uint32_t layout_size(const element_t* elements, size_t num_elements, uint32_t slot);

// total size of all elements in all slots combined 
uint32_t layout_size(const element_t* elements, size_t num_elements);

// total size of all elements in all slots combined, and captures the number of slots
uint32_t layout_size(const element_t* elements, size_t num_elements, uint32_t* out_nslots);

template<size_t size> uint32_t element_offset(const element_t (&elements)[size], uint32_t element_index) { return element_offset(elements, size); }
template<size_t size> uint32_t layout_slots(const element_t (&elements)[size]) { return layout_slots(elements, size); }
template<size_t size> uint32_t layout_size(const element_t (&elements)[size], uint32_t slot) { return layout_size(elements, size, slot); }
template<size_t size> uint32_t layout_size(const element_t (&elements)[size]) { return layout_size(elements, size); }
template<size_t size> uint32_t layout_size(const element_t (&elements)[size], uint32_t* out_nslots) { return layout_size(elements, size, out_nslots); }

// convenience wrapper for a single object to encapsulate a vertex
typedef std::array<element_t, max_num_elements> layout_t;

layout_t                       layout(const element_t* elements, size_t num_elements);
template<size_t size> layout_t layout(const element_t (&elements)[size]) { return layout(elements, size); }

template<size_t size> uint32_t element_offset(const std::array<element_t, size>& elements, uint32_t element_index) { return element_offset(elements.data(), elements.size(), element_index); }
template<size_t size> uint32_t layout_slots(const std::array<element_t, size>& elements) { return layout_slots(elements.data(), elements.size()); }
template<size_t size> uint32_t layout_size(const std::array<element_t, size>& elements, uint32_t slot) { return layout_size(elements.data(), elements.size(), slot); }
template<size_t size> uint32_t layout_size(const std::array<element_t, size>& elements) { return layout_size(elements.data(), elements.size()); }
template<size_t size> uint32_t layout_size(const std::array<element_t, size>& elements, uint32_t* out_nslots) { return layout_size(elements.data(), elements.size(), out_nslots); }

namespace basic {

	static const element_t pos[] = 
	{
		{ element_semantic::position, 0, surface::format::r32g32b32_float, 0 },
	};

	static const element_t pos_col[] = 
	{
		{ element_semantic::position, 0, surface::format::r32g32b32_float, 0 },
		{ element_semantic::color,    0, surface::format::b8g8r8a8_unorm,  0 },
	};

	static const element_t pos_uv0[] = 
	{
		{ element_semantic::position, 0, surface::format::r32g32b32_float, 0 },
		{ element_semantic::texcoord, 0, surface::format::r32g32_float,    0 },
	};

	static const element_t pos_uvw[] = 
	{
		{ element_semantic::position, 0, surface::format::r32g32b32_float, 0 },
		{ element_semantic::texcoord, 0, surface::format::r32g32b32_float, 0 },
	};

	static const element_t wavefront_obj[] = 
	{
		{ element_semantic::position, 0, surface::format::r32g32b32_float, 0 },
		{ element_semantic::texcoord, 0, surface::format::r32g32b32_float, 1 },
		{ element_semantic::normal,   0, surface::format::r32g32b32_float, 2 },
	};

	static const element_t meshf[] = 
	{
		{ element_semantic::position, 0, surface::format::r32g32b32_float,    0 },
		{ element_semantic::normal,   0, surface::format::r32g32b32_float,    0 },
		{ element_semantic::tangent,  0, surface::format::r32g32b32a32_float, 0 },
		{ element_semantic::texcoord, 0, surface::format::r32g32_float,       0 },
	};

}}}
