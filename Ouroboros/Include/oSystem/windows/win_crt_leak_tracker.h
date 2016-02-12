// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Tracks CRT allocations. Mainly this is glue code linking oBase/leak_tracker
// to libcrt malloc.

#pragma once
#include <oBase/leak_tracker.h>

namespace ouro { namespace windows { namespace crt_leak_tracker {

// This ensures the crt_leak_tracker has been initialized
void ensure_initialized();

void enable(bool enable);
bool enabled();

void enable_report(bool enable);
bool enable_report();

// All outstanding allocations are cleared as if they never happened. This is
// useful in unit test situations where even though a test leaked, leaks should 
// not be reported for subsequent tests.
void new_context();

// For each allocation the callstack leading to malloc is recorded. This is very
// slow, but can easily identify the source of memory leaks.
void capture_callstack(bool capture);
bool capture_callstack();

void thread_local_tracking(bool enable);
bool thread_local_tracking();

// Returns true if there were leaks or false if there were none.
bool report(bool current_context_only = true);

// Reset all tracking bookkeeping
void reset();

// In rare low-level systems that need to persist after leaks have been 
// reported it is helpful not to report those allocations as a leak. For example 
// a log file that is going to retain the leak report itself should not be 
// reported as a leak.
void ignore(void* ptr);

// See oBase/leak_tracker for more details
void add_delay();
void release_delay();

}}}
