// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// O(1) fine-grained concurrent block allocator: uses space inside free blocks_ to 
// maintain freelist.

#pragma once
#include <oArch/arch.h>
#include <cstdint>
#include <atomic>

namespace ouro {

class concurrent_pool
{
public:
	typedef uint32_t index_type;
	typedef uint32_t size_type;

	static const index_type nullidx  = 0x00ffffff;                                 // if at capacity allocate() will return this value (upper bits reserved for atomic tagging)
	static const size_type max_index = nullidx - 1;                                // largest index issued by this pool, also serving as a static max_capacity()
	
	static size_type max_capacity() { return max_index; }                          // maximum number of items a concurrent_pool can hold
	static size_type calc_size(size_type capacity, size_type block_size);          // required bytes for the memory passed to initialize()
	static size_type calc_capacity(size_type bytes, size_type block_size);         // max block count that fits in the specified bytes


	// === non-concurrent api ===

	concurrent_pool();                                                             // default ctor (creates as empty)
	concurrent_pool(concurrent_pool&& that);                                       // move ctor
	concurrent_pool(void* arena, size_type bytes, size_type block_size);           // ctor calls initialize()
	~concurrent_pool();                                                            // dtor
	concurrent_pool& operator=(concurrent_pool&& that);                            // calls deinitialize on this, moves in that's memory 
	concurrent_pool(const concurrent_pool&)                  = delete;		         // no copy ctor
	const concurrent_pool& operator=(const concurrent_pool&) = delete;		         // no copy assigment

	void initialize(void* arena, size_type bytes, size_type block_size);           // use calc_size() to determine memory size
	void* deinitialize();                                                          // returns the memory passed to initialize()

	size_type  block_size()           const { return stride_; }                    // size of each allocated block
	size_type  count_free()           const;                                       // SLOW! (walks freelist) number of items available
	size_type  size()                 const { return capacity() - count_free(); }  // SLOW! number of allocated elements
	bool       valid()                const { return !!blocks_; }                  // true if the pool has been initialized
	bool       full()                 const { return capacity() == count_free(); } // SLOW! true if there are no outstanding allocations


	// === concurrent api ===

	size_type  capacity()             const { return nblocks_; }                   // max number of items that can be available
	bool       empty()                const;                                       // true if all items have been allocated

	bool       owns(index_type index) const { return index < nblocks_; }			     // range-check the pointer to within the pool
	bool       owns(void* ptr)        const { return owns(index(ptr)); }			     // range-check the pointer to within the pool

	index_type allocate_index();																									 // same allocation as via pointer below, nullidx on failure
	void       deallocate(index_type index);																			 // same allocation as via pointer below

	void*      allocate()                { return pointer(allocate_index()); }     // allocation returns a pointer to a block_size chunk
	void       deallocate(void* pointer) { deallocate(index(pointer)); }           // free by pointer

	void*      pointer(index_type index) const;                                   // pointer from index
	index_type index(void* ptr)          const;                                   // index from pointer

private:
  union
	{
		char cache_padding_[oCACHE_LINE_SIZE];
		struct
		{
			concurrent_pool* next_;
			uint8_t* blocks_;
			size_type stride_;
			size_type nblocks_;
			std::atomic_uint head_;
		};
	};
};
static_assert(sizeof(concurrent_pool) == oCACHE_LINE_SIZE, "size mismatch");

}
