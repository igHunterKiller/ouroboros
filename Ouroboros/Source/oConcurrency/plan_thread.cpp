// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oConcurrency/plan_thread.h>
#include <oConcurrency/backoff.h>
#include <oSystem/thread_traits.h>
#include <algorithm>

#include <oCore/assert.h>

using namespace ouro;

plan_thread::plan_thread()
	: dealloc(nullptr)
	, phases(nullptr)
	, techniques(nullptr)
	, consumer_name(nullptr)
	, num_phases(0)
	, num_techniques(0)
	, producer_plan(nullptr)
	, consumer_plan(nullptr)
	, state(uninitialized)
{
}

void plan_thread::initialize(const init_t& init, const allocator& a)
{
	// calculate how much memory will be required by components
	const uint32_t TasklistSetBytes = master_tasklist::calc_size(init.max_num_tasklists);
	const uint32_t PlanBytes = plan::calc_size(init.plan_allocator_bytes, init.num_phases);
	const uint32_t TotalBytes = TasklistSetBytes + 2*PlanBytes + 2*sizeof(plan);

	// allocate and offset the memory
	void* l = a.allocate(TotalBytes, "plan_thread memory");
	void* r = (uint8_t*)l + TasklistSetBytes;
	void* w = (uint8_t*)r + PlanBytes;

	producer_plan = (plan*)((uint8_t*)w + PlanBytes);
	consumer_plan = (plan*)((uint8_t*)producer_plan + sizeof(plan));
	memset(l, 0, TotalBytes);

	// initialize components
	producer_plan->initialize(w, PlanBytes, init.num_phases);
	consumer_plan->initialize(r, PlanBytes, init.num_phases);
	tasklists.initialize(producer_plan, l, init.max_num_tasklists);

	// initialize fields
	dealloc = a.deallocator();
	phases = init.phases;
	techniques = init.techniques;
	num_phases = init.num_phases;
	num_techniques = init.num_techniques;

	state = initializing;
	init_debug_name = init.debug_name ? init.debug_name : "plan_thread";

	// start up work thread and wait for it to be ready
	consumer = std::move(std::thread(std::bind(&plan_thread::run, this)));
	backoff bo;
	while (state == initializing)
		bo.pause();
}

void plan_thread::deinitialize()
{
	if (!phases) // already deinitialized
		return;

	// stop the work thread
	{
		std::unique_lock<std::mutex> lock(mtx);
		state = exiting;
	}
	cv_consumer.notify_all();
	consumer.join();

	if (state != uninitialized)
		throw std::invalid_argument("consumer thread failed to shut down");

	// tear down compoenents
	void* p = tasklists.deinitialize();
	consumer_plan->deinitialize();
	producer_plan->deinitialize();

	// free memory
	if (dealloc)
		dealloc(p);

	dealloc = nullptr;
	phases = nullptr;
	techniques = nullptr;
	num_phases = 0;
	num_techniques = 0;
	producer_plan = nullptr;
	consumer_plan = nullptr;
}

void plan_thread::kick()
{
	uint32_t n = producer_plan->consolidate(&tasklists, phases, num_phases, nullptr);

	// wait for the worker thread to be done
	std::unique_lock<std::mutex> lock(mtx);
	while (state > idle)
		cv_producer.wait(lock);

	state = swapping;
	std::swap(consumer_plan, producer_plan);

	// if there's work, fire up the work thread
	if (n)
	{
		state = executing;
		lock.unlock();
		cv_consumer.notify_all();
	}
	else
		state = idle;

	// reset the consumed plan and attach it to the master worklist
	tasklists.set_plan(producer_plan);
	producer_plan->reset();
}

void plan_thread::sync()
{
	std::unique_lock<std::mutex> lock(mtx);
	while (state > idle)
		cv_producer.wait(lock);
}

void plan_thread::run()
{
	core_thread_traits::begin_thread(init_debug_name);
	init_debug_name = nullptr;

	begin_thread();
	state = idle;
	while (1)
	{
		// wait for frame swapping to be finished
		std::unique_lock<std::mutex> lock(mtx);
		
		while (state < executing)
			cv_consumer.wait(lock);
		if (state == exiting)
			break;

		for (uint32_t phase = 0; phase < num_phases; phase++)
		{
			begin_phase(phases[phase].name);
			consumer_plan->execute(phase, techniques);
			end_phase(phases[phase].name);
		}

		// frame is done: release the lock so frames can be flipped and notify the 
		// producer
		state = idle;
		lock.unlock();
		cv_producer.notify_all();
	}

	state = uninitialized;
	end_thread();
	core_thread_traits::end_thread();
}
