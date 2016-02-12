// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oMesh/element.h>

namespace ouro { namespace mesh {

uint32_t element_offset(const element_t* elements, size_t num_elements, uint32_t element_index)
{
	uint32_t offset = 0;
	const uint32_t slot = elements[element_index].slot;
	for (size_t i = 0; i < element_index; i++)
		if (elements[i].slot == slot)
			offset += surface::element_size(elements[i].format);
	return offset;
}

uint32_t layout_slots(const element_t* elements, size_t num_elements)
{
  uint8_t max_ = 0;
  for (size_t i = 0; i < num_elements; i++)
    max_ = max(max_, elements[i].slot);
	return max_ + 1;
}

uint32_t layout_size(const element_t* elements, size_t num_elements, uint32_t slot)
{
	uint32_t size = 0;
	for (size_t i = 0; i < num_elements; i++)
		if (elements[i].slot == slot)
			size += surface::element_size(elements[i].format);
	return size;
}

uint32_t layout_size(const element_t* elements, size_t num_elements)
{
	uint32_t vertex_size = 0;
	for (uint32_t slot = 0; slot < max_num_slots; slot++)
	{
		uint32_t slot_size = layout_size(elements, num_elements, slot);
		if (slot_size)
			vertex_size += slot_size;
	}

	return vertex_size;
}

uint32_t layout_size(const element_t* elements, size_t num_elements, uint32_t* out_nslots)
{
	uint32_t vertex_size = 0;
	uint32_t nslots = 0;
	for (uint32_t slot = 0; slot < max_num_slots; slot++)
	{
		uint32_t slot_size = layout_size(elements, num_elements, slot);
		if (slot_size)
		{
			vertex_size += slot_size;
			nslots++;
		}
	}

	if (out_nslots)
		*out_nslots = nslots;
	return vertex_size;
}

}}
