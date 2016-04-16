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

	small_block_allocator();
	small_block_allocator(void* memory, size_type bytes, const uint16_t* block_sizes, size_type num_block_sizes);
	~small_block_allocator();
	small_block_allocator                 (small_block_allocator&& that);
	small_block_allocator& operator=      (small_block_allocator&& that);
	small_block_allocator                 (const small_block_allocator&) = delete;
	const small_block_allocator& operator=(const small_block_allocator&) = delete;

	// manages the specified memory block to fulfill allocators of exactly one of
	// the sizes in block_sizes. The memory must be chunk_size-aligned both in 
	// base pointer and size.
	void  initialize(void* memory, size_type bytes, const uint16_t* block_sizes, size_type num_block_sizes); // note max_num_block_sizes
	void* deinitialize();                                                                                    // returns memory passed to initialize()
	void* allocate(size_t size);                                                                             // if a best-fit is not available, this will allocate 1-size larger - careful with alignment relating to block size
	void  deallocate(void* ptr);
	bool  owns(void* ptr) const { return chunks_.owns(ptr); }                                                // range-check the pointer to within the allocator

private:
	struct chunk_t
	{
		static const uint16_t nullidx = uint16_t(-1);
		chunk_t() : prev(nullidx), next(nullidx), bin(nullidx), pad(0) {}
		pool pool;
		uint16_t prev, next, bin, pad;
	};

	void remove_chunk(chunk_t* c);
	uint16_t find_bin(size_t size);
	
	pool chunks_;
	
	std::array<uint16_t, max_num_block_sizes> blksizes_;
	std::array<uint16_t, max_num_block_sizes> partials_;
};

}
