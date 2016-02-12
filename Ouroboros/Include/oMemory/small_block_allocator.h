// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Manages block allocators for several user-specified block sizes. This grows
// pools for block sizes in page-size chunks and can recycle a page for use as 
// a different block size.

#pragma once
#include <oMemory/pool.h>
#include <array>
#include <cstdint>

namespace ouro
{

class small_block_allocator
{
public:
	typedef uint32_t size_type;

	static const uint32_t max_num_block_sizes = 16;
	static const uint32_t chunk_size = 32 * 1024;

	// returns the bookkeeping overhead
	static size_type overhead();

	// ctor creates as empty
	small_block_allocator();

	// ctor moves an existing small_block_allocator into this one
	small_block_allocator(small_block_allocator&& that);

	// ctor creates as a valid small_block_allocator using external memory
	small_block_allocator(void* arena, size_type bytes, const uint16_t* block_sizes, size_type num_block_sizes);

	// dtor
	~small_block_allocator();

	// calls deinitialize on this, moves that's memory under the same config
	small_block_allocator& operator=(small_block_allocator&& that);

	// manages the specified memory block to fulfill allocators of exactly one of
	// the sizes in block_sizes. The memory must be chunk_size-aligned both in 
	// base pointer and size.
	void initialize(void* arena, size_type bytes, const uint16_t* block_sizes, size_type num_block_sizes);

	// deinitializes and returns the memory passed to initialize()
	void* deinitialize();

	// allocates memory of the specified size if that size is equal to one of the
	// block sizes used to initialize this allocator. If no match or memory is not
	// available this returns nullptr
	void* allocate(size_t size);

	// frees memory returned from allocate
	void deallocate(void* ptr);

	// simple range check that returns true if this index/pointer could have been 
	// allocated from this allocator
	bool owns(void* ptr) const { return chunks_.owns(ptr); }

private:
	small_block_allocator(const small_block_allocator&); /* = delete; */
	const small_block_allocator& operator=(const small_block_allocator&); /* = delete; */

	struct chunk_t
	{
		static const uint16_t nullidx = uint16_t(-1);

		chunk_t() : prev(nullidx), next(nullidx), bin(nullidx), pad(0) {}

		pool pool;
		uint16_t prev;
		uint16_t next;
		uint16_t bin;
		uint16_t pad;
	};

	void remove_chunk(chunk_t* c);
	uint16_t find_bin(size_t size);
	
	pool chunks_;
	
	std::array<uint16_t, max_num_block_sizes> blksizes_;
	std::array<uint16_t, max_num_block_sizes> partials_;
};

}
