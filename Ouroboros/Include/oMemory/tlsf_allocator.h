// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Allocator implementation using the two-level segregated fit allocation 
// algorithm. This is a wrapper for the excellent allocator found at 
// http://tlsf.baisoku.org/.

#pragma once
#include <oMemory/allocate.h>

namespace ouro {

class tlsf_allocator
{
public:
	tlsf_allocator()                           : heap_(nullptr), heap_size_(0) {}
	tlsf_allocator(void* memory, size_t bytes)                                 { initialize(memory, bytes); }
	~tlsf_allocator()                                                          { deinitialize(); }
	tlsf_allocator                 (tlsf_allocator&& that);
	tlsf_allocator& operator=      (tlsf_allocator&& that);
	tlsf_allocator                 (const tlsf_allocator&) = delete;
	const tlsf_allocator& operator=(const tlsf_allocator&) = delete;

	void  initialize(void* memory, size_t bytes);
	void* deinitialize(); // returns memory passed to initialize()

	const void*     base()            const { return heap_; }						 // value passed to memory in initialize()
	size_t          capacity()        const { return heap_size_; }			 // size of allocatable memory
	allocator_stats get_stats()       const;                             // general stats from the allocator's current state
	size_t          size  (void* ptr) const;                             // allocated size, not always the same as the user size specified to (re)allocate()
	size_t          offset(void* ptr) const;                             // bytes off from the memory passed to initialize
	bool            owns  (void* ptr) const;                             // range-check the pointer to within the allocator
	bool            valid()           const;                             // true for an initialize()'ed heap
	void            reset();                                             // returns allocator to state just after initialize() is called
	void            walk_heap(allocate_heap_walk_fn walker, void* user); // visits each allocation for debugging/reporting purposes

	void*           allocate(size_t bytes, const char* label = "?", const allocate_options& options = allocate_options());
	void*           reallocate(void* ptr, size_t bytes);
	void            deallocate(void* ptr);

private:
	void*           heap_;
	size_t          heap_size_;
	allocator_stats stats_;
};

}
