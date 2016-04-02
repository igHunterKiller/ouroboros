// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/resource_registry.h>
#include <oCore/assert.h>

namespace ouro {

resource_registry_t::size_type resource_registry_t::calc_size(size_type capacity)
{
	allocate_options opts(required_alignment);
	const size_type pool_req = opts.align(concurrent_object_pool<file_info>::calc_size(capacity));
	const size_type lookup_req = opts.align(hash_map_t::calc_size(capacity));
	const size_type entries_req = opts.align(capacity * (uint32_t)sizeof(void*));
  return pool_req + lookup_req + entries_req;
}

resource_registry_t::size_type resource_registry_t::calc_capacity(size_type bytes)
{
	auto capacity_guess = bytes / calc_size(1);
	while (calc_size(capacity_guess) < bytes)
		capacity_guess++;
	return capacity_guess - 1;
}

resource_registry_t::resource_registry_t()
	: entries_(nullptr)
	, missing_(nullptr)
	, failed_(nullptr)
	, loading_(nullptr)
{}

resource_registry_t::resource_registry_t(resource_registry_t&& that)
	: entries_(that.entries_)
	, missing_(that.missing_)
	, failed_(that.failed_)
	, loading_(that.loading_)
{
	that.entries_ = nullptr;
	that.missing_ = nullptr;
	that.failed_ = nullptr;
	that.loading_ = nullptr;
	pool_ = std::move(that.pool_);
	lookup_ = std::move(that.lookup_);
	inserts_ = std::move(that.inserts_);
	removes_ = std::move(that.removes_);
}

resource_registry_t::resource_registry_t(void* memory, size_type bytes, resource_placeholders_t& placeholders)
{
	initialize(memory, bytes, placeholders);
}

resource_registry_t::~resource_registry_t()
{
	deinitialize();
}

resource_registry_t& resource_registry_t::operator=(resource_registry_t&& that)
{
	if (this != &that)
	{
		pool_ = std::move(that.pool_);
		lookup_ = std::move(that.lookup_);
		inserts_ = std::move(that.inserts_);
		removes_ = std::move(that.removes_);
		entries_ = that.entries_; that.entries_ = nullptr;
		missing_ = that.missing_; that.missing_ = nullptr;
		failed_ = that.failed_; that.failed_ = nullptr;
		loading_ = that.loading_; that.loading_ = nullptr;
	}
	return *this;
}

void resource_registry_t::initialize(void* memory, size_type bytes, resource_placeholders_t& placeholders)
{
	allocate_options opts(required_alignment);

  if (!memory)
    throw allocate_error(allocate_errc::invalid_ptr);

	if (!opts.aligned(memory))
    throw allocate_error(allocate_errc::alignment);

	auto capacity = calc_capacity(bytes);

	auto pool_bytes = opts.align(concurrent_object_pool<file_info>::calc_size(capacity));
	auto lookup_bytes = opts.align(hash_map_t::calc_size(capacity));

	pool_.initialize(memory, pool_bytes);

	auto p = (uint8_t*)memory + pool_bytes;
	lookup_.initialize(nullidx, p, capacity);
	p += lookup_bytes;
	entries_ = (void**)p;

	missing_ = create("missing_placeholder", placeholders.compiled_missing);
	loading_ = create("loading_placeholder", placeholders.compiled_loading);
	failed_ = create("failed_placeholder", placeholders.compiled_failed);
		
	for (index_type i = 0; i < capacity; i++)
		entries_[i] = missing_;
}
	
void* resource_registry_t::deinitialize()
{
	void* p = nullptr;
	if (entries_)
	{
		if (!inserts_.empty() || !removes_.empty())
			throw std::exception("resource_registry_t should have been flushed before destruction");

		if (missing_) { destroy(missing_); missing_ = nullptr; }
		if (failed_)	{ destroy(failed_); failed_ = nullptr; }
		if (loading_)	{ destroy(loading_); loading_ = nullptr; }

		memset(entries_, 0, sizeof(void*) * pool_.capacity());
		entries_ = nullptr;
		lookup_.deinitialize();

		p = pool_.deinitialize();
	}

	return p;
}

void resource_registry_t::remove_all()
{
	size_type remaining = flush();
	if (remaining)
		throw std::exception("did not flush all outstanding items");

	const size_type n = capacity();
	for (size_type i = 0; i < n; i++)
	{
		void** e = entries_ + i;
		if (*e && *e != missing_ && *e != failed_ && *e != loading_)
		{
			destroy(*e);
			auto f = pool_.typed_pointer(i);
			lookup_.set(f->key, nullidx);
			*e = f->status == resource_status::deinitializing ? missing_ : failed_;
			pool_.destroy(f);
		}
	}
}

resource_registry_t::size_type resource_registry_t::flush(size_type max_operations)
{
	size_type n = max_operations;
	file_info* f = nullptr;
	while (n && removes_.pop(&f))
	{
		resource_status status = f->status;
		if ((int)status < (int)resource_status::deinitializing)
			continue;
		index_type index = pool_.index(f);
		void** e = entries_ + index;
		destroy(*e);
		int placeholder_index = (int)status - (int)resource_status::deinitializing;
		*e = (&missing_)[placeholder_index];
		if (placeholder_index) // only free the entry if removing, leave entry if discarding
		{
			lookup_.set(f->path.hash(), nullidx);
			pool_.destroy(f);
		}
		else
			f->status.exchange(placeholder_index == 1 ? resource_status::initializing : resource_status::failed);

		n--;
	}

	size_type EstHashesToReclaim = max_operations - n;

	while (n && inserts_.pop(&f))
	{
		index_type index = pool_.index(f);

		void** e = entries_ + index;
		
		// free any prior asset (should this count as an operation n--?)
		if (*e && *e != missing_ && *e != failed_ && *e != loading_)
			destroy(*e);

		if (!f->compiled)
		{
			*e = failed_;
			f->status.store(resource_status::failed);
		}

		else
		{
			try
			{
				*e = create(f->path, f->compiled);
				f->status.store(resource_status::initialized);
			}
		
			catch (std::exception& ex)
			{
				ex;
				*e = failed_;
				f->status.store(resource_status::failed);
				oTraceA("failed: %s (%s)", f->path.c_str(), ex.what());
			}
			n--;
		}
	}

	if (EstHashesToReclaim && EstHashesToReclaim <= n)
		lookup_.reclaim_keys();

	return inserts_.size() + removes_.size();
}

resource_base_t resource_registry_t::get(key_type key) const
{
	auto index = lookup_.get(key);
	return index == nullidx ? &missing_ : entries_ + index;
}

path_t resource_registry_t::path(key_type key) const
{
	auto index = lookup_.get(key);
	if (index == nullidx)
		return path_t();
	auto f = pool_.typed_pointer(index);
	return f->path;
}

path_t resource_registry_t::path(const resource_base_t& resource) const
{
	void* entry = (void*)resource;
	if (!in_range(entry, entries_, pool_.capacity()))
		return path_t();
	index_type index = (index_type)index_of(entry, entries_, sizeof(void*));
	auto f = pool_.typed_pointer(index);
	return f->path;
}

resource_status resource_registry_t::status(key_type key) const
{
	auto index = lookup_.get(key);
	if (index == nullidx)
		return resource_status::invalid;
	auto f = pool_.typed_pointer(index);
	return f->status;
}

resource_status resource_registry_t::status(const resource_base_t& resource) const
{
	const void* entry = (const void*)resource;
	if (!in_range(entry, entries_, (uint8_t*)entry + pool_.capacity()))
		return resource_status::invalid;
	auto index = (index_type)index_of(entry, entries_, sizeof(void*));
	auto f = pool_.typed_pointer(index);
	return f->status;
}

resource_base_t resource_registry_t::insert(key_type key, const path_t& path, blob& compiled, bool force)
{
	//                                       Truth Table
	//                            n/c                n/c/f              n                n/f
	// <no entry>          ins/q/initializing  ins/q/initializing    ins/err           ins/err
	// failed                q/initializing      q/initializing       noop              noop
	// initializing               noop                noop            noop              noop
	// initialized                noop           q/initializing       noop         q/deinit_to_err
	// deinitializing          initialized       q/initializing  q/deinit_to_err   q/deinit_to_err
	// deinitializing_to_err  q/initializing     q/initializing       noop              noop

	const bool has_compiled = !!compiled;
	void** e = nullptr;
	index_type index = lookup_.get(key);
	file_info* f = nullptr;

	if (index != nullidx)
	{
		e = entries_ + index;
		f = pool_.typed_pointer(index);
		resource_status old = f->status;
		
		// handle noop cases
		if (old == resource_status::initializing || (old == resource_status::initialized && !force) 
			|| ((old == resource_status::failed || old == resource_status::deinitializing_to_failed) && !compiled))
			return e;

		// try to take this to loading
		old = f->status.exchange(has_compiled ? resource_status::initializing : resource_status::failed);
		if (old == resource_status::initializing) // let the other thread do the loading
			return e;

		if (!has_compiled && (force || old == resource_status::deinitializing))
		{
			f->status.store(resource_status::deinitializing_to_failed);
			removes_.push(f);
		}

		// anything else is some sort of loading which is flagged
		oAssert(force || old != resource_status::initialized, "unexpected status");

		if (old == resource_status::deinitializing && !force)
			f->status.store(resource_status::initialized);
		else if (has_compiled)
		{
			f->compiled = std::move(compiled);
			inserts_.push(f);
		}
	}

	else
	{
		// initialize a new entry
		f = pool_.create();
		if (f)
		{
			f->status.store(!compiled ? resource_status::failed : resource_status::initializing);
			f->key = key;
			f->path = path;
			f->compiled = std::move(compiled);
			index = (index_type)pool_.index(f);
		}
		else
			return &missing_; // oom

		e = entries_ + index;
		
		// another thread is trying to do this same thing, so let it
		// hmm... why would two threads get back the same index from a call to pool_create()? Maybe this isn't needed.
		if (nullidx != lookup_.set(key, index))
		{
			oTrace("resource_registry_t race on setting index %u", index);
			pool_.destroy(f);
		}
		else if (f->status == resource_status::failed)
			*e = failed_;
		else
			*e = loading_;

		if (has_compiled)
			inserts_.push(f);
	}

	return e;
}

bool resource_registry_t::remove(key_type key)
{
	//              Truth Table
	//													 entry
	// <no entry>								 return false
	// failed										 q/deinitializing
	// initializing							 q/deinitializing
	// initialized							 q/deinitializing
	// deinitializing						 noop
	// deinitializing_to_loading deinitializing (must be already queued)
	// deinitializing_to_failed  deinitializing (must be already queued)

	auto index = lookup_.get(key);
	if (index == nullidx)
		return false;
	auto f = pool_.typed_pointer(index);
	resource_status old = f->status.exchange(resource_status::deinitializing);
	if ((int)old == (int)resource_status::initialized || (int)old == (int)resource_status::initializing)
		removes_.push(f);
	return true;
}

bool resource_registry_t::remove(const resource_base_t& resource)
{
	void* entry = (void*)resource;
	if (!in_range(entry, entries_, pool_.capacity()))
		return false;

	index_type index = (index_type)index_of(entry, entries_, sizeof(void*));
	auto f = pool_.typed_pointer(index);
	resource_status old = f->status.exchange(resource_status::deinitializing);
	if ((int)old < (int)resource_status::deinitializing)
		removes_.push(f);
	return true;
}

bool resource_registry_t::discard(const resource_base_t& resource)
{
	//               Truth Table
	//													 entry
	// <no entry>								 return false
	// failed										 q/deinitializing_to_loading
	// initializing							 q/deinitializing_to_loading
	// initialized							 q/deinitializing_to_loading
	// deinitializing						 deinitializing_to_loading (must be already queued)
	// deinitializing_to_loading noop 
	// deinitializing_to_failed  deinitializing_to_loading (must be already queued)

	void* entry = (void*)resource;
	if (!in_range(entry, entries_, pool_.capacity()))
		return false;

	index_type index = (index_type)index_of(entry, entries_, sizeof(void*));
	auto f = pool_.typed_pointer(index);
	resource_status old = f->status.exchange(resource_status::deinitializing_to_loading);
	if ((int)old < (int)resource_status::deinitializing)
		removes_.push(f);
	return true;
}

}
