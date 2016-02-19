// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// A stage of specification to resource_registry. This adds file I/O and
// gpu::device services to the base resource_registry_t type. This is
// still intended to be further specialzed by a specific resource type.
// There is a common deinitialize() that any derivative should be able
// to call. For initialize() there are several utility classes to help,
// but the specifics are left to another level of derivation, as are
// the virtual create/destroy methods.

// The io_allocator should be the allocator for blobs. For some 
// registries it may make sense to load the source, process it further
// put the results in a different heap, and free the temporary buffer.
// For others it may make sense to load the source and keep it 
// persistently. The io_allocator thus may be either a temp or a final
// allocator for the resources. blobs are passed around by reference
// so that move operators can be done to either take ownership of the
// contents or allow it to scope out.

#pragma once
#include <oBase/resource_registry.h>
#include <oCore/assert.h>
#include <oGPU/gpu.h>
#include <oMemory/object_pool.h>
#include <oSystem/filesystem.h>

namespace ouro { namespace gfx {

template<typename resourceT>
class device_resource_registry_t : public resource_registry_t
{
public:
	typedef resourceT basic_resource_type;
	typedef resource_t<basic_resource_type> resource_type;
	typedef resource_registry_t::size_type size_type;

	static const memory_alignment required_alignment = resource_registry_t::required_alignment;

	device_resource_registry_t() : dev_(nullptr) {}
	~device_resource_registry_t() { if (dev_) throw std::runtime_error("must call deinitialize in derived dtor or earlier"); }

	static size_type calc_size(size_type capacity)
	{
		allocate_options opts(required_alignment);
		auto pool_bytes = opts.align(object_pool<basic_resource_type>::calc_size(capacity));
		auto reg_bytes = opts.align(resource_registry_t::calc_size(capacity));
		return pool_bytes + reg_bytes;
	}

	static size_type calc_capacity(size_type bytes)
	{
		auto capacity_guess = bytes / calc_size(1);
		while (calc_size(capacity_guess) < bytes)
			capacity_guess++;
		return capacity_guess - 1;
	}

protected:
	void initialize_base(gpu::device* dev, const char* label, size_type bytes, const allocator& alloc
		, const allocator& io_alloc, resource_placeholders_t& placeholders)
	{
		allocate_options opts(required_alignment);

		void* arena = alloc.allocate(bytes, label, opts);

		if (!arena)
			throw allocate_error(allocate_errc::invalid_ptr);

		if (!opts.aligned(arena))
			throw allocate_error(allocate_errc::alignment);

		auto capacity = calc_capacity(bytes);

		auto pool_bytes = opts.align(pool_.calc_size(capacity));

		auto mem_pool = (uint8_t*)arena;
		auto mem_reg = mem_pool + pool_bytes;

		dev_ = dev;
		alloc_ = alloc ? alloc : default_allocator;
		io_alloc_ = io_alloc ? io_alloc : default_allocator;
	  pool_.initialize(mem_pool, capacity);
		initialize(mem_reg, bytes - pool_bytes, placeholders);
	}

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
	void initialize_base_with_builtins(gpu::device* dev, const char* label, size_type bytes
		, const allocator& alloc
		, const allocator& io_alloc
		, const void* (*get_builtin)(const builtin_enumT& which)
		, const builtin_enumT& missing
		, const builtin_enumT& loading
		, const builtin_enumT& failed)
	{
		resource_placeholders_t placeholders;
		placeholders.compiled_missing = blob((void*)get_builtin(missing), 1, noop_deallocate);
		placeholders.compiled_loading = blob((void*)get_builtin(loading), 1, noop_deallocate);
		placeholders.compiled_failed  = blob((void*)get_builtin(failed), 1, noop_deallocate);
		initialize_base(dev, label, bytes, alloc, io_alloc, placeholders);

		for (int i = 0; i < (int)builtin_enumT::count; i++)
		{
			auto sh = builtin_enumT(i);
			blob buffer((void*)get_builtin(sh), 1, noop_deallocate);
			insert(i, as_string(sh), buffer);
		}

		flush();
	}

	static void default_load_completion(const path_t& path, blob& buffer, const std::system_error* syserr, void* user)
	{
		if (syserr)
		{
			oTrace("Load failed: %s\n  %s", path.c_str(), syserr->what());
			return;
		}

		auto orig_path = path.relative_path(filesystem::data_path());
		auto reg = (resource_registry_t*)user;
		reg->insert(orig_path, buffer, true);
	}

	resource_base_t default_load(const path_t& path)
	{
		blob empty;
		auto res = insert(path, empty);

		filesystem::load_async(filesystem::data_path() / path
			, default_load_completion
			, this, filesystem::load_option::binary_read, io_alloc_);

		return res;
	}

	gpu::device* dev_;
	allocator alloc_;
	allocator io_alloc_;
	object_pool<basic_resource_type> pool_;

private:
  device_resource_registry_t(const device_resource_registry_t&);
  const device_resource_registry_t& operator=(const device_resource_registry_t&);
};

}}
