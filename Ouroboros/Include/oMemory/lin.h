// Copyright (c) 2015 Antony Arciuolo. See License.txt regarding use.

/* O(1) linear allocator: cannot free but is extremely quick to allocate */

#pragma once
#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct lin__ {}* lin_t;

/* initialize/deinitialize the linear allocator */
lin_t lin_create(void* arena, size_t bytes);
void lin_destroy(lin_t lin);

/* memory cannot be reclaimed individually, free_all() frees all allocations */
void* lin_malloc(lin_t lin, size_t bytes);
void* lin_memalign(lin_t lin, size_t align, size_t bytes);
void lin_free_all(lin_t lin);

/* returns values passed to sbb_create() */
void* lin_arena(lin_t lin);
size_t lin_arena_bytes(lin_t lin);

/* returns allocator-wide info */
size_t lin_overhead();
size_t lin_used_bytes(lin_t lin);
size_t lin_free_bytes(lin_t lin);
size_t lin_max_bytes(lin_t lin);

/* return base of allocations, useful when accumulating a buffer and using it directly */
void* lin_base(lin_t lin);

#if defined(__cplusplus)
};
#endif
