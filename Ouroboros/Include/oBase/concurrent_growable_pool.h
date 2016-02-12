// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.
#ifndef oBase_concurrent_growable_pool_h
#define oBase_concurrent_growable_pool_h

#include <oConcurrency/concurrent_stack.h>
#include <oMemory/concurrent_pool.h>

namespace ouro {

class concurrent_growable_pool
{
public:
	typedef concurrent_pool::size_type size_type;

	static const size_type max_blocks_per_chunk = concurrent_pool::max_index;


	// non-concurrent api

	// ctor creates as empty
	concurrent_growable_pool();

	// ctor that moves an existing pool into this one
	concurrent_growable_pool(concurrent_growable_pool&& that);

	// ctor creates as a valid pool using internally allocated memory
	concurrent_growable_pool(size_type block_size, size_type capacity_per_chunk, size_type block_alignment = oDEFAULT_MEMORY_ALIGNMENT);

	// dtor
	~concurrent_growable_pool();

	// calls deinitialize on this, moves that's memory under the same config
	concurrent_growable_pool& operator=(concurrent_growable_pool&& that);

	// self-allocates and manages memory used, otherwise this is like the other initialize()
	bool initialize(size_type block_size, size_type capacity_per_chunk, size_type block_alignment = oDEFAULT_MEMORY_ALIGNMENT);

	// deinitializes the pool, returning it to a default-constructed state.
	void deinitialize();

	// Reserves enough memory to allocate the specified number of elements. Allocation 
	// occurs in chunks so this may allocate more memory than is needed. This will not 
	// release memory, only ensure there is at least enough to meet the specified needs. 
	// Existing allocations will be preserved but this should not be called when other 
	// threads might be allocating or deallocating memory.
	void grow(size_type capacity);

	// Ensures the specified number of elements can be allocated and will release all 
	// chunks not required to meet the specified needs. Only chunks with zero outstanding 
	// allocations can be freed so this will quietly skip any chunks with at least one
	// outstanding allocation. Existing allocations will be preserved but this should not 
	// be called when other threads might be allocating or deallocating memory.
	void shrink(size_type capacity);

	// SLOW! walks the chunks list to count them
	size_type num_chunks() const;

	// SLOW! returns the current capacity without (auto)growing or shrinking
	inline size_type capacity() const { return num_chunks() * chunk_capacity(); }

	// SLOW! walks the free list and returns the count of all chunk pools
	size_type count_free() const;

	// SLOW! returns the number of allocated elements
	inline size_type size() const { return capacity() - count_free(); }

	// SLOW! returns true of there are no outstanding allocations
	inline bool full() const { return capacity() == count_free(); }

	// Returns true if the specified pointer was allocated by this allocator.
	bool owns(void* pointer) const;


	// concurrent api

	// returns the number of blocks per chunk as specified at initialization
	inline size_type chunk_capacity() const { return capacity_per_chunk; }

	// Allocates a block. If there isn't enough memory this will allocate more
	// memory from the underlying platform. Such allocations occur in chunks. The 
	// memory is not yielded back to the platform automatically: use shrink() to 
	// explicitly deallocate chunks.
	void* allocate_pointer();
	void deallocate(void* pointer);

private:
	struct chunk_t
	{
		concurrent_pool pool;
		chunk_t* next;
	};

	std::atomic<concurrent_pool*> last_allocate;
	std::atomic<concurrent_pool*> last_deallocate;
	size_type block_size;
	size_type block_alignment;
	size_type capacity_per_chunk;

	concurrent_stack<chunk_t> chunks;

	// allocates and initializes a new chunk for when out of currently reserved memory
	chunk_t* allocate_chunk();

	// maps a pointer back to the pool it came from
	concurrent_pool* find_pool(void* _Pointer) const;

	concurrent_growable_pool(const concurrent_growable_pool&); /* = delete */
	const concurrent_growable_pool& operator=(const concurrent_growable_pool&); /* = delete */
};

}

#endif
