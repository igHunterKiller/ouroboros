// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// wrapper around a conceptual concurrent hash map mapping a hash of a uri_ref to its 
// contents prepared for runtime. This is intended to be the heavy-lifting innards 
// of a derived class that fills out resource-specific create/destroy and also more 
// thoroughly defines file I/O.

#pragma once
#include <atomic>
#include <cstdint>
#include <oConcurrency/concurrent_hash_map.h>
#include <oConcurrency/concurrent_stack.h>
#include <oMemory/concurrent_object_pool.h>
#include <oString/uri.h>

namespace ouro {

class base_resource_registry
{
public:
	typedef uint32_t size_type;
	typedef uint64_t key_type;

	// called by load, this function should asynchronously load the specified file's binary
	// and call complete_load() on the resulting blob.
	typedef void (*load_fn)(const uri_t& uri_ref, allocator& io_alloc, void* user);

	class handle
	{
	public:
		typedef uint64_t type;
		typedef std::atomic<type> atm_type;

		enum class status : uint8_t // (4-bit)
		{
			missing,
			loading,
			indexed,
			ready,
			error,

			count,
		};

		// ctors
		handle() : handle_(nullptr)                   {}
		~handle()                                     { release(); }
		handle(const handle& that)                    { handle_ = that.handle_; reference(); }
		handle(handle&& that) : handle_(that.handle_) { that.handle_ = nullptr; }
		const handle& operator=(const handle& that);
		handle&       operator=(handle&& that);

		// accessors
		void* get()                   const           { auto h = handle_->load();                          return ptr(h); }
		void* get(status* out_status) const           { auto h = handle_->load(); *out_status = status(h); return ptr(h); }
  
	protected:
		static const type refcnt_mask      = 0xfff0000000000000;
		static const type status_mask      = 0x000f000000000000;
		static const type ptr_mask         = 0x0000ffffffffffff;
		static const type type_mask        = 0x000000000000000f;
		static const type refcnt_max       = 0x0000000000000fff;
		static const type status_max       = 0x000000000000000f;
		static const type refcnt_one       = 0x0010000000000000;
		static const uint32_t refcnt_shift = 52;
		static const uint32_t status_shift = 48;

		friend class base_resource_registry;
	
		static type                   pack          (void* resource, const enum class status& status, uint16_t type, uint16_t refcnt); // combines all elements of a resource handle
		static type                   repack        (type previous_packed, void* resource, const enum class status& status);           // updates all but refcount
		static type                   reference     (type packed);                                                                     // add 1 to ref count
		static type                   release       (type packed);                                                                     // subtract 1 from ref count

		static bool                   is_placeholder(const status& status) { return status != status::ready;                                  }
		static bool                   is_indexed    (const status& status) { return status == status::indexed;                                }
		static void*                  ptr           (type          packed) { return (void*)  (packed                  & ptr_mask  );          }
		static enum class status      status        (type          packed) { return enum class status((packed >> status_shift) & status_max); }
		static uint16_t               refcnt        (type          packed) { return uint16_t((packed >> refcnt_shift) & refcnt_max);          }
		static bool                   hasref        (type          packed) { return (packed & refcnt_mask) != 0;                              }
		static bool                   is_placeholder(type          packed) { return is_placeholder(status(packed));                           }

		handle(type* h) : handle_((atm_type*)h) {}

		void reference();
		void release();

		atm_type* handle_;
	};

	static const memory_alignment required_alignment = memory_alignment::cacheline;

	// returns the minimum size in bytes required of memory passed to initialize_base()
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
	virtual void* create(const uri_t& uri_ref, blob& compiled) = 0;

	// free a resource created in create()
	virtual void destroy(void* resource) = 0;

	// ctor creates as empty
	base_resource_registry();

	// ctor that moves an existing base_resource_registry into this one
	base_resource_registry(base_resource_registry&& that);

	// dtor
	virtual ~base_resource_registry();

	// calls deinitialize on this before move
	base_resource_registry& operator=(base_resource_registry&& that);

	// derived classes should set up all apparatus for create/destroy to work, then call this - placeholder is immediately created
	void initialize_base(const char* registry_label, void* memory, size_type bytes, blob& error_placeholder, load_fn load, void* load_user, const allocator& io_alloc);
	
	// destroys the internals of the instance and returns the memory passed to initialize_base()
	void* deinitialize_base();

	// returns the io allocator associated with this registry
	allocator get_io_allocator() { return io_alloc_; }

	// queues for create, or if invalid inserts error placeholder
	void complete_load(const uri_t& uri_ref, blob& compiled, const char* error_message);
	
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
	handle resolve(const key_type& key, void* placeholder);
	handle resolve(const uri_t& uri_ref, void* placeholder) { return resolve(uri_ref.hash(), placeholder); }

	// takes ownership of and queues compiled for creation and immediately returns a 
	// ref-counted handle to it. If force is false, the handle may resolve to a pre-existing
	// entry and no ownership of compiled is taken. If a new entry, the specified placeholder
	// is inserted immediately to hold resolution over until the queue is flushed.
	handle insert(const uri_t& uri_ref, void* placeholder, blob& compiled, bool force);

