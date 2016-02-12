// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#pragma once
#include <oGPU/gpu.h>

namespace ouro { namespace gfx2 {

enum class input_layout : uint8_t
{
	none,
	lines,       // vbuffer0: float3 position, uint32_t color
	geometry,    // vbuffer0: float3 position, float3 normal, float4 tangent, float2 uv
	full_screen, // vbuffer0: float3 position, float2 uv, float4 misc
	
	count,
};

enum class rt_blend_state : uint8_t
{
	// Blend mode math from http://en.wikipedia.org/wiki/Blend_modes

	opaque,      // Output.rgba = Source.rgba
	alpha_test,  // Same as opaque, test is done in user code
	accumulate,  // Output.rgba = Source.rgba + Destination.rgba
	additive,    // Output.rgb = Source.rgb * Source.a + Destination.rgb
	multiply,    // Output.rgb = Source.rgb * Destination.rgb
	screen,      // Output.rgb = Source.rgb * (1 - Destination.rgb) + Destination.rgb (as reduced from webpage's 255 - [((255 - Src)*(255 - Dst))/255])
	translucent, // Output.rgb = Source.rgb * Source.a + Destination.rgb * (1 - Source.a)
	minimum,     // Output.rgba = min(Source.rgba, Destination.rgba)
	maximum,     // Output.rgba = max(Source.rgba, Destination.rgba)

	count,
};

enum class rasterizer_state : uint8_t
{
	// Front-facing is clockwise winding order. Back-facing is counter-clockwise.

	front_face,          // Draws all primitives whose vertices are specified in clockwise order
	back_face,           // Draws all primitives whose vertices are specified in counter-clockwise order
	two_sided,           // Draws all faces
	front_wireframe,     // Draws borders of faces whose vertices are specified in clockwise order
	back_wireframe,      // Draws borders of faces whose vertices are specified in counter-clockwise order
	two_sided_wireframe, // Draws the borders of all faces

	count,
};

enum class depth_stencil_state : uint8_t
{
	// No depth or stencil operation.
	none,

	// normal z-buffering mode where if occluded or same value (less_equal comparison), exit 
	// else render and write new Z value. No stencil operation.
	test_and_write,
	
	// test depth only using same method as test-and-write but do not write. No 
	// stencil operation.
	test,
	
	count,
};

const gpu::rt_blend_desc& get_rt_blend_state(const rt_blend_state& state);
const gpu::rasterizer_desc& get_rasterizer_desc(const rasterizer_state& state);
const gpu::depth_stencil_desc& get_depth_stencil_desc(const depth_stencil_state& state);
const mesh::element_t* get_input_layout_desc(const input_layout& layout, uint32_t* out_num_elements);

}}
