// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// This double-buffers a plan and executes it in a different thread.

#pragma once
#include <oBase/plan.h>
#include <oMemory/allocate.h>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace ouro {

class tasklist;
class plan_thread
{
public:
	static const uint32_t default_alignment = plan::default_alignment;

	struct init_t
	{
		init_t()
			: debug_name("plan_thread")
			, phases(nullptr)
			, num_phases(0)
			, techniques(nullptr)
			, num_techniques(0)
			, max_num_tasklists(100)
			, plan_allocator_bytes(32)
		{}

		// Name of the thread as it will appear in the debugger
		const char* debug_name;

		// Pointer to a static const array of render phases for which bookkeeping
		// will be set up
		const plan_phase* phases;
		
		// Number of phases specified above
		uint32_t num_phases;

		// Pointer to a static const array of techniques that will be used when 
		// elements are submitted
		const technique_t* techniques;

		// Number of techniques specified above
		uint32_t num_techniques;

		// The maximum number of tasklists that can be flushed to the master_tasklist
		uint32_t max_num_tasklists;
		
		// This number of bytes will be allocated for each plan being assembled. There
		// are two: one being built and one being worked on (double-buffered) so this
		// size is allocated twice.
		uint32_t plan_allocator_bytes;
	};

	// --- Producer Thread API ---

	plan_thread();
	~plan_thread() { deinitialize(); }

	void initialize(const init_t& init, const allocator& a);
	void deinitialize();

	// This object contains a master_list, so expose the same factory api to make
	// tasklists.
	template<typename phase_enum>
	tasklist make_tasklist(const phase_enum& phase) { return tasklists.make_tasklist(phase); }

	// Draws from the same heap as a tasklist's allocate(): the current producer-
	// side plan's allocate()
	inline void* allocate(uint32_t bytes, uint32_t align = default_alignment) { return producer_plan->allocate(bytes, align); }
	
	template<typename T>
	T* allocate(uint32_t bytes = sizeof(T), uint32_t align = default_alignment) { return (T*)allocate(bytes, align); }

	// call this after all tasklists have been flushed to kick off execution of 
	// the plan. It is safe to immediately begin rebuilding a plan.
	void kick();

	// blocks until the plan thread is idle
	void sync();

protected:
	// --- Consumer Thread API ---

	// these are called before and after each phase is executed, primarily intended
	// to attach to some debugging/profiling system
	virtual void begin_thread() {}
	virtual void end_thread() {}

	virtual void begin_phase(const char* phase_name) {}
	virtual void end_phase(const char* phase_name) {}

private:
	enum state_t
	{
		uninitialized,
		initializing,
		finished,
		swapping,
		idle,
		executing,
		exiting,
	};

	void run();

	master_tasklist tasklists;

	// static config items
	deallocate_fn dealloc;
	const char* init_debug_name; // only valid during init
	const plan_phase* phases;
	const technique_t* techniques;
	const char* consumer_name;
	uint32_t num_phases;
	uint32_t num_techniques;

	// double-buffered plans
	plan* producer_plan;
	plan* consumer_plan;

	// thread sync
	std::mutex mtx;
	std::condition_variable cv_producer;
	std::condition_variable cv_consumer;
	std::thread consumer;
	state_t state;
};

}
