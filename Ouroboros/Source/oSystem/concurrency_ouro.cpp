// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oCore/assert.h>
#include <oConcurrency/concurrency.h>
#include <oConcurrency/threadpool.h>
#include <oSystem/process_heap.h>
#include <oSystem/thread_traits.h>
#include <oMemory/allocate.h>

namespace ouro {

class ouro_context
{
public:
	typedef process_heap::std_allocator<std::function<void()>> allocator_type;
	typedef threadpool<core_thread_traits, allocator_type> threadpool_type;
	typedef detail::task_group<core_thread_traits, allocator_type> task_group_type;

	static ouro_context& singleton();
	ouro_context() {}
	~ouro_context() { tp.join(); }
	inline threadpool_type& get_threadpool() { return tp; }
	inline void dispatch(const std::function<void()>& _Task) { tp.dispatch(_Task); }
	inline void parallel_for(size_t _Begin, size_t _End, const std::function<void(size_t _Index)>& _Task) { ouro::detail::parallel_for<16>(tp, _Begin, _End, _Task); }
private:
	threadpool_type tp;
};

ouro_context& ouro_context::singleton()
{
	static ouro_context* sInstance = nullptr;
	if (!sInstance)
	{
		process_heap::find_or_allocate(
			"ouro_context"
			, process_heap::per_process
			, process_heap::leak_tracked
			, [=](void* _pMemory) { new (_pMemory) ouro_context(); }
			, [=](void* _pMemory) { ((ouro_context*)_pMemory)->~ouro_context(); }
			, &sInstance);
	}
	return *sInstance;
}

class task_group_ouro : public task_group
{
	ouro_context::task_group_type g;
public:
	task_group_ouro() : g(ouro_context::singleton().get_threadpool()) {}
	~task_group_ouro() { wait(); }
	void run(const std::function<void()>& _Task) override { g.run(_Task); }
	void wait() override { g.wait(); }
	void cancel() override { g.cancel(); }
	bool is_canceling() override { return g.is_canceling(); }
};

ouro::task_group* new_task_group()
{
	void* p = default_allocate(sizeof(task_group_ouro), scheduler_name(), memory_alignment::cacheline);
	return p ? new (p) task_group_ouro() : nullptr;
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
	return "ouro";
}

void ensure_scheduler_initialized()
{
	ouro_context::singleton();
}

void dispatch(const std::function<void()>& _Task)
{
	ouro_context::singleton().dispatch(_Task);
}

void parallel_for(size_t _Begin, size_t _End, const std::function<void(size_t _Index)>& _Task)
{
	ouro_context::singleton().parallel_for(_Begin, _End, _Task);
}

void at_thread_exit(const std::function<void()>& _Task)
{
	oThrow(std::errc::operation_not_supported, "");
}

}
