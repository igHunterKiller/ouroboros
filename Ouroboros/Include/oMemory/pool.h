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

	// if at capacity allocate() will return this value
	static const index_type nullidx = index_type(-1);

	// largest index issued by this pool, also serving as a static max_capacity()
	static const size_type max_index = nullidx - 1;

	// the maximum number of items a pool can hold
	static size_type max_capacity() { return max_index; }

	// returns the bookkeeping overhead
	static size_type overhead();

	// returns the minimum size in bytes required of the memory passed to initialize()
	static size_type calc_size(size_type capacity, size_type block_size);

	// returns the max block count that fits into the specified bytes
	static size_type calc_capacity(size_type bytes, size_type block_size);

	// ctor creates as empty
	pool();

	// ctor moves an existing pool into this one
	pool(pool&& that);

	// ctor creates as a valid pool using external memory
	pool(void* memory, size_type bytes, size_type block_size);

	// dtor
	~pool();

	// calls deinitialize on this, moves that's memory under the same config
	pool& operator=(pool&& that);

	// no copy semantics
	pool(const pool&) = delete;
	const pool& operator=(const pool&) = delete;

	// use calc_size() to determine memory size
	void initialize(void* memory, size_type bytes, size_type block_size);

	// deinitializes the pool and returns the memory passed to initialize()
	void* deinitialize();

	// returns if the pool has been initialized
	inline bool valid() const { return !!blocks_; }

	// returns the size each allocated block will be
	inline size_type block_size() const { return stride_; }

	// returns the number of items available
	inline size_type size_free() const { return nfree_; }

	// returns the number of allocated elements
	inline size_type size() const { return capacity() - size_free(); }

	// returns true of there are no outstanding allocations
	inline bool full() const { return capacity() == size_free(); }

	// returns the max number of items that can be allocated from this pool
	inline size_type capacity() const { return nblocks_; }

	// returns true if all items have been allocated
	inline bool empty() const { return head_ == nullidx; }

	// allocate/deallocate an index into the pool. If empty out of resources 
	// nullidx will be returned.
	index_type allocate_index();
	void deallocate(index_type index);

	// Wrappers for treating the block as a pointer to block_size memory.
	inline void* allocate() { index_type i = allocate_index(); return pointer(i); }
	inline void deallocate(void* pointer) { deallocate(index(pointer)); }

	// convert between allocated index and pointer values
	void* pointer(index_type index) const;
	index_type index(void* ptr) const;

	// simple range check that returns true if this index/pointer could have been 
	// allocated from this pool
	bool owns(index_type index) const { return index < nblocks_; }
	bool owns(void* ptr) const { return owns(index(ptr)); }

private:
	uint8_t* blocks_;
	size_type stride_;
	size_type nblocks_;
	size_type nfree_;
	uint32_t head_;
};

}
