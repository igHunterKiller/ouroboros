// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/resource_registry2.h>
#include <oCore/assert.h>
#include <oSystem/filesystem.h>

namespace ouro {

uint64_t base_resource::pack(void* resource, status status, uint16_t type, uint16_t refcnt)
{
	uint64_t ref = uint64_t(refcnt);
	uint64_t sta = uint64_t(status);
	uint64_t ptr = uint64_t(resource); // seed with actual data

  // check that parameters fit in masks
	oCheck((ref & refcnt_max) == ref, std::errc::invalid_argument, "invalid refcount (12-bit max)");
	oCheck((ptr & ptr_mask  ) == ptr, std::errc::invalid_argument, "invalid pointer (48-bit, min 16-byte aligned)");
	oCheck((sta & status_max) == sta, std::errc::invalid_argument, "invalid status (4-bit max)");

  // merge metadata
  ptr |= (ref << refcnt_shift) & refcnt_mask;
  ptr |= (sta << status_shift) & status_mask;

	return ptr;
}

uint64_t base_resource::repack(uint64_t previous_packed, void* resource, status status)
{
	uint64_t ptr = previous_packed & ~(ptr_mask|status_mask); // clear all but type and ref count
	uint64_t sta = uint64_t(status);
	
	ptr |= uint64_t(resource);
  ptr |= (sta << status_shift) & status_mask;

	return ptr;
}

uint64_t base_resource::reference(uint64_t packed)
{
  oCheck(packed, std::errc::invalid_argument, "referencing nullptr");            // if there was a ref it'd be on a valid resource
  uint64_t n = (packed & refcnt_mask);                                           // isolate refcnt
  oCheck(n != refcnt_mask, std::errc::invalid_argument, "bad refcnt increment"); // check for overflow
  n += refcnt_one;                                                               // increment by one shifted to the correct position
  n |= (packed & ~refcnt_mask);                                                  // remerge the not-refcnt part
	return n;
}

uint64_t base_resource::release(uint64_t packed)
{
  oCheck(packed, std::errc::invalid_argument, "referencing nullptr");            // if 0, it means 0 ref count and a nullptr - not a valid condition
  uint64_t n = (packed & refcnt_mask);                                           // isolate refcnt
  n -= refcnt_one;                                                               // decrement by one shifted to the correct position
  n |= (packed & ~refcnt_mask);                                                  // remerge the not-refcnt part
	return n;
}

const base_resource& base_resource::operator=(const base_resource& that)
{
  that.reference();
  release();
  handle_ = that.handle_;
  return *this;
}

base_resource& base_resource::operator=(base_resource&& that)
{
  if (this != &that)
  {
    release();
    handle_ = that.handle_;
    that.handle_ = nullptr;
  }
    
  return *this;
}

void base_resource::reference() const
{
  if (!handle_)
    return;

  uint64_t o, n;
  do
  {
    o = handle_->load();
		n = reference(o);

  } while(!handle_->compare_exchange_strong(o, n));
}
  
void base_resource::release() const
{
  if (!handle_)
    return;

  uint64_t o, n;
  do
  {
    o = handle_->load();
		n = release(o);

  } while(!handle_->compare_exchange_strong(o, n));

	handle_ = nullptr;
}

resource_registry2_t::size_type resource_registry2_t::calc_size(size_type capacity)
{
	allocate_options opts(required_alignment);
	const size_type lookup_bytes = opts.align(lookup_t::calc_size(capacity));
	const size_type res_bytes    = opts.align(res_pool_t::calc_size(capacity));
	const size_type queued_bytes = opts.align(queued_pool_t::calc_size(capacity));
  return lookup_bytes + res_bytes + queued_bytes;
}

resource_registry2_t::size_type resource_registry2_t::calc_capacity(size_type bytes)
{
	auto capacity_guess = bytes / calc_size(1);
	while (calc_size(capacity_guess) < bytes)
		capacity_guess++;
	return capacity_guess - 1;
}

resource_registry2_t::resource_registry2_t()
	: error_placeholder_(nullptr)
{
}

resource_registry2_t::resource_registry2_t(resource_registry2_t&& that)
	: error_placeholder_(nullptr)
{
	lookup_            = std::move(that.lookup_);
	res_pool_          = std::move(that.res_pool_);
	queued_pool_        = std::move(that.queued_pool_);
	creates_           = std::move(that.creates_);
	destroys_          = std::move(that.destroys_);
	error_placeholder_ = that.error_placeholder_; that.error_placeholder_ = nullptr;
}

resource_registry2_t::resource_registry2_t(void* memory, size_type bytes, blob& error_placeholder, const allocator& io_alloc)
	: error_placeholder_(nullptr)
{
	initialize(memory, bytes, error_placeholder, io_alloc);
}

resource_registry2_t::~resource_registry2_t()
{
	deinitialize();
}

resource_registry2_t& resource_registry2_t::operator=(resource_registry2_t&& that)
{
	if (this != &that)
	{
		deinitialize();

		lookup_            = std::move(that.lookup_);
		res_pool_          = std::move(that.res_pool_);
		queued_pool_       = std::move(that.queued_pool_);
		creates_           = std::move(that.creates_);
		destroys_          = std::move(that.destroys_);
		error_placeholder_ = that.error_placeholder_; that.error_placeholder_ = nullptr;
	}
	return *this;
}

void resource_registry2_t::initialize(void* memory, size_type bytes, void* error_placeholder, const allocator& io_alloc)
{
	allocate_options opts(required_alignment);

  if (!memory)
    throw allocate_error(allocate_errc::invalid_ptr);

	if (!opts.aligned(memory))
    throw allocate_error(allocate_errc::alignment);

	if (!error_placeholder)
		throw std::invalid_argument("error placeholder create failed");

	auto capacity = calc_capacity(bytes);

	const size_type lookup_bytes = opts.align(lookup_t::calc_size(capacity));
	const size_type res_bytes    = opts.align(res_pool_t::calc_size(capacity));
	const size_type queued_bytes = opts.align(queued_pool_t::calc_size(capacity));

	auto lookup_mem              = (uint8_t*)memory;
	auto res_mem                 = lookup_mem + lookup_bytes;
	auto queued_mem              = res_mem    + res_bytes;
	
	lookup_     .initialize(res_pool_.nullidx, memory, capacity);
	res_pool_   .initialize(res_mem, res_bytes);
	queued_pool_.initialize(queued_mem, queued_bytes);

	error_placeholder_ = error_placeholder;
	io_alloc_          = io_alloc;
}

void* resource_registry2_t::deinitialize()
{
	void* p = nullptr;
	if (lookup_.valid())
	{
		if (!creates_.empty() || !destroys_.empty())
			throw std::exception("resource_registry2_t should have been flushed before destruction");

		io_alloc_ = allocator();
		error_placeholder_ = nullptr;

		queued_pool_.deinitialize();
		res_pool_.deinitialize();
		p = lookup_.deinitialize();
	}

	return p;
}

bool resource_registry2_t::insert_or_resolve(const key_type& key, void* placeholder, const base_resource::status& status, base_resource::handle_type*& out_handle)
{
	auto packed = base_resource::pack(placeholder, status, 0, 1);

	// do a quick check to see if there's already a value
	auto index = lookup_.get(key);
	if (index == lookup_.nul())
	{
		// create a new entry
		index = res_pool_.allocate_index();
		if (index == res_pool_.nullidx)
			throw std::invalid_argument("out of slots");

		// initialize the entry
		out_handle  = res_pool_.typed_pointer(index);
		*out_handle = packed;

		// activate new entry
		auto prev = lookup_.nul();

		bool inserted = lookup_.cas(key, prev, index);
		if (!inserted)
		{
			out_handle = res_pool_.typed_pointer(prev); // another thread did an insert before this one, so resolve to its value
			res_pool_.deallocate(index);							  // and roll back this thread's effort
		}

		return inserted;
	}

	// valid index, resolve it
	out_handle = res_pool_.typed_pointer(index);

	return false;
}

base_resource resource_registry2_t::resolve(const key_type& key, void* placeholder)
{
	base_resource::handle_type* handle;
	insert_or_resolve(key, placeholder, base_resource::status::missing, handle);
	return base_resource(handle);
}

void resource_registry2_t::load_completion(const path_t& path, blob& buffer, const std::system_error* syserr, void* user)
{
	auto reg = (resource_registry2_t*)user;
	reg->load_completion(path, buffer, syserr);
}

void resource_registry2_t::load_completion(const path_t& path, blob& buffer, const std::system_error* syserr)
{
	auto relative_path = path.relative_path(filesystem::data_path());
	auto key           = relative_path.hash();

	if (!syserr)
		queue_create(relative_path, buffer);
	else
	{
		oTrace("Load failed: %s\n  %s", path.c_str(), syserr->what());
		base_resource::handle_type* handle;
		insert_or_resolve(key, error_placeholder_, base_resource::status::error, handle);
	}
}

base_resource resource_registry2_t::insert(const path_t& path, blob& compiled, void* placeholder, bool force)
{
	oCheck(!path.is_windows_absolute(), std::errc::invalid_argument, "path should be relative to data path (%s)", path.c_str());
	base_resource::handle_type* handle;
	auto key = path.hash();
	if (insert_or_resolve(key, placeholder, base_resource::status::loading, handle) || force)
		queue_create(path, compiled);
	return base_resource(handle);
}

base_resource resource_registry2_t::load(const path_t& path, void* placeholder, bool force)
{
	oCheck(!path.is_windows_absolute(), std::errc::invalid_argument, "path should be relative to data path (%s)", path.c_str());
	base_resource::handle_type* handle;
	auto key = path.hash();
	if (insert_or_resolve(key, placeholder, base_resource::status::loading, handle) || force)
		filesystem::load_async(filesystem::data_path() / path, load_completion, this, filesystem::load_option::binary_read, io_alloc_);
	return base_resource(handle);
}

void resource_registry2_t::replace(const key_type& key, void* resource, const base_resource::status& status)
{
	// note: status could be bool at this point if there was a way to uniquely map placeholders to a status

	auto index = lookup_.get(key);
	if (index == lookup_.nul())
		throw std::invalid_argument("invalid key");

	atm_resource_t* handle = (atm_resource_t*)res_pool_.typed_pointer(index);
	base_resource::handle_type next, prev = handle->load();

	do
	{
		// if an entry's key was deleted in a sweep (0-refs) while loading, then this result isn't needed anymore
		if (prev == 0 && !base_resource::is_placeholder(status))
		{
			queue_destroy(resource);
			return;
		}

		next = base_resource::repack(prev, resource, status); // retain refcount (type, for now?)
		
	} while (!handle->compare_exchange_strong(prev, next));

	oTrace("[resource_registry2] replaced %p with %p", base_resource::ptr(prev), base_resource::ptr(next));

	// free any previously valid data
	if (!base_resource::is_placeholder(prev))
		queue_destroy(base_resource::ptr(prev));
}

void resource_registry2_t::queue_create(const path_t& path, blob& compiled)
{
	auto q = (queued_t*)queued_pool_.allocate();
	if (q)
	{
		new (q) queued_t(path, compiled);
		creates_.push(q);
	}
	else
		oTrace("queue overflow: ignoring %s", path.c_str());
}

void resource_registry2_t::queue_destroy(void* resource)
{
	auto q = (queued_t*)queued_pool_.allocate();
	if (q)
	{
		new (q) queued_t(resource);
		destroys_.push(q);
	}
	else
		oTrace("queue overflow: leaking resource %p", resource);
}

resource_registry2_t::size_type resource_registry2_t::flush(size_type max_operations)
{
	size_type n = max_operations;

	// sweep 0-refed entries in the hash map
	lookup_.visit([&](const lookup_t::key_type& key, const lookup_t::val_type& index)
	{
		// todo: make a missing placeholder separate from error
		uint64_t null_handle = base_resource::pack(error_placeholder_, base_resource::status::missing, 0, 0);

		// if 0-ref'ed replace entry with something that won't crash, but is invalid
		// this is an attempt to make this part of the flush still remain concurrent
		auto* handle = (atm_resource_t*)res_pool_.typed_pointer(index);
		base_resource::handle_type prev = handle->load();
		do
		{
			if (base_resource::hasref(prev) || base_resource::is_placeholder(prev))
				return true;

		} while (!handle->compare_exchange_strong(prev, null_handle));

		lookup_.nix(key);
		res_pool_.deallocate(index);
		destroy(base_resource::ptr(prev));
		n--;
		return n != 0; // false (i.e. stop traversing) if the quota is used up)
	});

	queued_t* q = nullptr;

	// sweep separate create queue and insert new resources
	while (n && creates_.pop(&q))
	{
		// maybe move this above the full sweep? in case stuff is inserted with a 0 refcount?
		void* resource = create(q->path, q->compiled);
		auto stat = resource ? base_resource::status::ready : base_resource::status::error;
		replace(q->path.hash(), resource, stat);
		queued_pool_.deallocate(q);
		n--;
	}

	// sweep separate destroy queue to free any out-of-band entries
	// this is done after create in case some creates are noop'ed
	while (n && destroys_.pop(&q))
	{
		destroy(q->resource);
		queued_pool_.deallocate(q);
		n--;
	}

	// reclaim keys, nothing about this is concurrent
	// if there are any modifications during this period, this all breaks down
	// so although there is optimism in the above operations, overall flush is
	// non-concurrent because of this.
	lookup_.reclaim_keys();

	return n;
}

}
