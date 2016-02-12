// Copyright (c) 2015 Antony Arciuolo. See License.txt regarding use.

/* O(1) block allocator. It can also be used as an index allocator if 
** the block size is sizeof(uint32_t).
*/

#pragma once
#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct fba__ {}* fba_t;

/* returns bytes to manage num_blocks of block_size + overhead */
uint32_t fba_calc_arena_bytes(uint32_t num_blocks, uint32_t block_size);

/* arena_bytes must include fba_overhead(). block_size must be at least sizeof(uint32_t) */
fba_t fba_create(void* arena, uint32_t bytes, uint32_t block_size);
void fba_destroy(fba_t fba);

/* allocs are always block size */
void* fba_malloc(fba_t fba);
void fba_free(fba_t fba, void* ptr);
uint32_t fba_malloc_index(fba_t fba);
void fba_free_index(fba_t fba, uint32_t index);

/* convert between allocated indices and pointers */
uint32_t fba_index(fba_t fba, void* ptr);
void* fba_pointer(fba_t fba, uint32_t index);

/* returns values passed to sbb_create() */
void* fba_arena(fba_t fba);
uint32_t fba_arena_bytes(fba_t fba);
uint32_t fba_block_size(fba_t fba);

/* returns allocator-wide info */
uint32_t fba_overhead();
uint32_t fba_max_num_blocks(fba_t fba);
uint32_t fba_num_free_blocks(fba_t fba);

/* debugging */
typedef void (*fba_walker)(void* ptr, size_t size, int used, void* user);
void fba_walk_heap(fba_t fba, fba_walker walker, void* user);

#if defined(__cplusplus)
};
#endif
