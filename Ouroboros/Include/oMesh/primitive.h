// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Creates models filled with common primitive shapes. The model is allocated 
// with 'alloc'. Any temp working space is allocated with 'tmp' and all that memory 
// will be freed by the time the function returns.

// Facet: number of vertices used to describe alloc curve (usually alloc circle's)
// Divide: number of subdivisions. Keep this <= 6 for performance/memory sanity.
// Height: is along -+Z
// Apex: is at 0
// Base: is at height in +Z

// Note: circle can create a triangle (divide=3) or a square (divide=4)
// Note: cylinder can create a cone (apex_radius=0.0f) or square pyramid (divide=4)
// Note: If color is specified it will be set to vertex values. If left as 0 then
//       the color channel will not be generated.

#pragma once
#include <oMemory/allocate.h>
#include <oMesh/face.h>
#include <oMesh/model.h>

namespace ouro { namespace mesh {

model box(const allocator& alloc, const allocator& tmp, const face_type& type, const element_t* elements, size_t num_elements
	, const float3& aabb_min, const float3& aabb_max, uint32_t color = 0);

model circle(const allocator& alloc, const allocator& tmp, const face_type& type, const element_t* elements, size_t num_elements
	, const uint16_t facet, float radius, uint32_t color = 0);

model cylinder(const allocator& alloc, const allocator& tmp, const face_type& type, const element_t* elements, size_t num_elements
	, const uint16_t facet, const uint16_t divide
	, float base_radius, float apex_radius, float height, uint32_t color = 0);

model frustum(const allocator& alloc, const allocator& tmp, const face_type& type, const element_t* elements, size_t num_elements
	, const float4x4& projection, uint32_t color = 0);

model sphere(const allocator& alloc, const allocator& tmp, const face_type& type, const element_t* elements, size_t num_elements
	, float radius, uint32_t color = 0);

model torus(const allocator& alloc, const allocator& tmp, const face_type& type, const element_t* elements, size_t num_elements
	, const uint16_t facet, const uint16_t divide
	, float inner_radius, float outer_radius, uint32_t color = 0);

// C-array support
template<size_t size>
model box(const allocator& alloc, const allocator& tmp, const face_type& type, const element_t (&elements)[size]
	, const float3& aabb_min, const float3& aabb_max, uint32_t color = 0) { return box(alloc, tmp, type, elements, size, aabb_min, aabb_max, color); }

template<size_t size>
model circle(const allocator& alloc, const allocator& tmp, const face_type& type, const element_t (&elements)[size]
	, const uint16_t facet, float radius, uint32_t color = 0) { return circle(alloc, tmp, type, elements, size, facet, radius, color); }

template<size_t size>
model cylinder(const allocator& alloc, const allocator& tmp, const face_type& type, const element_t (&elements)[size]
	, const uint16_t facet, const uint16_t divide
	, float base_radius, float apex_radius, float height, uint32_t color = 0) { return cylinder(alloc, tmp, type, elements, size, facet, divide, base_radius, apex_radius, height, color); }

template<size_t size>
model frustum(const allocator& alloc, const allocator& tmp, const face_type& type, const element_t (&elements)[size]
	, const float4x4& projection, uint32_t color = 0) { return frustum(alloc, tmp, type, elements, size, projection, color); }

template<size_t size>
model sphere(const allocator& alloc, const allocator& tmp, const face_type& type, const element_t (&elements)[size]
	, float radius, uint32_t color = 0) { return sphere(alloc, tmp, type, elements, size, radius, color); }

template<size_t size>
model torus(const allocator& alloc, const allocator& tmp, const face_type& type, const element_t (&elements)[size]
	, const uint16_t facet, const uint16_t divide
	, float inner_radius, float outer_radius, uint32_t color = 0) { return torus(alloc, tmp, type, elements, size, facet, divide, inner_radius, outer_radius, color); }

}}
