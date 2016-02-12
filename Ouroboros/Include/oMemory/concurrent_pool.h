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

	// if at capacity allocate() will return this value
	// (upper bits reserved for atomic tagging)
	static const index_type nullidx = 0x00ffffff;

	// the largest index issued by this pool, also serving as a static max_capacity()
	static const size_type max_index = nullidx - 1;

	// the maximum number of items a concurrent_pool can hold
	static size_type max_capacity() { return max_index; }

	// returns the minimum size in bytes required of the arena passed to initialize()
	static size_type calc_size(size_type capacity, size_type block_size);

	// returns the max block count that fits into the specified bytes
	static size_type calc_capacity(size_type bytes, size_type block_size);


	// === non-concurrent api ===

	// ctor creates as empty
	concurrent_pool();

	// ctor moves an existing pool into this one
	concurrent_pool(concurrent_pool&& that);

	// ctor creates as a valid pool using external memory
	concurrent_pool(void* arena, size_type bytes, size_type block_size);

	// dtor
	~concurrent_pool();

	// calls deinitialize on this, moves that's memory under the same config
	concurrent_pool& operator=(concurrent_pool&& that);

	// use calc_size() to determine arena size
	void initialize(void* arena, size_type bytes, size_type block_size);

	// deinitializes the pool and returns the memory passed to initialize()
	void* deinitialize();

	// returns true if this class has been initialized
	inline bool valid() const { return !!blocks_; }

	// returns the size each allocated block will be
	inline size_type block_size() const { return stride_; }

	// SLOW! walks the free list and returns the count
	size_type count_free() const;

	// SLOW! returns the number of allocated elements
	inline size_type size() const { return capacity() - count_free(); }

	// SLOW! returns true of there are no outstanding allocations
	inline bool full() const { return capacity() == count_free(); }


	// === concurrent api ===

	// returns the max number of items that can be allocated from this pool
	inline size_type capacity() const { return nblocks_; }

	// returns true if all items have been allocated
	bool empty() const;

	// allocate/deallocate an index into the pool. If empty out of resources nullidx
	// will be returned.
	index_type allocate_index();
	void deallocate(index_type index);

	// Wrappers for treating the block as a pointer to block_size memory.
	inline void* allocate() { index_type i = allocate_index(); return pointer(i); }
	inline void deallocate(void* pointer) { deallocate(index(pointer)); }

	// convert between allocated index and pointer values
	void* pointer(index_type index) const;
	index_type index(void* ptr) const;

	// simple range check that returns true if this index/pointer could have been allocated from this pool
	bool owns(index_type index) const { return index < nblocks_; }
	bool owns(void* ptr) const { return owns(index(ptr)); }

private:
	concurrent_pool(const concurrent_pool&); /* = delete; */
	const concurrent_pool& operator=(const concurrent_pool&); /* = delete; */

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
