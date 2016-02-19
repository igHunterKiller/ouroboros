// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oCore/assert.h>
#include <oBase/plan.h>
#include <algorithm>
#include <stdexcept>

using namespace ouro;

struct flushed_tasklist
{
	plan_task* tasks;
	uint32_t num_tasks;
	uint32_t phase;
};

tasklist::~tasklist()
{
	if (tasks || num_tasks)
		throw std::exception("tasklist must be flushed before deinitialized");

	tasks = nullptr;
	num_tasks = 0;
	phase = 0;
	master = nullptr;
}

void tasklist::internal_add(uint32_t priority, uint32_t technique, void* data)
{
	static const uint32_t kMaxSize = tasklist::max_tasks * sizeof(plan_task);

	if (num_tasks == max_tasks)
	{
		master->add(*this); // quietly noops if OOM
		tasks = allocate(kMaxSize);
		num_tasks = 0;
	}

	else if (!tasks)
	{
		tasks = allocate(kMaxSize);
		num_tasks = 0;
	}

	plan_task* t = (plan_task*)tasks + num_tasks++;
	t->priority = priority;
	t->technique = technique;
	t->data = data;
}

void tasklist::flush()
{
	if (num_tasks)
	{
		master->add(*this); // quietly noops if OOM
		num_tasks = 0;
	}

	tasks = nullptr;
}

void* tasklist::allocate(uint32_t bytes, uint32_t align)
{
	return master->plan->allocate(bytes, align);
}

uint32_t master_tasklist::calc_size(uint32_t max_num_tasklists)
{
	return align((uint32_t)sizeof(plan_task) * max_num_tasklists, oCACHE_LINE_SIZE);
}

void master_tasklist::initialize(ouro::plan* p, void* memory, uint32_t max_num_tasklists)
{
	if (!aligned(memory, oCACHE_LINE_SIZE))
		oThrow(std::errc::invalid_argument, "memory must be cacheline size aligned");

	const size_t Bytes = calc_size(max_num_tasklists);
	memset(memory, 0, Bytes);
	plan = p;
	lists = memory;
	num_lists.store(0);
	max_lists = max_num_tasklists;
	overflow = false;
}

void* master_tasklist::deinitialize()
{
	void* p = lists;
	lists = nullptr;
	num_lists.store(0);
	max_lists = 0;
	overflow = false;
	return p;
}

tasklist master_tasklist::internal_make_tasklist(uint32_t phase)
{
	tasklist t;
	t.tasks = nullptr;
	t.num_tasks = 0;
	t.phase = phase;
	t.master = this;
	return t;
}

void master_tasklist::set_plan(ouro::plan* p)
{
	if (num_lists.load())
		oThrow(std::errc::invalid_argument, "cannot change plan while there are outstanding tasks");
	plan = p;
}

void master_tasklist::add(const tasklist& list)
{
	const uint32_t i = num_lists++;
	if (i >= max_lists)
	{
		overflow = true;
		num_lists.store(0);
	}

	flushed_tasklist* l = (flushed_tasklist*)lists + i;
	l->tasks = (plan_task*)list.tasks;
	l->num_tasks = list.num_tasks;
	l->phase = list.phase;
}

uint32_t plan::calc_size(uint32_t allocator_bytes, uint32_t num_phases)
{
	const uint32_t AllocBytes = align(allocator_bytes, oCACHE_LINE_SIZE);
	const uint32_t PhaseTasklistsBytes = align((uint32_t)sizeof(flushed_tasklist) * num_phases, oCACHE_LINE_SIZE);
	return AllocBytes + PhaseTasklistsBytes;
}

void plan::initialize(void* memory, uint32_t bytes, uint32_t num_phases)
{
	if (!aligned(memory, oCACHE_LINE_SIZE))
		oThrow(std::errc::invalid_argument, "memory must be cacheline size aligned");

	const uint32_t PhaseTasklistsBytes = align((uint32_t)sizeof(flushed_tasklist) * num_phases, oCACHE_LINE_SIZE);
	const uint32_t AllocBytes = bytes - PhaseTasklistsBytes;
	memset(memory, 0, bytes);

	allocator.initialize(memory, AllocBytes);
	phase_tasks = (flushed_tasklist*)((uint8_t*)memory + AllocBytes);
	overflow = false;
}

void* plan::deinitialize()
{
	phase_tasks = nullptr;
	overflow = false;
	return allocator.deinitialize();
}

