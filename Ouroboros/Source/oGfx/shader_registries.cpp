// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// @tony: disabled after move to gpu.h
#if 0

#include <oGfx/shader_registries.h>
#include <oGfx/oGfxShaders.h>

namespace ouro { namespace gfx {

void layout_state::initialize(const char* name, gpu::device* dev)
{
	deinitialize();
	mstring n;
	for (int i = 0; i < (int)vertex_layout::count; i++)
	{
		vertex_layout input = vertex_layout(i);
		snprintf(n, "%s vertex_layout::%s", name, as_string(input));
		const void* bc = sig_bytecode(input);
		if (!bc)
			continue;
		layouts[i].initialize(n, dev, layout(input), bc);
	}
}

void layout_state::deinitialize()
{
	for (auto& layout : layouts)
		layout.deinitialize();
}

void vs_registry::initialize(gpu::device* dev, void* memory, size_type capacity, const allocator& io_alloc)
{
	base_type::initialize_base_with_builtins<vertex_shader>(dev, memory, capacity, io_alloc, bytecode
		, vertex_shader::pos
		, vertex_shader::pos
		, vertex_shader::pos);
}

void hs_registry::initialize(gpu::device* dev, void* memory, size_type capacity, const allocator& io_alloc)
{
	base_type::initialize_base_with_builtins<hull_shader>(dev, memory, capacity, io_alloc, bytecode
		, hull_shader::none
		, hull_shader::none
		, hull_shader::none);
}

void ds_registry::initialize(gpu::device* dev, void* memory, size_type capacity, const allocator& io_alloc)
{
	base_type::initialize_base_with_builtins<domain_shader>(dev, memory, capacity, io_alloc, bytecode
		, domain_shader::none
		, domain_shader::none
		, domain_shader::none);
}

void gs_registry::initialize(gpu::device* dev, void* memory, size_type capacity, const allocator& io_alloc)
{
	base_type::initialize_base_with_builtins<geometry_shader>(dev, memory, capacity, io_alloc, bytecode
		, geometry_shader::none
		, geometry_shader::none
		, geometry_shader::none);
}

void ps_registry::initialize(gpu::device* dev, void* memory, size_type capacity, const allocator& io_alloc)
{
	base_type::initialize_base_with_builtins<pixel_shader>(dev, memory, capacity, io_alloc, bytecode
		, pixel_shader::yellow
		, pixel_shader::gray
		, pixel_shader::red);
}

void cs_registry::initialize(gpu::device* dev, void* memory, size_type capacity, const allocator& io_alloc)
{
	base_type::initialize_base_with_builtins<compute_shader>(dev, memory, capacity, io_alloc, bytecode
		, compute_shader::none
		, compute_shader::none
		, compute_shader::none);
}

}}

#endif