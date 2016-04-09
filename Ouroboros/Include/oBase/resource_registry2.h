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
#include <oMemory/concurrent_object_pool.h>
#include <oString/path.h>

namespace ouro {

class base_resource
{
public:
	typedef uint64_t handle_type;
	typedef std::atomic<handle_type> atm_handle_type;

	enum class status : uint8_t // (4-bit)
	{
		missing,
		loading,
		indexed,
		ready,
		error,

		count,
	};

	// @tony: The way things are shaping up, I really only need 1 bit for whether the asset is a placeholder
	// or not (i.e. placeholders don't get created or destroyed). The /type/ and thus the status of the placeholder
	// can be determined by the pointer itself, comparing against a small set of states - which is as large as there
	// are different assets.

	static handle_type pack     (void* resource, status status, uint16_t type, uint16_t refcnt); // combines all elements of a resource handle
	static handle_type repack   (handle_type previous_packed, void* resource, status status);    // updates all but refcount
	static handle_type reference(handle_type packed);                                            // add 1 to ref count
	static handle_type release  (handle_type packed);                                            // subtract 1 from ref count

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
	static const handle_type refcnt_mask  = 0xfff0000000000000;
	static const handle_type status_mask  = 0x000f000000000000;
	static const handle_type ptr_mask     = 0x0000ffffffffffff;
	static const handle_type type_mask    = 0x000000000000000f;
	static const handle_type refcnt_max   = 0x0000000000000fff;
	static const handle_type status_max   = 0x000000000000000f;
	static const handle_type refcnt_one   = 0x0010000000000000;
	static const uint32_t refcnt_shift    = 52;
	static const uint32_t status_shift    = 48;
	
	static bool     is_placeholder(status      status) { return status != status::ready;                         }
	static void*    ptr           (handle_type packed) { return (void*)  (packed                  & ptr_mask  ); }
	static status   stat          (handle_type packed) { return status  ((packed >> status_shift) & status_max); }
	static uint16_t refcnt        (handle_type packed) { return uint16_t((packed >> refcnt_shift) & refcnt_max); }
	static bool     hasref        (handle_type packed) { return (packed & refcnt_mask) != 0;                     }
	static bool     is_placeholder(handle_type packed) { return is_placeholder(stat(packed));                    }

	friend class resource_registry2_t;
	base_resource(handle_type* handle) : handle_((atm_handle_type*)handle) {}

	void reference() const; // const makes copy ctor easier

	mutable atm_handle_type* handle_;
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

	// returns the io allocator associated with this registry
	allocator get_io_allocator() { return io_alloc_; }

	// destroys all indexed resources, this should be called before any final flush during deinitialize()
	void destroy_indexed();

public:

	// returns true if this class has been initialized
	bool valid() const { return lookup_.valid(); }

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
	base_resource insert(const path_t& path, void* placeholder, blob& compiled, bool force);

	// inserts a resource that is locked in memory (like a placeholder) and is referenced directly (no handle, no ref-counting)
	// these will be cleaned up during deinitialize(). Calling insert_indexed on a valid resource will overwrite it and clean up 
	// the prior one. resolve_index will return the error placeholder until after the next flush. The intended pattern is that
	// an enumerated array of assets will be inserted and a final flush will create them at derived-registry initialization time.
	void  insert_indexed (const key_type& index, const char* label, blob& compiled);
	void* resolve_indexed(const key_type& index) const;

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
		queued_t(const path_t& path, blob& compiled, const key_type& index) : path(path), compiled(std::move(compiled)), index(index) {}

		~queued_t() = delete;

		queued_t* next;
		
		union
		{
			void* resource; // for destroy
			struct          // for create
			{
				blob     compiled;
				path_t   path;
				key_type index;
			};
		};
	};

	typedef concurrent_hash_map<key_type, uint32_t>            lookup_t;
	typedef concurrent_object_pool<queued_t>                   queued_pool_t;
	typedef concurrent_object_pool<base_resource::handle_type> res_pool_t;
	typedef concurrent_stack<queued_t>                         queue_t;
	typedef std::atomic<base_resource::handle_type>            atm_resource_t;

	lookup_t      lookup_;            // stores resources, accessible by key
	res_pool_t    res_pool_;          // stores available resources
	queued_pool_t queued_pool_;       // stores available queue nodes
	queue_t       creates_;           // queue for all compiled buffers that can be passed to create and then inserted
	queue_t       destroys_;          // queue for all valid resources to destroy in the next flush
	void*         error_placeholder_; // inserted if a file load fails
	allocator     io_alloc_;          // used for temporary file io and decode operations

	static void load_completion(const path_t& path, blob& buffer, const std::system_error* syserr, void* user);
	void load_completion(const path_t& path, blob& buffer, const std::system_error* syserr);

	// replaces an existing key with the specified resource or placeholder and updates the status
	// if the key is in a bad state, such as having been discarded, the resource will be queued for
	// destroy. Any previous valid resource will also be queued for destroy.
	void replace(const key_type& key, void* resource, const base_resource::status& status);

	// returns true if insertion occured or false if an existing handle was retreived
	bool insert_or_resolve(const key_type& key, void* placeholder, const base_resource::status& status, base_resource::handle_type*& out_handle);

	void queue_create(const path_t& path, blob& compiled, const key_type& index = 0);
	void queue_destroy(void* resource);
};

}
