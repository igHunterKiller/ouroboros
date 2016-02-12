// Copyright (c) 2015 Antony Arciuolo. See License.txt regarding use.

// A small block allocator.

// This keeps a fixed block allocator of chunks. On allocation of a size that
// can be accomodated by one of the list of block sizes, a new chunk is turned
// into a fixed block allocator for that size. If that chunk fills, then another
// can be allocated for that same size or another size. If chunks are exhausted
// then smallers sizes can be accomodated in chunks reserved for larger sizes
// until all chunks and blocks are exhausted. As fixed block allocator becomes
// completely free, that chunk is returned for reallocation of other sizes.

#pragma once
#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct sba__ {}* sba_t;

// returns the fixed overhead for the allocator
uint32_t sba_overhead();

// initializes an sba. Its size must accomodate sba_overhead(). All remaining bytes are used to house 
// an fba of chunk_sizes used to allocate slabs/chunks of each fixed size. The number of block sizes 
// must be less than or equal to 16. block_sizes should be sorted in size lowest to highest.
sba_t sba_create(void* arena, uint32_t bytes, uint32_t chunk_size, const uint16_t* block_sizes, uint32_t num_block_sizes);

// deinitializes an sba after this the sba is no longer valid
void sba_destroy(sba_t sba);

// returns the pointer passed as arena in fba_create
void* sba_arena(sba_t sba);

// returns the size specified for the arena in fba_create
uint32_t sba_arena_bytes(sba_t sba);

typedef void (*sba_walker)(void* ptr, size_t size, int used, void* user);
void sba_walk_heap(sba_t sba, sba_walker walker, void* user);

	// This will return nullptr if the size is too large to accomodate, otherwise memory fit for the 
// specified size, though a block larger may be allocated.
void* sba_malloc(sba_t sba, size_t size);
void sba_free(sba_t sba, void* ptr);

#if defined(__cplusplus)
};
#endif
