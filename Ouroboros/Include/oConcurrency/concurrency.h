// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Concurrent interfaces. These are not implemented in oBase but are used by 
// oBase implementations since concurrency is such a big performance win and 
// should be provided by the C++1x standard. Until then, client code must 
// implement these interfaces.

#pragma once
#include <functional>
#include <memory>

namespace ouro {

// This function should be implemented for queue types since some "queues" have 
// valid reasons for being LIFOs.
template<typename T> struct is_fifo : std::true_type {};

// defines a subset of functions that can be waited on.
class task_group
{
public:
	virtual ~task_group() {}

	// dispatches a task flagged as part of this group
	virtual void run(const std::function<void()>& task) = 0;

	// waits for only tasks dispatched with run() to finish
	virtual void wait() = 0;

	// cancels all queued tasks (executed tasks will have already been completed). 
	// This only flags tasks not to execution, but the threadpool still needs to 
	// process them before wait() will be unblocked to guarantee any resources 
	// bound in the run task are released.
	virtual void cancel() = 0;

	// returns true if cancel was called, but wait has not yet finished (or has 
	// not yet been called)
	virtual bool is_canceling() = 0;
};

// create/destroy a task group
task_group* new_task_group();
void delete_task_group(task_group* g);

// general-purpose allocator mainly used to allocate commitments themselves,
// which because they're templated on the storage type can be any size.
void* commitment_allocate(size_t bytes);
void commitment_deallocate(void* ptr);

// Returns the name of the current job scheduler
const char* scheduler_name();

// Ensures all initial allocations are done and the system is ready.
void ensure_scheduler_initialized();

// issues a task for asynchronous execution on any thread in the underlying
// threadpool.
void dispatch(const std::function<void()>& task);

// Implements the parallel for pattern, executing the specified task with the index
// from begin to end.
void parallel_for(size_t begin, size_t end, const std::function<void(size_t index)>& task);

// For debugging
inline void serial_for(size_t begin, size_t end, const std::function<void(size_t index)>& task)
{
	for (size_t i = 0; i < end; i++)
		task(i);
}

// Runs a task when the calling thread exits.
void at_thread_exit(const std::function<void()>& task);

}
