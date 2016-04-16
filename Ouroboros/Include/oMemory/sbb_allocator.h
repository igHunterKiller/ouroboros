// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Allocator implementation using the segregated binary buddy algorithm.

// The allocator works solely on bookkeeping memory - the actual user 
// memory is never touched so this allocator is appropriate to manage 
// write-combined or foreign (GPU) memory.

#pragma once
#include <oMemory/allocate.h>

namespace ouro {

class sbb_allocator
{
public:
	static const size_t default_block_size = 16;

	// size allocated for param 'bookkeeping' in initialize() below: pass in the byte size to be managed
	static size_t calc_bookkeeping_size(size_t memory_bytes, size_t min_block_size = default_block_size);
  
  sbb_allocator()                                                                                         : heap_(nullptr), heap_size_(0) {}
	sbb_allocator(void* memory, size_t bytes, void* bookkeeping, size_t min_block_size = default_block_size)                                { initialize(memory, bytes, bookkeeping, min_block_size); }
	~sbb_allocator()                                                                                                                        { deinitialize(); }
	sbb_allocator                 (sbb_allocator&& that);
	sbb_allocator& operator=      (sbb_allocator&& that);
	sbb_allocator                 (const sbb_allocator&) = delete;
	const sbb_allocator& operator=(const sbb_allocator&) = delete;

	// use calc_bookkeeping_size() to determine memory size for bookkeeping. memory can be nullptr: this allocator is intended to manage foreign memory
	void            initialize(void* memory, size_t bytes, void* bookkeeping, size_t min_block_size = default_block_size);
	void*           deinitialize();                                      // returns 'bookkeeping' as passed to initialize()
	const void*     bookkeeping()         const;                         // value passed to bookkeeping in initialize()
	const void*     base()                const;                         // value passed to memory in initialize()
	size_t          capacity()            const;                         // size of allocatable memory
	allocator_stats get_stats()           const;                         // general stats from the allocator's current state
	size_t          size  (void* ptr)     const;                         // allocated size, not always the same as the user size specified to (re)allocate()
	size_t          offset(void* ptr)     const;                         // bytes off from the memory passed to initialize
	void*           ptr   (size_t offset) const;                         // given a value returned by offset(), convert it back to a pointer from this allocator
	bool            owns  (void* ptr)     const;                         // range-check the pointer to within the allocator
	bool            valid()               const;                         // true for an initialize()'ed heap
	void            reset();                                             // returns allocator to state just after initialize() is called
	void            walk_heap(allocate_heap_walk_fn walker, void* user); // visits each allocation for debugging/reporting purposes
	
	void*           allocate  (           size_t bytes, const char* label = "?", const allocate_options& options = allocate_options());
	void*           reallocate(void* ptr, size_t bytes);
	void            deallocate(void* ptr);

private:
	void*           heap_;
	size_t          heap_size_;
	size_t          min_block_size_;
	allocator_stats stats_;
};

}
