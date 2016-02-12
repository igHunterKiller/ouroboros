// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Allocator implementation using the segregated binary buddy algorithm.

#pragma once
#include <oMemory/allocate.h>

namespace ouro {

class sbb_allocator
{
public:
	static const size_t default_block_size = 16;

  // returns the size the arena param must be in initialize() below
	static size_t calc_bookkeeping_size(size_t arena_bytes, size_t min_block_size = default_block_size);
  
  sbb_allocator() : heap_(nullptr), heap_size_(0) {}
	sbb_allocator(void* arena, size_t bytes, void* bookkeeping, size_t min_block_size = default_block_size) { initialize(arena, bytes, bookkeeping, min_block_size); }
	~sbb_allocator() { deinitialize(); }

	sbb_allocator(sbb_allocator&& that);
	sbb_allocator& operator=(sbb_allocator&& that);

	// creates an allocator for the specified arena keeping all bookkeeping in separate memory
  // arena can be nullptr if this allocated is intended to allocate offsets into non-local memory
	void initialize(void* arena, size_t bytes, void* bookkeeping, size_t min_block_size = default_block_size);

	// invalidates the allocator and returns the memory passed in initialize as bookkeeping
	void* deinitialize();

	const void* bookkeeping() const;
	const void* arena() const;
	inline size_t arena_size() const;
	allocator_stats get_stats() const;
	void* allocate(size_t bytes, const char* label = "?", const allocate_options& options = allocate_options());
	void* reallocate(void* ptr, size_t bytes);
	void deallocate(void* ptr);
	size_t size(void* ptr) const;
	bool owns(void* ptr) const;
	bool valid() const;
	void reset();
	void walk_heap(allocate_heap_walk_fn walker, void* user);

private:
	sbb_allocator(const sbb_allocator&)/* = delete*/;
	const sbb_allocator& operator=(const sbb_allocator&)/* = delete*/;

	void* heap_;
	size_t heap_size_;
	size_t min_block_size_;
	allocator_stats stats_;
};

}
