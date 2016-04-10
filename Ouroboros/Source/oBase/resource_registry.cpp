// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/resource_registry.h>
#include <oCore/assert.h>

namespace ouro {

template<> const char* as_string(const enum class base_resource_registry::handle::status& s)
{
	static const char* s_names[] =
	{
		"missing",
		"loading",
		"indexed",
		"ready",
		"error",
	};
	return as_string(s, s_names);
}

uint64_t base_resource_registry::handle::pack(void* resource, const enum class status& status, uint16_t type, uint16_t refcnt)
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

uint64_t base_resource_registry::handle::repack(uint64_t previous_packed, void* resource, const enum class status& status)
{
	uint64_t ptr = previous_packed & ~(ptr_mask|status_mask); // clear all but type and ref count
	uint64_t sta = uint64_t(status);
	
	ptr |= uint64_t(resource);
  ptr |= (sta << status_shift) & status_mask;

	return ptr;
}

uint64_t base_resource_registry::handle::reference(uint64_t packed)
{
  oCheck(packed, std::errc::invalid_argument, "referencing nullptr");            // if there was a ref it'd be on a valid resource
  uint64_t n = (packed & refcnt_mask);                                           // isolate refcnt
  oCheck(n != refcnt_mask, std::errc::invalid_argument, "bad refcnt increment"); // check for overflow
  n += refcnt_one;                                                               // increment by one shifted to the correct position
  n |= (packed & ~refcnt_mask);                                                  // remerge the not-refcnt part
	return n;
}

uint64_t base_resource_registry::handle::release(uint64_t packed)
{
  oCheck(packed, std::errc::invalid_argument, "referencing nullptr");            // if 0, it means 0 ref count and a nullptr - not a valid condition
  uint64_t n = (packed & refcnt_mask);                                           // isolate refcnt
  n -= refcnt_one;                                                               // decrement by one shifted to the correct position
  n |= (packed & ~refcnt_mask);                                                  // remerge the not-refcnt part
	return n;
}

const base_resource_registry::handle& base_resource_registry::handle::operator=(const handle& that)
{
  release();
  handle_ = that.handle_;
  reference();
  return *this;
}

base_resource_registry::handle& base_resource_registry::handle::operator=(handle&& that)
{
  if (this != &that)
  {
    release();
    handle_ = that.handle_;
    that.handle_ = nullptr;
  }
    
  return *this;
}

void base_resource_registry::handle::reference()
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
  
void base_resource_registry::handle::release()
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

base_resource_registry::size_type base_resource_registry::calc_size(size_type capacity)
{
	allocate_options opts(required_alignment);
	const size_type lookup_bytes = opts.align(lookup_t::calc_size(capacity));
	const size_type res_bytes    = opts.align(res_pool_t::calc_size(capacity));
	const size_type queued_bytes = opts.align(queued_pool_t::calc_size(capacity));
  return lookup_bytes + res_bytes + queued_bytes;
}

base_resource_registry::size_type base_resource_registry::calc_capacity(size_type bytes)
{
	auto capacity_guess = bytes / calc_size(1);
	while (calc_size(capacity_guess) < bytes)
		capacity_guess++;
	return capacity_guess - 1;
}

base_resource_registry::base_resource_registry()
	: load_(nullptr)
	, load_user_(nullptr)
	, error_placeholder_(nullptr)
{
}

base_resource_registry::base_resource_registry(base_resource_registry&& that)
	: load_(nullptr)
	, load_user_(nullptr)
	, error_placeholder_(nullptr)
{
	lookup_            = std::move(that.lookup_);
	res_pool_          = std::move(that.res_pool_);
	queued_pool_       = std::move(that.queued_pool_);
	creates_           = std::move(that.creates_);
	destroys_          = std::move(that.destroys_);
	load_              = that.load_;              that.load_              = nullptr;
	load_user_         = that.load_user_;         that.load_user_         = nullptr;
	error_placeholder_ = that.error_placeholder_; that.error_placeholder_ = nullptr;
	label_             = std::move(that.label_);
}

base_resource_registry::~base_resource_registry()
{
	deinitialize_base();
}

base_resource_registry& base_resource_registry::operator=(base_resource_registry&& that)
{
	if (this != &that)
	{
		deinitialize_base();

		lookup_            = std::move(that.lookup_);
		res_pool_          = std::move(that.res_pool_);
		queued_pool_       = std::move(that.queued_pool_);
		creates_           = std::move(that.creates_);
		destroys_          = std::move(that.destroys_);
		load_              = that.load_;              that.load_              = nullptr;
		load_user_         = that.load_user_;         that.load_user_         = nullptr;
		error_placeholder_ = that.error_placeholder_; that.error_placeholder_ = nullptr;
		label_             = std::move(that.label_);
	}
	return *this;
}

void base_resource_registry::initialize_base(const char* registry_label, void* memory, size_type bytes, blob& error_placeholder, load_fn load, void* load_user, const allocator& io_alloc)
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

