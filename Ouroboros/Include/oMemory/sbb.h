// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

/* Segregated Binary-Buddy memory allocator.
** This is an implementation of a binary-buddy memory allocator that keeps 
** its bookkeeping separate from the heap itself so that write-combined 
** memory can be managed. To keep the bookkeeping small this uses a compact 
** binary tree comprised of 1 bit per allocation block at any level rather 
** than a linked-list freelist node. O(log n) allocation/free time.
** Per-sbb overhead: 32-bit: 24-bytes + pages; 64-bit: 32-bytes + pages.
*/

#pragma once
#include <stddef.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct sbb__ {}* sbb_t;

/* returns required bytes for sbb_create()'s bookkeeping memory */
size_t sbb_bookkeeping_size(size_t arena_bytes, size_t min_block_size);

/* arena can be NULL. Both bytes and min_block_size must be a power of two */
sbb_t sbb_create(void* arena, size_t bytes, size_t min_block_size, void* bookkeeping);
void sbb_destroy(sbb_t sbb);

/* bytes will be rounded to the next power of two */
void* sbb_malloc(sbb_t sbb, size_t bytes);
void* sbb_memalign(sbb_t sbb, size_t align, size_t bytes);
void* sbb_realloc(sbb_t sbb, void* ptr, size_t bytes);
void sbb_free(sbb_t sbb, void* ptr);

/* returns internal block size, not original request size. O(log n) */
size_t sbb_block_size(sbb_t sbb, void* ptr);

/* returns values passed to sbb_create() */
void* sbb_arena(sbb_t sbb);
void* sbb_bookkeeping(sbb_t sbb);
size_t sbb_arena_bytes(sbb_t sbb);
size_t sbb_min_block_size(sbb_t sbb);

/* returns allocator-wide info */
size_t sbb_max_free_block_size(sbb_t sbb);
size_t sbb_num_free_blocks(sbb_t sbb); /* (fragmentation hint) */
size_t sbb_overhead(sbb_t sbb);

/* debugging */
typedef void (*sbb_walker)(void* ptr, size_t size, int used, void* user);
void sbb_walk_heap(sbb_t sbb, sbb_walker walker, void* user);
bool sbb_check_heap(sbb_t sbb);

#if defined(__cplusplus)
};
#endif
