// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// A wrapper for a handle returned from resource_registry. This gets a 
// direct handle into the storage of the registry, and that is a pointer
// to a resource with metadata. The only metadata this modifies is the 
// reference count, but it can see modifications made by resource registry.
// When the refcount goes to 0, this does not itself take action to free
// the resource. Instead all 0-reference resources are swept up at a known
// time since resoures of this type tend to be created from a single-threaded
// device (d3d).

#pragma once
#include <atomic>
#include <cstdint>
#include <oConcurrency/concurrent_hash_map.h>
#include <oConcurrency/concurrent_stack.h>
#include <oString/path.h>

namespace ouro {

class base_resource
{
public:
	enum class status : uint8_t // (4-bit)
	{
		missing,
		loading,
		ready,
		error,
	};

	// @tony: The way things are shaping up, I really only need 1 bit for whether the asset is a placeholder
	// or not (i.e. placeholders don't get created or destroyed). The /type/ and thus the status of the placeholder
	// can be determined by the pointer itself, comparing against a small set of states - which is as large as there
	// are different assets.

	static uint64_t pack(void* resource, status status, uint16_t type, uint16_t refcnt);
	static uint64_t repack(uint64_t previous_packed, void* resource, status status);
	static uint64_t reference(uint64_t packed); // add 1 to ref count
	static uint64_t release(uint64_t packed);   // subtract 1 from ref count

	// ctors
	base_resource() : handle_(nullptr)                                       {}
	~base_resource()                                                         { release(); }
	base_resource(const base_resource& that)                                 { that.reference(); handle_ = that.handle_; }
	base_resource(base_resource&& that) : handle_(that.handle_)              { that.handle_ = nullptr; }
  const base_resource& operator=(const base_resource& that);
	base_resource&       operator=(base_resource&& that);

  // accessors
	void* get()                   const                  { auto h = handle_->load();                        return ptr(h); }
	void* get(status* out_status) const                  { auto h = handle_->load(); *out_status = stat(h); return ptr(h); }
  
	void release()   const; // symmetry with reference()
	
protected:
	static const uint64_t refcnt_mask  = 0xfff0000000000000;
	static const uint64_t status_mask  = 0x000f000000000000;
	static const uint64_t ptr_mask     = 0x0000ffffffffffff;
  static const uint64_t type_mask    = 0x000000000000000f;
  static const uint64_t refcnt_max   = 0x0000000000000fff;
  static const uint64_t status_max   = 0x000000000000000f;
  static const uint64_t refcnt_one   = 0x0010000000000000;
  static const uint32_t refcnt_shift = 52;
  static const uint32_t status_shift = 48;
	
	static bool     is_placeholder(status   status) { return status != status::ready;                         }
	static void*    ptr           (uint64_t packed) { return (void*)  (packed                  & ptr_mask  ); }
	static status   stat          (uint64_t packed) { return status  ((packed >> status_shift) & status_max); }
	static uint16_t refcnt        (uint64_t packed) { return uint16_t((packed >> refcnt_shift) & refcnt_max); }
	static bool     hasref        (uint64_t packed) { return (packed & refcnt_mask) != 0;                     }
	static bool     is_placeholder(uint64_t packed) { return is_placeholder(stat(packed));                    }

	friend class resource_registry2_t;
	base_resource(uint64_t* handle) : handle_((std::atomic_uint64_t*)handle) {}

	void reference() const; // const makes copy ctor easier

	mutable std::atomic_uint64_t* handle_;
};

template<typename T>
class typed_resource : public base_resource
{
public:
	typedef T type;

	typed_resource()                           : base_resource()                {}
	typed_resource(const typed_resource& that) : base_resource(that)            {}
	typed_resource(const base_resource& that)  : base_resource(that)            {}
	~typed_resource()                                                           { release(); }
	typed_resource(typed_resource&& that)      : base_resource(std::move(that)) {}
	typed_resource(base_resource&& that)       : base_resource(std::move(that)) {}

	const typed_resource& operator=(const typed_resource& that)                 { return (const typed_resource&)base_resource::operator=(that); }
	const typed_resource& operator=(const base_resource& that)                  { return (const typed_resource&)base_resource::operator=(that); }
	typed_resource&       operator=(typed_resource&& that)                      { return (typed_resource&)base_resource::operator=(std::move(that)); }
	typed_resource&       operator=(base_resource&& that)                       { return (typed_resource&)base_resource::operator=(std::move(that)); }

	type* get()                                       const                     { return (type*)base_resource::get(); }
	type* get(status* out_status)                     const                     { return (type*)base_resource::get(out_status); }
	type* get(status* out_status, uint16_t* out_type) const                     { return (type*)base_resource::get(out_status, out_type); }