	load_              = load;
	load_user_         = load_user;
	error_placeholder_ = create("error_placeholder", error_placeholder);
	io_alloc_          = io_alloc;

	strlcpy(label_, registry_label);
}

void* base_resource_registry::deinitialize_base()
{
	void* p = nullptr;
	if (lookup_.valid())
	{
		destroy_indexed();
		destroy(error_placeholder_);
		error_placeholder_ = nullptr;
		
		flush();
		oCheck(res_pool_.full() && creates_.empty() && destroys_.empty(), std::errc::protocol_error, "[%s] flush should have emptied all queues and released all assets", label_.c_str());

		io_alloc_ = allocator();
		queued_pool_.deinitialize();
		res_pool_.deinitialize();
		p = lookup_.deinitialize();

		load_ = nullptr;
		load_user_ = nullptr;

		label_.clear();
	}

	return p;
}

void base_resource_registry::complete_load(const uri_t& uri_ref, blob& compiled, const char* error_message)
{
	if (compiled)
		queue_create(uri_ref, compiled);
	else
	{
		handle::type* h;
		insert_or_resolve(uri_ref.hash(), error_placeholder_, handle::status::error, h);
		oTrace("[%s] Load failed: %s\n  %s", label_.c_str(), uri_ref.c_str(), error_message ? error_message : "unknown error");
	}
}

bool base_resource_registry::insert_or_resolve(const key_type& key, void* placeholder, const enum class handle::status& status, handle::type*& out_handle)
{
	auto packed = handle::pack(placeholder ? placeholder : error_placeholder_, status, 0, 1);

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

		oCheck(index != res_pool_.nullidx, std::errc::invalid_argument, "something bad happened");

		return inserted;
	}

	// valid index, resolve it
	out_handle = res_pool_.typed_pointer(index);

	return false;
}

base_resource_registry::handle base_resource_registry::resolve(const key_type& key, void* placeholder)
{
	handle::type* h;
	insert_or_resolve(key, placeholder, handle::status::missing, h);
	return handle(h);
}

base_resource_registry::handle base_resource_registry::insert(const uri_t& uri_ref, void* placeholder, blob& compiled, bool force)
{
	handle::type* h;
	if (insert_or_resolve(uri_ref.hash(), placeholder, handle::status::loading, h) || force)
		queue_create(uri_ref, compiled);
	return handle(h);
}

void base_resource_registry::insert_indexed(const key_type& index, const char* label, blob& compiled)
{
	handle::type* h;
	insert_or_resolve(index, nullptr, handle::status::indexed, h);

	path_t path("indexed:");
	path /= label;

	queue_create(path, compiled, index);
}

void* base_resource_registry::resolve_indexed(const key_type& index) const 
{
	auto resolved = lookup_.get(index);
	if (resolved == lookup_.nul())
		throw std::invalid_argument("failed to resolve an indexed resource");
	handle::type packed = *res_pool_.typed_pointer(resolved);
	oCheck(handle::is_placeholder(packed), std::errc::invalid_argument, "non-placeholder asset being resolved by index");
	return handle::ptr(packed);
}

