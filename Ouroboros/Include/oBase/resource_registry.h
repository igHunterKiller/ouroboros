// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// A concurrent name<->index mapping for objects whose creation and destruction must be 
// done at a controlled time.

// Comments will refer to a 'device' below. It is assumed that resources managed
// by this registry are generated from a factory-style singleton, be it a facade for
// a large api or middleware. Often such devices have resource lifetime constraints
// such as creating/destroying from the same thread, or otherwise being single-threaded.
// The advise in the comments is not binding, the main message being this registry bridges
// higher-level code and an underlying middleware, so all the rules of that middleware apply.

// The intent of this class is to serve as a base implementation for another layer of wrapper 
// that introduces more specific types, implements file or other I/O and encapsulates any 
// factory dependencies.

#pragma once
#include <oConcurrency/concurrent_hash_map.h>
#include <oConcurrency/concurrent_stack.h>
#include <oMemory/allocate.h>
#include <oMemory/concurrent_object_pool.h>
#include <oMemory/pool.h>
#include <oString/path.h>
#include <cstdint>

namespace ouro {

enum class resource_status
{
	invalid = 0,
	failed,
	initializing,
	initialized,
	deinitializing,
	deinitializing_to_loading,
	deinitializing_to_failed,
};

class resource_base_t
{
public:
	resource_base_t() : entry_(nullptr) {}
	resource_base_t(void** that) : entry_(that) {}
	resource_base_t(void* const* that) : entry_((void**)that) {}
	inline operator bool() const { return !!entry_ && !!*entry_; }
	inline operator void*() const { return *entry_; }
protected:
	void** entry_;
};

template<typename T>
class resource_t : public resource_base_t
{
public:
	resource_t() {}
	resource_t(const resource_base_t& that) : resource_base_t(that) {}
	resource_t(resource_base_t&& that) : resource_base_t(std::move(that)) {}
	resource_t& operator=(resource_base_t&& that) { if (this != &that) { entry_ = that.entry_; that.entry_ = nullptr; } return *this; }
	resource_t(T& that) : resource_base_t(&that) {}
	resource_t(T* that) : resource_base_t(that) {}
	inline operator bool() const { return !!entry_; }
	inline operator bool() { return !!entry_; }
	inline operator T*() { return *(T**)entry_; }
	inline operator const T*() { return *(T**)entry_; }
	inline T* operator->() { return *(T**)entry_; }
	inline const T* operator->() const { return *(T**)entry_; }
};

// define this struct for initialize(). These buffers are not retained but are 
// used to initialize fallback resources used when user resources are not available.
struct resource_placeholders_t
{
	blob compiled_missing;
	blob compiled_loading;
	blob compiled_failed;
};

class resource_registry_t
{
public:
	typedef uint32_t size_type;
	typedef uint32_t index_type;
	typedef path_t::hash_type key_type;

	static const memory_alignment required_alignment = memory_alignment::cacheline;
	
	// returns the minimum size in bytes required of the arena passed to initialize()
	static size_type calc_size(size_type capacity);

	// returns the max block count that fits into the specified bytes
	static size_type calc_capacity(size_type bytes);


	// === non-concurrent api ===

protected:
	// lifetime management specific to the resource type
	// called during flush(), so if there are any rules associated with creation,
	// such as being on a certain thread, ensure flush is called from that thread

	// converts a compiled (baked/finalized) buffer into the resource type 
	// maintained in this registry. This is called during flushes()
	virtual void* create(const char* name, blob& compiled) = 0;

	// free a resource created in create()
	virtual void destroy(void* resource) = 0;

public:
	// ctor creates as empty
	resource_registry_t();

	// ctor that moves an existing resource_registry_t into this one
	resource_registry_t(resource_registry_t&& that);

	// initializing ctor: see initialize()
	resource_registry_t(void* arena, size_type bytes, resource_placeholders_t& placeholders);

	// dtor
	virtual ~resource_registry_t();

