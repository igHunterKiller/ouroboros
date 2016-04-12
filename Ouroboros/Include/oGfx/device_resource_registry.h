// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Implements file I/O for a resource registry and adds some memory management 
// for oGPU resources.

#pragma once
#include <oBase/resource_registry.h>
#include <oMemory/object_pool.h>
#include <oGPU/gpu.h>
#include <oSystem/filesystem.h>

namespace ouro { namespace gfx {

template<typename resourceT>
class device_resource_registry : public resource_registry<resourceT>
{
protected:
	// all these api are meant to implement a further type-wrapper

	typedef device_resource_registry<resourceT> self_t;
	typedef resource_registry<resourceT>        base_t;
	typedef typename base_t::resource_type      basic_resource_type;
	typedef typename base_t::size_type          size_type;
	typedef typename base_t::handle             handle;

	static const memory_alignment required_alignment = base_t::required_alignment;

	static size_type calc_size(size_type capacity)
	{
		allocate_options opts(required_alignment);
		auto reg_bytes = base_t::calc_size(capacity);
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

	device_resource_registry() : dev_(nullptr) {}
	~device_resource_registry() {}
  device_resource_registry(const device_resource_registry&) = delete;
  const device_resource_registry& operator=(const device_resource_registry&) = delete;

	void initialize(const char* registry_label, void* memory, size_type bytes, gpu::device* dev, blob& error_placeholder, const allocator& io_alloc)
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

		initialize_base(registry_label, reg_memory, reg_bytes, error_placeholder, io_alloc, true);
	}

	void* deinitialize()
	{
		void* p = nullptr;
		if (dev_)
		{
			deinitialize_base();
			p = pool_.deinitialize();
			dev_ = nullptr;
		}
		return p;
	}

protected:
	gpu::device* dev_;
	object_pool<basic_resource_type> pool_;

	static void on_completion(const path_t& path, blob& buffer, const std::system_error* syserr, void* user)
	{
		auto reg = (device_resource_registry*)user;
		path_t relative_path = path.relative_path(filesystem::data_path());
		reg->complete_load_resource(uri_t(relative_path.c_str()), buffer, syserr ? syserr->what() : "no error");
	}

	void load_resource(const uri_t& uri_ref, allocator& io_alloc) override
	{
		path_t path = uri_ref.path();
		oCheck(!path.is_windows_absolute(), std::errc::invalid_argument, "path should be relative to data path (%s)", path.c_str());
		filesystem::load_async(filesystem::data_path() / path, on_completion, this, filesystem::load_option::binary_read, io_alloc);
	}
};

}}
