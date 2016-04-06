// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// O(1) block allocator: uses space inside free blocks to maintain freelist.

#pragma once
#include <cstdint>

namespace ouro {

class pool
{
public:
	typedef uint32_t index_type;
	typedef uint32_t size_type;

	static const index_type nullidx   = index_type(-1);                           // allocate() returns this if at capacity 
	static const size_type  max_index = nullidx - 1;                              // largest index issued by this pool
	
	static size_type max_capacity() { return max_index; }                         // the maximum number of items a pool can hold
	static size_type overhead();                                                  // returns the bookkeeping overhead
	static size_type calc_size(size_type capacity, size_type block_size);         // required bytes for the memory passed to initialize()
	static size_type calc_capacity(size_type bytes, size_type block_size);        // max block count that fits in the specified bytes

	pool();                                                                       // default ctor (creates as empty)
	pool(pool&& that);                                                            // move ctor
	pool(void* memory, size_type bytes, size_type block_size);                    // ctor calls initialize()
	~pool();                                                                      // dtor
	pool& operator=(pool&& that);                                                 // calls deinitialize on this, moves in that's memory 
	pool(const pool&)                  = delete;                                  // no copy ctor
	const pool& operator=(const pool&) = delete;                                  // no copy assigment
	
	void       initialize(void* memory, size_type bytes, size_type block_size);   // use calc_size() to determine memory size
	void*      deinitialize();                                                    // returns the memory passed to initialize()
	
	size_type  block_size()           const { return stride_; }                   // size of each allocated block
	size_type  size_free()            const { return nfree_; }                    // number of items available
	size_type  size()                 const { return capacity() - size_free(); }  // number of allocated elements
	size_type  capacity()             const { return nblocks_; }                  // max number of items that can be available
	bool       valid()                const { return !!blocks_; }                 // true if the pool has been initialized
	bool       full()                 const { return capacity() == size_free(); } // true if there are no outstanding allocations
	bool       empty()                const { return head_ == nullidx; }          // true if all items have been allocated

	bool       owns(index_type index) const { return index < nblocks_; }          // range-check the pointer to within the pool
	bool       owns(void* ptr)        const { return owns(index(ptr)); }          // range-check the pointer to within the pool

	index_type allocate_index();                                                  // same allocation as via pointer below, nullidx on failure
	void       deallocate(index_type index);                                      // same allocation as via pointer below

	void*      allocate()                { return pointer(allocate_index()); }    // allocation returns a pointer to a block_size chunk
	void       deallocate(void* pointer) { deallocate(index(pointer)); }          // free by pointer

	void*      pointer(index_type index) const;                                   // pointer from index
	index_type index(void* ptr)          const;                                   // index from pointer

private:
	uint8_t*  blocks_;
	size_type stride_;
	size_type nblocks_;
	size_type nfree_;
	uint32_t  head_;
};

}