	// calls deinitialize on this before move
	resource_registry_t& operator=(resource_registry_t&& that);

	// placeholders are created (see create()) immediately. use calc_size() to determine bytes below.
	void initialize(void* arena, size_type bytes, resource_placeholders_t& placeholders);
	
	// destroys the internals of the instance and returns the arena passed to initialize()
	void* deinitialize();

	// returns true if this class has been initialized
	inline bool valid() const { return pool_.valid(); }
	
	// size/capacity accessors
	inline size_type capacity() const { return pool_.capacity(); }
	inline size_type size() const { return pool_.size(); }
	inline bool empty() const { return pool_.empty(); }

	// removes all entries from the registry except placeholders
	// note: should be called on the device thread because the flush is immediate
	void remove_all();

	// flush insert and remove queues. This should be called from the thread 
	// note: should be called on the device thread because the flush is immediate
	// returns number of operations completed
	size_type flush(size_type max_operations = ~0u);


	// === concurrent api ===

	// note: topology of data stored is fixed except during flush(), so concurrency
	// is guaranteed except during flush().

	// returns a persistently consistent resource by key. The underlying resource 
	// may be discarded or loading but the resource itself will be stable and 
	// useable until remove is called on it and the next flush() is called. This
	// may dereference to one of the resource placeholders.

	// retrieve a persistent handle; depending on the state of the true resource, 
	// this may dereference to one of the placeholder resources.
	resource_base_t get(key_type key) const;
	resource_base_t get(const path_t& path) const { return get(path.hash()); }

	// retrieve a persistent handle by index; this is a fast-path for accessing 
	// builtin resources defined by their enumeration and initialized into the 
	// registry before any other. Use with care.
	resource_base_t by_index(int index) const { return entries_ + index; }
	template<typename enumT> resource_base_t by_index(const enumT e) const { return by_index((int)e); }

	// returns the path used to identify the resource
	path_t path(key_type key) const;
	path_t path(const resource_base_t& resource) const;

	// returns the state of the specified entry
	resource_status status(key_type key) const;
	resource_status status(const path_t& path) const { return status(path.hash()); }
	resource_status status(const resource_base_t& resource) const;

	// queues the compiled blob for creation and immediately returns a handle that will 
	// dereference to a placeholder until the blob is fully processes by a flush(). If
	// compiled is null then the resource will remain a placeholder. If another thread calls
	// insert, then the most-correct will win, i.e. either an already-valid cached version or
	// a thread that successfully creates the resource to replace a failed or missing attempt.
	resource_base_t insert(key_type key, const path_t& path, blob& compiled, bool force = false);
	resource_base_t insert(const path_t& path, blob& compiled, bool force = false) { return insert(path.hash(), path, compiled, force); }
	
	// removes the entry allocated from this registry. The entry is still valid 
	// until the next call to flush() so any device async processing can finish 
	// using it.
	bool remove(key_type key);
	bool remove(const path_t& path) { return remove(path.hash()); }
	bool remove(const resource_base_t& resource);

	// Sets the resource to the loading placeholder and destroys the associated real
	// resource. This is like a remove, but without removing the bookkeeping: useful 
	// during reload operations.
	bool discard(const resource_base_t& resource);
	bool discard(const key_type key) { return discard(get(key)); }
	bool discard(const char* name) { return discard(get(name)); }

private:
	static const index_type nullidx = index_type(-1);

	struct file_info
	{
		file_info() : next(nullptr) { status = resource_status::invalid; }
		~file_info() { status = resource_status::invalid; }
		file_info* next;
		key_type key;
		std::atomic<resource_status> status;
		path_t path;
		blob compiled;
	};
	
	typedef concurrent_hash_map<uint64_t, uint32_t> hash_map_t;

	concurrent_object_pool<file_info> pool_;
	hash_map_t lookup_;
	concurrent_stack<file_info> inserts_;
	concurrent_stack<file_info> removes_;
	void** entries_;
	void* missing_;
	void* loading_;
	void* failed_;
};

}
