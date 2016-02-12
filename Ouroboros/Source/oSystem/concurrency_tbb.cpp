// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.
#include <oConcurrency/concurrency.h>
#include <oSystem/process_heap.h>
#include <oSystem/thread_traits.h>
#include <oMemory/allocate.h>
#include <tbb/tbb.h>
#include <tbb/task_scheduler_init.h>

namespace ouro {

class tbb_context
{
public:
	static tbb_context& singleton();

	tbb_context()
	{
		Observer = new observer();
		Init = new ::tbb::task_scheduler_init();
	}

	~tbb_context()
	{
		// This was causing a hang/deadlock so disable the check for now - this only 
		// gets called when the process exits anyway so nothing should be affected.
		//delete Init;
		//delete Observer;
	}

	inline void dispatch(const std::function<void()>& _Task)
	{
		// Use task::enqueue for tasks with no dependency to ensure the main thread
		// never has to participate in TBB threading and prioritizes tasks that are 
		// issued without dependency as these tend to be tasks that are longer running 
		// and behave more like raw threads.
		//
		// task::enqueue vs task::spawn
		// http://software.intel.com/en-us/blogs/2010/05/04/tbb-30-new-today-version-of-intel-threading-building-blocks/
		// The TBB 3.0 schedule supports task::enqueue, which is effectively a “run me 
		// after the other things already pending” request. Although similar to 
		// spawning a task, an enqueued task is scheduled in a different manner. 
		// Enqueued tasks are valuable when approximately first-in first-out behavior 
		// is important, such as in situations where latency of response is more 
		// important than efficient throughput.

		::tbb::task& taskToSpawn = *new(::tbb::task::allocate_root()) task_adapter(_Task);
		::tbb::task::enqueue(taskToSpawn);
	}

private:
	class observer : public ::tbb::task_scheduler_observer
	{
	public:
		observer() { observe(); }
		void on_scheduler_entry(bool is_worker) override { if (is_worker) core_thread_traits::begin_thread("TBB Worker"); }
		void on_scheduler_exit(bool is_worker) override { if (is_worker) core_thread_traits::end_thread(); }
	};

	class task_adapter : public ::tbb::task
	{
	public:
		task_adapter(const std::function<void()>& _task_adapter) : Task(_task_adapter) {}
		task_adapter(std::function<void()>&& _task_adapter) { operator=(std::move(_task_adapter)); }
		task_adapter& operator=(task_adapter&& _That) { if (this != &_That) Task = std::move(_That.Task); return *this; }
		task* execute() { Task(); return nullptr; }
	private:
		std::function<void()> Task;
		task_adapter(const task_adapter&);
		const task_adapter& operator=(const task_adapter&);
	};

	::tbb::task_scheduler_init* Init;
	observer* Observer;
};

template<typename T> T& get_singleton(T*& _pInstance, process_heap::scope _Scope = process_heap::per_process, process_heap::tracking _Tracking = process_heap::garbage_collected)
{
	if (!_pInstance)
	{
		process_heap::find_or_allocate(
			typeid(T).name()
			, process_heap::per_process
			, process_heap::garbage_collected
			, [=](void* _pMemory) { new (_pMemory) T(); }
			, [=](void* _pMemory) { ((T*)_pMemory)->~T(); }
			, &_pInstance);
	}
	return *_pInstance;
}

#define oSINGLETON(_ClassName) _ClassName& _ClassName::singleton() { static _ClassName* sInstance = nullptr; return get_singleton(sInstance); }

oSINGLETON(tbb_context);

class task_group_tbb : public task_group
{
	::tbb::task_group g;
public:
	~task_group_tbb() { wait(); }
	void run(const std::function<void()>& _Task) override { g.run(_Task); }
	void wait() override { g.wait(); }
	void cancel() override { g.cancel(); }
	bool is_canceling() override { return g.is_canceling(); }
};

ouro::task_group* new_task_group()
{
	void* p = default_allocate(sizeof(task_group_tbb), scheduler_name(), memory_alignment::cacheline);
	return p ? new (p) task_group_tbb() : nullptr;
}

void delete_task_group(ouro::task_group* g)
{
	default_deallocate(g);
}

void* commitment_allocate(size_t bytes)
{
	return default_allocate(bytes, scheduler_name(), memory_alignment::cacheline);
}

void commitment_deallocate(void* ptr)
{
	default_deallocate(ptr);
}

const char* scheduler_name()
{
	return "tbb";
}

void ensure_scheduler_initialized()
{
	tbb_context::singleton();
}

void dispatch(const std::function<void()>& _Task)
{
	tbb_context::singleton().dispatch(_Task);
}

void parallel_for(size_t _Begin, size_t _End, const std::function<void(size_t _Index)>& _Task)
{
	::tbb::parallel_for(_Begin, _End, _Task);
}

void at_thread_exit(const std::function<void()>& _Task)
{
	throw std::system_error(std::errc::operation_not_supported, std::system_category());
}

}
