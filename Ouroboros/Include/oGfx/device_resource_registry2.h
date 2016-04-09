// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#pragma once
#include <oBase/resource_registry2.h>
#include <oMemory/object_pool.h>
#include <oGPU/gpu.h>

namespace ouro { namespace gfx {

template<typename resourceT>
class device_resource_registry2_t : protected resource_registry2_t
{
protected:
	// all these api are meant to implement a further type-wrapper

	typedef resourceT basic_resource_type;
	typedef typed_resource<basic_resource_type> resource_type;
	typedef resource_registry2_t::size_type size_type;

	static const memory_alignment required_alignment = resource_registry2_t::required_alignment;

	static size_type calc_size(size_type capacity)
	{
		allocate_options opts(required_alignment);
		auto reg_bytes = resource_registry2_t::calc_size(capacity);
		auto pool_bytes = opts.align(object_pool<basic_resource_type>::calc_size(capacity));
		return reg_bytes + pool_bytes;
	}

	static size_type calc_capacity(size_type bytes)
	{
		auto capacity_guess = bytes / calc_size(1);
		while (calc_size(capacity_guess) < bytes)
			capacity_guess++;
		return capacity_guess - 1;
	}

	// == non-concurrent api ==

	device_resource_registry2_t() : dev_(nullptr), error_resource_(nullptr) {}
	~device_resource_registry2_t() {}

	void initialize(void* memory, size_type bytes, gpu::device* dev, blob& error_placeholder, const allocator& io_alloc)
	{
		allocate_options opts(required_alignment);
		
		if (!memory)
			throw allocate_error(allocate_errc::invalid_ptr);

		if (!opts.aligned(memory))
			throw allocate_error(allocate_errc::alignment);

		auto capacity = calc_capacity(bytes);

		auto pool_bytes = opts.align(pool_.calc_size(capacity));

		pool_.initialize(memory, pool_bytes);

		auto reg_memory = (uint8_t*)memory + pool_bytes;
		auto reg_bytes  = bytes - pool_bytes;

		dev_ = dev;

		error_resource_ = create("error_placeholder", error_placeholder);

		resource_registry2_t::initialize(reg_memory, reg_bytes, error_resource_, io_alloc);
	}

	void* deinitialize()
	{
		void* p = nullptr;
		if (dev_)
		{
			destroy_indexed();
			destroy(error_resource_);

			flush();
			oCheck(pool_.full(), std::errc::invalid_argument, "resource registry should be empty, all handles destroyed, but there are still %u entries", pool_.size());

			resource_registry2_t::deinitialize();
			void* arena = pool_.deinitialize();
			dev_ = nullptr;
		}
		return p;
	}


	// == concurrent api ==

	resource_type        resolve        (const key_type& key,   basic_resource_type* placeholder)                             { return (resource_type)       resource_registry2_t::resolve(key, placeholder);                  }
	resource_type        resolve        (const path_t&   path,  basic_resource_type* placeholder)                             { return (resource_type)       resource_registry2_t::resolve(path.hash(), placeholder);          }
	resource_type        insert         (const path_t&   path,  basic_resource_type* placeholder, blob& compiled, bool force) { return (resource_type)       resource_registry2_t::insert(path, placeholder, compiled, force); }
	resource_type        load           (const path_t&   path,  basic_resource_type* placeholder,                 bool force) { return (resource_type)       resource_registry2_t::load(path, placeholder, force);             }
	void                 insert_indexed (const key_type& index, const char* label,                blob& compiled)             {                              resource_registry2_t::insert_indexed(index, label, compiled);     }
	basic_resource_type* resolve_indexed(const key_type& index) const                                                         { return (basic_resource_type*)resource_registry2_t::resolve_indexed(index);                     }

protected:
	gpu::device* dev_;
	void* error_resource_;
	object_pool<basic_resource_type> pool_;

private:
  device_resource_registry2_t(const device_resource_registry2_t&);
  const device_resource_registry2_t& operator=(const device_resource_registry2_t&);

#if 0

protected:

	void deinitialize_base()
	{
		if (dev_)
		{
			resource_registry_t::remove_all();
			resource_registry_t::deinitialize();
			void* arena = pool_.deinitialize();
			alloc_.deallocate(arena);
			dev_ = nullptr;
		}
	}

	template<typename builtin_enumT>
	void create_and_insert_builtins(const void* (*get_builtin)(const builtin_enumT& which))
	{
		for (int i = 0; i < (int)builtin_enumT::count; i++)
		{
			auto b = builtin_enumT(i);
			blob buffer((void*)get_builtin(b), 1, noop_deallocate);
			insert(i, as_string(b), buffer);
		}

		flush();
	}

#endif
};

}}