	operator bool         () const                                              { return !!entry_; }
	operator bool         ()                                                    { return !!entry_; }
	operator type*        ()                                                    { return get(); }
	operator const type*  ()                                                    { return get(); }
	type*       operator->()                                                    { return get(); }
	const type* operator->() const                                              { return get(); }
};

class resource_registry2_t
{
public:
	typedef uint32_t size_type;
	typedef uint64_t key_type;
	typedef uint64_t val_type;

	static const memory_alignment required_alignment = memory_alignment::cacheline;

	// returns the minimum size in bytes required of memory passed to initialize()
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
	virtual void* create(const path_t& path, blob& compiled) = 0;

	// free a resource created in create()
	virtual void destroy(void* resource) = 0;

	// ctor creates as empty
	resource_registry2_t();

	// ctor that moves an existing resource_registry2_t into this one
	resource_registry2_t(resource_registry2_t&& that);

	// initializing ctor: see initialize()
	resource_registry2_t(void* memory, size_type bytes, blob& error_placeholder, const allocator& io_alloc);

	// dtor
	virtual ~resource_registry2_t();

	// calls deinitialize on this before move
	resource_registry2_t& operator=(resource_registry2_t&& that);

	// placeholder is immediately inserted
	void initialize(void* memory, size_type bytes, void* error_placeholder, const allocator& io_alloc);
	
	// destroys the internals of the instance and returns the arena passed to initialize()
	void* deinitialize();

public:

	// returns true if this class has been initialized
	bool valid() const { return pool_.valid(); }

	// ...
	void* get_error_placeholder() { return error_placeholder_; }
	
	// destroys all zero-ref resources, flushes other queued destroys and creates, 
	// and reclaims derelict keys. This is not concurrent and should be called at a 
	// time the device that supports create/destroy is not processing data. Returns 
	// the number of operations processed.
	size_type flush(size_type max_operations = ~0u);


	// === concurrent api ===

	// resolve the key to an existing key, or create a new one with the specified placeholder
	base_resource resolve(const key_type& key, void* placeholder);
	base_resource resolve(const path_t& path, void* placeholder) { return resolve(path.hash(), placeholder); }

	// takes ownership of and queues compiled for creation and immediately returns a 
	// ref-counted handle to it. If force is false, the handle may resolve to a pre-existing
	// entry and no ownership of compiled is taken. If a new entry, the specified placeholder
	// is inserted immediately to hold resolution over until the queue is flushed.
	base_resource insert(const path_t& path, blob& compiled, void* placeholder, bool force);

	// queues path for async loading and immediately returns a handle that will resolve to a 
	// placeholder until loading completes, at which time the result will be queued for creation.
	// If force is false, this may resolve to an already-existing handle. If force is true on a 
	// pre-existing valid resource, it will remain that resource until the load completes instead 
	// of an immediately new placeholder.
	base_resource load(const path_t& path, void* placeholder, bool force);

	// note: unloading/erasing is done by eviscerating all base_resource handles so they release 
	// their refcount. flush() garbage-collects all zero-referenced resources. It's also possible
	// through inserts/loads to validly resolve to a zero-referenced resource and increment it to
	// one, thereby preventing the next flush from destroying it. Imagine a level transition:
	// unload all previous assets, load all next-level assets and any shared assets will naturally
	// resolve and short-circuit a load request.

private:
	
	struct queued_t
	{
		queued_t(void* resource) : resource(resource) {}
		queued_t(const path_t& path, blob& compiled) : path(path), compiled(std::move(compiled)) {}

		~queued_t() = delete;

		queued_t* next;
		
		union
		{
			void* resource; // for destroy
			struct          // for create
			{
				blob compiled;
				path_t path;
			};
		};
	};
	
	concurrent_hash_map<key_type, val_type> store_;             // stores resources, accessible by key
	concurrent_object_pool<queued_t>        pool_;              // stores available queue nodes
	concurrent_stack<queued_t>              creates_;           // queue for all compiled buffers that can be passed to create and then inserted
	concurrent_stack<queued_t>              destroys_;          // queue for all valid resources to destroy in the next flush
	allocator                               io_alloc_;          // used for temporary file io and decode operations
	void*                                   error_placeholder_; // inserted if a file load fails

	static void load_completion(const path_t& path, blob& buffer, const std::system_error* syserr, void* user);
	void load_completion(const path_t& path, blob& buffer, const std::system_error* syserr);

	// replaces an existing key with the specified resource or placeholder and updates the status
	// if the key is in a bad state, such as having been discarded, the resource will be queued for
	// destroy. Any previous valid resource will also be queued for destroy.
	void replace(const key_type& key, void* resource, const base_resource::status& status);

	// returns true if insertion occured or false if an existing handle was updated
	bool insert_or_resolve(const key_type& key, void* placeholder, const base_resource::status& status, val_type*& out_handle);

	void queue_create(const path_t& path, blob& compiled);
	void queue_destroy(void* resource);
};

}
