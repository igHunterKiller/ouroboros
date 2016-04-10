// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#pragma once
#include <oBase/resource_registry2.h>
#include <oMemory/object_pool.h>
#include <oGPU/gpu.h>

namespace ouro { namespace gfx {

template<typename resourceT>
class device_resource_registry2_t : protected resource_registry<resourceT>
{
protected:
	// all these api are meant to implement a further type-wrapper

	typedef resource_registry<resourceT> base_t;
	typedef typename base_t::resource_type basic_resource_type;
	typedef typename base_t::size_type size_type;
	typedef typename base_t::handle handle;

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

	device_resource_registry2_t() : dev_(nullptr) {}
	~device_resource_registry2_t() {}

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

		initialize_base(registry_label, reg_memory, reg_bytes, error_placeholder, io_alloc);
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

private:
  device_resource_registry2_t(const device_resource_registry2_t&);
  const device_resource_registry2_t& operator=(const device_resource_registry2_t&);
};

}}