	// inserts a resource that is locked in memory (like a placeholder) and is referenced directly (no handle, no ref-counting)
	// these will be cleaned up during deinitialize_base(). Calling insert_indexed on a valid resource will overwrite it and 
	// clean up the prior one. resolve_index will return the error placeholder until after the next flush. The intended pattern 
	// is that an enumerated array of assets will be inserted and a final flush will create them at derived-registry 
	// initialization time.
	void  insert_indexed (const key_type& index, const char* label, blob& compiled);
	void* resolve_indexed(const key_type& index) const;

	// queues uri_ref for async loading and immediately returns a handle that will resolve to a 
	// placeholder until loading completes, at which time the result will be queued for creation.
	// If force is false, this may resolve to an already-existing handle. If force is true on a 
	// pre-existing valid resource, it will remain that resource until the load completes instead 
	// of an immediately new placeholder.
	handle load(const uri_t& uri_ref, void* placeholder, bool force);

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
		queued_t(const uri_t& uri_ref, blob& compiled, const key_type& index) : uri_ref(uri_ref), compiled(std::move(compiled)), index(index) {}

		~queued_t() = delete;

		queued_t* next;
		
		union
		{
			void* resource; // for destroy
			struct          // for create
			{
				blob     compiled;
				uri_t    uri_ref;
				key_type index;
			};
		};
	};

	typedef concurrent_hash_map<key_type, uint32_t> lookup_t;
	typedef concurrent_object_pool<queued_t>        queued_pool_t;
	typedef concurrent_object_pool<handle::type>    res_pool_t;
	typedef concurrent_stack<queued_t>              queue_t;
	typedef std::atomic<handle::type>               atm_resource_t;

	lookup_t      lookup_;            // stores resources, accessible by key
	res_pool_t    res_pool_;          // stores available resources
	queued_pool_t queued_pool_;       // stores available queue nodes
	queue_t       creates_;           // queue for all compiled buffers that can be passed to create and then inserted
	queue_t       destroys_;          // queue for all valid resources to destroy in the next flush
	load_fn       load_;              // a function to asynchronously load the binary of a uri_ref
	void*         load_user_;         // user data for calling load_
	void*         error_placeholder_; // inserted if a file load fails
	allocator     io_alloc_;          // used for temporary file io and decode operations
	sstring       label_;             // name used in traces

	// replaces an existing key with the specified resource or placeholder and updates the status
	// if the key is in a bad state, such as having been discarded, the resource will be queued for
	// destroy. Any previous valid resource will also be queued for destroy.
	void replace(const key_type& key, void* resource, const enum class handle::status& status);

	// returns true if insertion occured or false if an existing handle was retreived
	bool insert_or_resolve(const key_type& key, void* placeholder, const enum class handle::status& status, handle::type*& out_handle);

	// destroys all indexed resources, this should be called before any final flush during deinitialize_base()
	void destroy_indexed();

	void queue_create(const uri_t& uri_ref, blob& compiled, const key_type& index = 0);
	void queue_destroy(void* resource);
};

template<typename resourceT>
class resource_registry : protected base_resource_registry
{
public:
	typedef resourceT                         resource_type;
	typedef base_resource_registry::size_type size_type;

	class handle : public base_resource_registry::handle
	{
	public:
		typedef resourceT type;

		handle()                                           : base_resource_registry::handle()                {}
		handle(const handle&                         that) : base_resource_registry::handle(that)            {}
		handle(const base_resource_registry::handle& that) : base_resource_registry::handle(that)            {}
		~handle()                                                                                            { release(); }
		handle(handle&& that)                              : base_resource_registry::handle(std::move(that)) {}
		handle(base_resource_registry::handle&& that)      : base_resource_registry::handle(std::move(that)) {}

		const handle& operator=(const handle& that)                        { return (const handle&)base_resource_registry::handle::operator=(that); }
		handle&       operator=(handle&& that)                             { return (handle&)base_resource_registry::handle::operator=(std::move(that)); }

		type* get()                                                  const { return (type*)base_resource_registry::handle::get(); }
		type* get(enum class status* out_status)                     const { return (type*)base_resource_registry::handle::get(out_status); }
		type* get(enum class status* out_status, uint16_t* out_type) const { return (type*)base_resource_registry::handle::get(out_status, out_type); }
		enum class status status()                                   const { enum class status s; get(&s); return s; }

		operator bool         () const                                     { return !!entry_; }
		operator bool         ()                                           { return !!entry_; }
		operator type*        ()                                           { return get(); }
		operator const type*  ()                                           { return get(); }
		type*       operator->()                                           { return get(); }
		const type* operator->() const                                     { return get(); }
	};


	// == concurrent api ==

	handle         resolve        (const key_type& key,   resource_type* placeholder)                                 { return (handle)        base_resource_registry::resolve(key, placeholder);                  }
	handle         resolve        (const uri_t&    uri_ref,   resource_type* placeholder)                             { return (handle)        base_resource_registry::resolve(uri_ref.hash(), placeholder);          }
	handle         insert         (const uri_t&    uri_ref,   resource_type* placeholder, blob& compiled, bool force) { return (handle)        base_resource_registry::insert(uri_ref, placeholder, compiled, force); }
	handle         load           (const uri_t&    uri_ref,   resource_type* placeholder,                 bool force) { return (handle)        base_resource_registry::load(uri_ref, placeholder, force);             }
	void           insert_indexed (const key_type& index, const char* label,                blob& compiled)           {                        base_resource_registry::insert_indexed(index, label, compiled);     }
	resource_type* resolve_indexed(const key_type& index) const                                                       { return (resource_type*)base_resource_registry::resolve_indexed(index);                     }
};

}
