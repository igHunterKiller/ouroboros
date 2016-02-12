// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// @tony: disabled after move to gpu.h
#if 0

#pragma once
#include <oGfx/basic_registry.h>
#include <oGfx/bytecode.h>

namespace ouro { namespace gfx {

class layout_state
{
public:
	layout_state() {}
	~layout_state() { deinitialize(); }

	void initialize(const char* name, gpu::device* dev);
	void deinitialize();

	inline void set(gpu::command_list* cl, const vertex_layout& input, const mesh::primitive_type& prim_type) const { layouts[(size_t)input].set(cl, prim_type); }

private:
	std::array<gpu::vertex_layout, (size_t)gfx::vertex_layout::count> layouts;
};

template<typename shaderT>
class shader_registry : public basic_registry<shaderT>
{
public:
	typedef basic_registry<shaderT> base_type;
	typedef typename base_type::basic_resource_type shader_type;
	typedef base_type::size_type size_type;

	static const gpu::stage stage = basic_resource_type::stage;

	void compile(const lstring& include_paths, const lstring& defines, const path_t& source_path, const char* shader_source, const char* entry_point)
	{
		try
		{
			blob bytecode = gpu::compile_shader(include_paths, defines, source_path, stage, entry_point, shader_source);
			oTRACEA("[%s registry] insert \"%s\" from %s", as_string(stage), entry_point, source_path.c_str());
			insert(entry_point, bytecode, true);
		}
			
		catch (std::exception& e)
		{
			oTRACEA("[%s registry] insert \"%s\" as error", as_string(stage), entry_point, e.what());
			blob empty;
			insert(entry_point, empty, true);
		}
	}

protected:
	void* create(const char* name, blob& compiled) override { return pool_.create(name, dev_, (const void*)compiled); }
	void destroy(void* entry) override { pool_.destroy((shader_type*)entry); }
};

class vs_registry : public shader_registry<gpu::vertex_shader>
{
	typedef shader_registry<gpu::vertex_shader> base_type;

public:
	typedef base_type::shader_type shader_type;
	static const gpu::stage stage = base_type::stage;
	
	void initialize(gpu::device* dev, void* memory, size_type capacity, const allocator& io_alloc = default_allocator);

	inline void set(gpu::command_list* cl, const vertex_shader& shader) const
	{
		resource_type s = by_index(shader);
		s->set(cl);
	}
};

class hs_registry : public shader_registry<gpu::hull_shader>
{
	typedef shader_registry<gpu::hull_shader> base_type;

public:
	typedef base_type::shader_type shader_type;
	static const gpu::stage stage = base_type::stage;
	
	void initialize(gpu::device* dev, void* memory, size_type capacity, const allocator& io_alloc = default_allocator);

	inline void set(gpu::command_list* cl, const hull_shader& shader) const
	{
		resource_type s = by_index(shader);
		s->set(cl);
	}
};

class ds_registry : public shader_registry<gpu::domain_shader>
{
	typedef shader_registry<gpu::domain_shader> base_type;

public:
	typedef base_type::shader_type shader_type;
	static const gpu::stage stage = base_type::stage;
	
	void initialize(gpu::device* dev, void* memory, size_type capacity, const allocator& io_alloc = default_allocator);

	inline void set(gpu::command_list* cl, const domain_shader& shader) const
	{
		resource_type s = by_index(shader);
		s->set(cl);
	}
};

class gs_registry : public shader_registry<gpu::geometry_shader>
{
	typedef shader_registry<gpu::geometry_shader> base_type;

public:
	typedef base_type::shader_type shader_type;
	static const gpu::stage stage = base_type::stage;
	
	void initialize(gpu::device* dev, void* memory, size_type capacity, const allocator& io_alloc = default_allocator);

	inline void set(gpu::command_list* cl, const geometry_shader& shader) const
	{
		resource_type s = by_index(shader);
		s->set(cl);
	}
};

class ps_registry : public shader_registry<gpu::pixel_shader>
{
	typedef shader_registry<gpu::pixel_shader> base_type;

public:
	typedef base_type::shader_type shader_type;
	static const gpu::stage stage = base_type::stage;
	
	void initialize(gpu::device* dev, void* memory, size_type capacity, const allocator& io_alloc = default_allocator);

	inline void set(gpu::command_list* cl, const pixel_shader& shader) const
	{
		resource_type s = by_index(shader);
		s->set(cl);
	}
};

class cs_registry : public shader_registry<gpu::compute_shader>
{
	typedef shader_registry<gpu::compute_shader> base_type;

public:
	typedef base_type::shader_type shader_type;
	static const gpu::stage stage = base_type::stage;
	
	void initialize(gpu::device* dev, void* memory, size_type capacity, const allocator& io_alloc = default_allocator);

	inline void dispatch(gpu::command_list* cl, const compute_shader& shader, const uint3& dispatch_thread_count) const
	{
		resource_type s = by_index(shader);
		s->dispatch(cl, dispatch_thread_count);
	}

	inline void dispatch(gpu::command_list* cl, const compute_shader& shader, gpu::raw_buffer* dispatch_thread_counts, uint offset_in_uints) const
	{
		resource_type s = by_index(shader);
		s->dispatch(cl, dispatch_thread_counts, offset_in_uints);
	}
};

}}

#endif