base_resource_registry::handle base_resource_registry::load(const uri_t& uri_ref, void* placeholder, bool force)
{
	handle::type* h;
	if (insert_or_resolve(uri_ref.hash(), placeholder, handle::status::loading, h) || force)
		load_(uri_ref, io_alloc_, load_user_);
	return handle(h);
}

void base_resource_registry::replace(const key_type& key, void* resource, const enum class handle::status& status)
{
	// note: status could be bool at this point if there was a way to uniquely map placeholders to a status

	auto index = lookup_.get(key);
	if (index == lookup_.nul())
		throw std::invalid_argument("invalid key");

	atm_resource_t* handle = (atm_resource_t*)res_pool_.typed_pointer(index);
	handle::type next, prev = handle->load();

	do
	{
		// if an entry's key was deleted in a sweep (0-refs) while loading, then this result isn't needed anymore
		if (prev == 0 && !handle::is_placeholder(status))
		{
			queue_destroy(resource);
			return;
		}

		next = handle::repack(prev, resource, status); // retain refcount (type, for now?)
		
	} while (!handle->compare_exchange_strong(prev, next));

	oTrace("[%s] replaced %p (%s) with %p (%s)", label_.c_str(), handle::ptr(prev), as_string(handle::status(prev)), handle::ptr(next), as_string(handle::status(next)));

	// free any previously valid data
	if (!handle::is_placeholder(prev))
		queue_destroy(handle::ptr(prev));
}

void base_resource_registry::queue_create(const uri_t& uri_ref, blob& compiled, const key_type& index)
{
	auto q = (queued_t*)queued_pool_.allocate();
	if (q)
	{
		new (q) queued_t(uri_ref, compiled, index);
		creates_.push(q);
	}
	else
		oTrace("queue overflow: ignoring %s", uri_ref.c_str());
}

void base_resource_registry::queue_destroy(void* resource)
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

void base_resource_registry::destroy_indexed()
{
	lookup_.visit([&](const lookup_t::key_type& key, const lookup_t::val_type& index)
	{
		auto packed = *res_pool_.typed_pointer(index);

		if (handle::status(packed) == handle::status::indexed)
		{
			lookup_.nix(key);
			res_pool_.deallocate(index);
			destroy(handle::ptr(packed));
		}
		return true;
	});
}

base_resource_registry::size_type base_resource_registry::flush(size_type max_operations)
{
	size_type n = max_operations;

	// sweep 0-refed entries in the hash map
	lookup_.visit([&](const lookup_t::key_type& key, const lookup_t::val_type& index)
	{
		// todo: make a missing placeholder separate from error
		uint64_t null_handle = handle::pack(error_placeholder_, handle::status::missing, 0, 0);

		// if 0-ref'ed replace entry with something that won't crash, but is invalid
		// this is an attempt to make this part of the flush still remain concurrent
		auto* handle = (atm_resource_t*)res_pool_.typed_pointer(index);
		handle::type prev = handle->load();
		do
		{
			if (handle::hasref(prev) || handle::is_placeholder(prev))
				return true;

		} while (!handle->compare_exchange_strong(prev, null_handle));

		lookup_.nix(key);
		res_pool_.deallocate(index);
		destroy(handle::ptr(prev));
		n--;
		return n != 0; // false (i.e. stop traversing) if the quota is used up)
	});

	queued_t* q = nullptr;

	// sweep separate create queue and insert new resources
	while (n && creates_.pop(&q))
	{
		// maybe move this above the full sweep? in case stuff is inserted with a 0 refcount?
		void* resource = create(q->uri_ref, q->compiled);
		auto key       = q->index ? q->index : q->uri_ref.hash();
		auto ready     = q->index ? handle::status::indexed : handle::status::ready;
		auto stat      = resource ? ready : handle::status::error;
		
		if (q->index)
			oTrace("[%s] created %p for index %u", label_.c_str(), resource, q->index);
		else
			oTrace("[%s] created %p for %s", label_.c_str(), resource, q->uri_ref.c_str());
		
		replace(key, resource, stat);
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

	return max_operations - n;
}

}