void* plan::allocate(uint32_t size, uint32_t align)
{
	void* p = allocator.allocate(size, align);
	// if no memory, reset allocator and follow through on a non-null pointer so 
	// calling code never receives a nullptr. Consolidation will abort early with
	// an error code in such cases.
	if (!p)
	{
		overflow = true;
		allocator.reset();
		p = allocator.allocate(size, align);
	}

	return p;
}

uint32_t plan::consolidate(master_tasklist* master, const plan_phase* phases, uint32_t num_phases, task_overflow_t on_overflow)
{
start:
	if (overflow || master->overflowed())
	{
		// empty the degenerate plan
		master->reset();
		reset();

		// rebuild something simpler
		if (on_overflow)
		{
			on_overflow();
			// ensure it is simpler
			if (overflow || master->overflowed())
				oThrow(std::errc::invalid_argument, "overflow handler overflowed or did not reset all state");
		}
		else
			oThrow(std::errc::invalid_argument, "plan overflow with no handler");
	}

	// decode opaque pointers
	const flushed_tasklist* FlushedTasklists = (flushed_tasklist*)master->get_lists();
	const flushed_tasklist* EndFlushedTasklists = FlushedTasklists + master->get_num_lists();
	flushed_tasklist* PhaseTasks = (flushed_tasklist*)phase_tasks;
	
	const uint32_t PhaseTasklistsBytes = (uint32_t)sizeof(flushed_tasklist) * num_phases;

	// sum total items per phase
	uint32_t TotalNumItems = 0;
	memset(PhaseTasks, 0, PhaseTasklistsBytes);
	for (const flushed_tasklist* list = FlushedTasklists; list < EndFlushedTasklists; list++)
	{
		PhaseTasks[list->phase].num_tasks += list->num_tasks;
		TotalNumItems += list->num_tasks;
	}

	// prepare allocation sizes per phase
	uint32_t TotalBytes = 0;
	uint32_t* PhaseBytes = (uint32_t*)alloca((uint32_t)sizeof(uint32_t) * num_phases);
	for (uint32_t phase = 0; phase < num_phases; phase++)
	{
		PhaseBytes[phase] = align((uint32_t)sizeof(plan_task) * PhaseTasks[phase].num_tasks, oCACHE_LINE_SIZE);
		TotalBytes += PhaseBytes[phase];
	}

	// allocate space for all phase items
	plan_task* tasks = (plan_task*)allocator.allocate(TotalBytes);
	if (!tasks)
	{
		memset(phase_tasks, 0, PhaseTasklistsBytes);
		overflow = true;
		goto start;
	}
	
	// master up task starts for each phase
	for (uint32_t phase = 0; phase < num_phases; phase++)
	{
		PhaseTasks[phase].tasks = tasks;
		PhaseTasks[phase].phase = phase;
		tasks = (plan_task*)((uint8_t*)tasks + PhaseBytes[phase]);
	}

	// consolidate tasklists into a tasklist per phase
	for (const flushed_tasklist* list = FlushedTasklists; list < EndFlushedTasklists; list++)
		memcpy(PhaseTasks[list->phase].tasks, list->tasks, sizeof(plan_task) * list->num_tasks);

	// sort tasks per phase
	for (uint32_t phase = 0; phase < num_phases; phase++)
	{
		if (phases[phase].traits & plan_phase_traits::sorted)
		{
			auto& master = PhaseTasks[phase];
			std::stable_sort(master.tasks, master.tasks + master.num_tasks, [](const plan_task& a, const plan_task& b)->bool { return a.priority < b.priority; } );
		}
	}

	master->reset();
	return TotalNumItems;
}

void plan::execute(uint32_t phase, const technique_t* techniques)
{
	const flushed_tasklist* PhaseTasks = (const flushed_tasklist*)phase_tasks;
	const plan_task* Task = PhaseTasks[phase].tasks;
	const plan_task* End = Task + PhaseTasks[phase].num_tasks;

	while (Task < End)
	{
		const uint32_t itechnique = Task->technique;
		const plan_task* i = Task + 1;
		while (i < End && i->technique == itechnique)
			i++;
		technique_t technique = techniques[itechnique];
		const uint32_t NumSameItems = static_cast<uint32_t>(i - Task);
		technique(Task, NumSameItems);
		Task = i;
	}
}
