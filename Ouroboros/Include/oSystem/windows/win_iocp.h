// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace ouro { namespace windows { namespace iocp {

// _pContext is specified at the same time a completion_t is. It it responsible
// for its own cleanup. The completion will be called once and not recycled.
// _Size comes from the system. It is often 0 or a use is expected from certain 
// types of handles.
typedef void (*completion_t)(void* _pContext, unsigned long long _Size);

// The number of IO completion threads.
unsigned int concurrency();

void ensure_initialized();

// Retrieve an OVERLAPPED structure configured for an async call using IO 
// completion ports (IOCP). This should be used for all async operations on the
// handle until the handle is closed. The specified completion function and 
// anything it references will live as long as the association does. Once the 
// file is closed call disassociate to free the OVERLAPPED object.
OVERLAPPED* associate(HANDLE _Handle, completion_t _Completion, void* _pContext);
void disassociate(OVERLAPPED* _pOverlapped);

template<typename ContextT>
OVERLAPPED* associate(HANDLE _Handle, ContextT _Completion, void* _pContext) { return associate(_Handle, (completion_t)_Completion, _pContext); }

// Calls the completion routine explicitly. This is useful when operations can
// complete synchronously or asynchronously. If they complete synchronously they
// may not post the overlapped to IOCP for handling so call this in such cases.
void post_completion(OVERLAPPED* _pOverlapped);

// Executes the specified unassociated complextion on a completion port thread.
// This is useful for running an arbitrary function on an IO thread (for example
// CreateFile).
void post(completion_t _Completion, void* _pContext);

// Waits until all associated IO operations have completed
void wait();
bool wait_for(unsigned int _TimeoutMS);

// returns if IO threads are running
bool joinable();

// waits for all associated IO operations to complete then joins all IO threads
void join();

}}}
