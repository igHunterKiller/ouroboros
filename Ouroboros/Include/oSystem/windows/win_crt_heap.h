// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// abstraction for working with Microsoft's default CRT allocator's robust debugging

#pragma once
#include <stddef.h>

namespace ouro { namespace windows { namespace crt_heap {

void*        get_pointer(struct _CrtMemBlockHeader* hdr);    // return pointer as stored in the specified block
void*        next_pointer          (void* ptr);              // get the 'next' pointer as stored in CRT bookkeeping - walk through all pointers from a _CrtMemState until now
bool         is_valid              (void* ptr);              // true if from the MSVCRT heap, false if not something free() can handle (offset pointer)
size_t       size                  (void* ptr);              // user size
bool         is_free               (void* ptr);              // special debug mode where ptr is free-but-still-allocated to prevent reuse
bool         is_normal             (void* ptr);              // true if a typical allocation (not a special debug mode)
bool         is_crt                (void* ptr);              // true if an internal CRT allocation
bool         is_ignore             (void* ptr);              // true if alloc has been marked ignore-by-tracking (never reported as a leak)
bool         is_client             (void* ptr);              // true if flagged as a client allocation
const char*  allocation_filename   (void* ptr);              // returns __FILENAME__ for where the alloc originated
unsigned int allocation_line       (void* ptr);              // returns __LINE__ where the alloc originated
unsigned int allocation_id         (void* ptr);              // returns ordinal for the allocation
void         break_on_allocation   (unsigned int alloc_id);  // breaks when an alloc with this ordinal occurs
void         report_leaks_on_exit  (bool enable = true);     // use default reporting for leaks on program exit
bool         enable_memory_tracking(bool enable = true);     // if false alloc in IGNORE blocks to avoid leak reporting, especially from 3rd-party allocs like TBB

}}}
