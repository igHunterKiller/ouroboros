// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace ouro { namespace windows { namespace iocp {

// 'context' is specified at the same time a completion_fn is. It it responsible
// for its own cleanup. The completion will be called once and not recycled.
// 'size' comes from the system. It is often 0 or a use is expected from certain 
// types of handles.
typedef void (*completion_fn)(void* context, unsigned long long size);

// The number of IO completion threads.
unsigned int concurrency();

void ensure_initialized();

// Retrieve an OVERLAPPED structure configured for an async call using IO 
// completion ports (IOCP). This should be used for all async operations on the
// handle until the handle is closed. The specified completion function and 
// anything it references will live as long as the association does. Once the 
// file is closed call disassociate to free the OVERLAPPED object.
OVERLAPPED* associate(HANDLE handle, completion_fn completion, void* context);
void disassociate(OVERLAPPED* overlapped);

template<typename ContextT>
OVERLAPPED* associate(HANDLE handle, ContextT completion, void* context) { return associate(handle, (completion_fn)completion, context); }

// Calls the completion routine explicitly. This is useful when operations can
// complete synchronously or asynchronously. If they complete synchronously they
// may not post the overlapped to IOCP for handling so call this in such cases.
void post_completion(OVERLAPPED* overlapped);

// Executes the specified unassociated complextion on a completion port thread.
// This is useful for running an arbitrary function on an IO thread (for example
// CreateFile).
void post(completion_fn completion, void* context);

// Waits until all associated IO operations have completed
void wait();
bool wait_for(unsigned int timeout_ms);

// returns if IO threads are running
bool joinable();

// waits for all associated IO operations to complete then joins all IO threads
void join();

}}}
