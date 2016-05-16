// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// The plan system defines a backbone of execution for user-specified functions.
// The pattern is to have several threads submit work to thread-local worklists
// which will then be gathered/reduced into a single worklist that then executes
// in order. The master worklist is broken up into discrete phases that always
// execute in the same order. Within a phase, tasks are sorted by priority.

// To set up a plan:
// 1. Statically define an array of plan_phases.
// 2. Statically define an array of techniques used to operate on tasks.
// 3. Define a plan in a place accessible to main app logic.
// 4. Define a master_tasklist in a place accessible to main app logic.
// 5. Define a tasklist per phase per thread. These are very lightweight and 
//    are designed to have app logic add tasks to them in a multi-threaded 
//    manner. When the max_task threshold is reached, the tasklist is flushed
//    to the master_tasklist and a new one is readied to continue to be filled. 
//    Use the allocator on it to allocate any per-plan execution memory, such as 
//    the data passed with a technique. This is transient memory just for the 
//    scope of a plan's execution.
// 6. When finished with adding app logic tasks, flush all tasklists.
// 7. Consolidate the master_tasklist into a plan and then execute the plan.
// 8. reset the master_tasklist and plan to begin the next cycle of main app logic.
//
// There is also a plan_thread that will double-buffer plans so one can be built
// while another one is executing. Execution happens in a separate thread. See
// plan_thread.h for more details.

#pragma once
#include <oArch/arch.h>
#include <oArch/compiler.h>
#include <oMemory/concurrent_linear_allocator.h>
#include <atomic>

namespace ouro {

class plan;
class tasklist;
class master_tasklist;

// _____________________________________________________________________________
// Attributes for each phase of a plan

namespace plan_phase_traits
{	enum value : uint32_t {

	sorted = 1<<0,

};}

// _____________________________________________________________________________
// a phase of a plan mostly defines bins of tasks that always occur in the same
// order. Tasks may vary in order within a phase, but phase order is static.

struct plan_phase
{
	const char* name;
	uint32_t traits; // plan_phase_traits
};

// _____________________________________________________________________________
// A task is a technique and the data it operates on. Data should almost always
// be allocated from tasklist::allocate() so the lifetime of the memory is the
// same as the execution of the plan, but does not persist beyond that. Tasks
// can be sorted in ascending priority order. The technique field is an index
// into the statically defined array of techniques.

struct plan_task
{
	uint32_t priority;
	uint32_t technique;
	void* data;
};

// _____________________________________________________________________________
// A technique operates on a list of tasks.

typedef void (*technique_t)(const plan_task* tasks, uint32_t num_tasks);

// _____________________________________________________________________________
// This is called when there are more tasks than reserved in a master task list
// or if the consolidated list cannot be allocated.

typedef void (*task_overflow_t)();

// _____________________________________________________________________________
// A very lightweight facade for adding tasks from a multi-threaded source to
// a master_tasklist.

class tasklist
{
public:
	static const uint32_t default_alignment = oDEFAULT_MEMORY_ALIGNMENT;
	static const uint32_t max_tasks = 1024;

	tasklist() : tasks(nullptr), num_tasks(0), phase(0), master(nullptr) {}
	~tasklist();

	template<typename technique_enum>
	void add(uint32_t priority, const technique_enum& technique, void* data) { internal_add(priority, (uint32_t)technique, data); }

	void flush();

	inline void* allocate(uint32_t bytes, uint32_t align = default_alignment);
	
	template<typename T>
	T* allocate() { return (T*)allocate(sizeof(T)); }

private:
	friend class master_tasklist;
	void internal_add(uint32_t priority, uint32_t technique, void* data);

	void* tasks;
	uint32_t num_tasks;
	uint32_t phase;
	master_tasklist* master;
};

// _____________________________________________________________________________
// A set of taskslists that are collected as stand-alone tasklists are flushed.
// This setup minimizes contention on atomics associated with allocate for data
// and tasklist backing.

class alignas(oCACHE_LINE_SIZE) master_tasklist
{
public:
	static uint32_t calc_size(uint32_t tasklist_capacity);

	master_tasklist() : plan(nullptr), lists(nullptr), max_lists(0) { num_lists.store(0); }
	~master_tasklist() { deinitialize(); }

	void initialize(plan* p, void* memory, uint32_t max_num_tasklists);
	void* deinitialize();

	// creates a new tasklist that is fit for efficient threadsafe submission to
	// the master_tasklist.
	template<typename phase_enum>
	tasklist make_tasklist(const phase_enum& phase) { return internal_make_tasklist((uint32_t)phase); }

	void set_plan(plan* p);
	inline bool overflowed() const { return overflow; }

protected:
	friend class plan;
	friend class tasklist;
	friend class master_tasklist;

	void add(const tasklist& list);

	friend class master_tasklist;
	void* get_lists() const { return lists; }
	uint32_t get_num_lists() const { return num_lists.load(); }
	inline void reset() { num_lists.store(0); overflow = false; }

private:
	tasklist internal_make_tasklist(uint32_t phase);

	void* lists;
	plan* plan;
	std::atomic_uint num_lists;
	uint32_t max_lists;
	bool overflow;
};

// _____________________________________________________________________________
// The plan orders tasks by their priority and phase from a master_tasklist and 
// then can execute it phase-by-phase.

class alignas(oCACHE_LINE_SIZE) plan
{
public:
	static const uint32_t default_alignment = oDEFAULT_MEMORY_ALIGNMENT;

	static uint32_t calc_size(uint32_t allocator_bytes, uint32_t num_phases);
	
	plan() : phase_tasks(nullptr), overflow(false) {}
	~plan() { deinitialize(); }

	// memory must be oCACHE_LINE_SIZE-aligned. Bytes is the value returned from
	// calc_size.
	void initialize(void* memory, uint32_t bytes, uint32_t num_phases);
	void* deinitialize();

	void* allocate(uint32_t size, uint32_t align = default_alignment);
	inline void reset() { allocator.reset(); }

	// combines the various tasklists into phases and sorts each phase, then 
	// resets the master tasklist. If either the master overflowed or the 
	// allocator overflow both the plan and master tasklist are reset then 
	// on_overflow is called. The recommended course of action is to create a new, 
	// lightweight plan indicating overflow. This returns the total number of 
	// tasks in the plan.
	uint32_t consolidate(master_tasklist* master, const plan_phase* phases, uint32_t num_phases, task_overflow_t on_overflow);

	// runs the technique on all tasks in the consolidated list
	void execute(uint32_t phase, const technique_t* techniques);

private:
	concurrent_linear_allocator allocator;
	void* phase_tasks;
	bool overflow;
};

}
