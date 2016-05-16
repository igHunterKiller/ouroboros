// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oSystem/windows/win_crt_leak_tracker.h>
#include <oSystem/debugger.h>
#include <oConcurrency/mutex.h>
#include <oSystem/process_heap.h>
#include <oSystem/reporting.h>
#include <oSystem/windows/win_crt_heap.h>

namespace ouro { namespace windows { namespace crt_leak_tracker {

static bool& thread_local_tracking_enabled()
{
	// has to be a pointer so for multi-module support (all instances of this from
	// a DLL perspective must point to the same bool value)
	thread_local static bool* enabled = nullptr;
	if (!enabled)
	{
		process_heap::find_or_allocate(
			"thread_local_tracking_enabled"
			, process_heap::per_thread
			, process_heap::none
			, [=](void* _pMemory) { *(bool*)_pMemory = true; }
			, nullptr
			, &enabled);
	}

	return *enabled;
}

class context : public leak_tracker
{
public:
	static context& singleton();

	void enable(bool enable);
	inline bool enabled() const { return enabled_; }

	inline void enable_report(bool enable) { report_enabled_ = enable; }
	inline bool enable_report() { return report_enabled_; }

	bool report(bool current_context_only);

	inline void ignore(void* ptr) 
	{
		allocation_stats s;
		s.pointer = ptr;
		s.ordinal = windows::crt_heap::allocation_id(ptr);
		s.operation = memory_operation::deallocate;
		on_stat_ordinal(s);
	}
	
	int on_malloc_event(int alloc_type, void* user_data, size_t size, int block_type, long request_number, const unsigned char* path, int line);

protected:
	context();
	~context();

	static int malloc_hook(int alloc_type, void* user_data, size_t size, int block_type, long request_number, const unsigned char* path, int line);
	
	size_t nonlinear_bytes_; // todo: make this track memory that was allocated and not recorded.
	_CRT_ALLOC_HOOK orig_alloc_hook_;
	bool enabled_;
	bool report_enabled_;
};

context::context()
	: nonlinear_bytes_(0)
	, orig_alloc_hook_(nullptr)
	, enabled_(false)
	, report_enabled_(false)
{
	ouro::reporting::ensure_initialized();

	leak_tracker::init_t i;
	i.thread_local_tracking_enabled = thread_local_tracking_enabled;
	i.callstack = debugger::callstack;
	i.format = debugger::format;
	i.print = debugger::print;

	static const uint32_t kNumAllocs = 200000;
	auto req = leak_tracker::calc_size(kNumAllocs);
	void* mem = process_heap::allocate(req);
	initialize(i, mem, kNumAllocs);
}

context::~context()
{
	report(false);
	_CrtSetAllocHook(orig_alloc_hook_);

	if (nonlinear_bytes_)
	{
		mstring buf;
		format_bytes(buf, nonlinear_bytes_, 2);
		oTrace("CRT leak tracker: Allocated %s beyond the internal reserve. Increase kTrackingInternalReserve to improve performance, especially on shutdown.", buf.c_str());
	}

	process_heap::deallocate(deinitialize());
}

context& context::singleton()
{
	static context* sInstance = nullptr;
	if (!sInstance)
	{
		process_heap::find_or_allocate(
			"crt_leak_tracker context"
			, process_heap::per_process
			, process_heap::none
			, [=](void* mem) { new (mem) context(); }
			, [=](void* mem) { ((context*)mem)->~context(); }
			, &sInstance);
	}
	return *sInstance;
}

void context::enable(bool enable)
{
	if (enable && !enabled_)
		orig_alloc_hook_ = _CrtSetAllocHook(malloc_hook);
	else if (!enable && enabled_)
	{
		_CrtSetAllocHook(orig_alloc_hook_);
		orig_alloc_hook_ = nullptr;
	}

	enabled_ = enable;
}

bool context::report(bool current_context_only)
{
	size_t nleaks = 0;
	if (report_enabled_)
	{
		bool prior = enabled();
		enable(false);
		nleaks = leak_tracker::report(current_context_only);
		enable(prior);
	}

	return nleaks > 0;
}

int context::on_malloc_event(int alloc_type, void* user_data, size_t size
	, int block_type, long request_number, const unsigned char* path, int line)
{
	int allowAllocationToProceed = 1;

	// Call any prior hook first
	if (orig_alloc_hook_)
		allowAllocationToProceed = orig_alloc_hook_(alloc_type, user_data, size, block_type, request_number, path, line);

	if (allowAllocationToProceed && block_type != _IGNORE_BLOCK && block_type != _CRT_BLOCK)
	{
		allocation_stats s;
		s.pointer = user_data;
		s.label = "crt";
		s.size = size;
		//s.options;
		s.frame = 0;

		uint32_t old_id = uint32_t(-1);

		switch (alloc_type)
		{
			case _HOOK_ALLOC:
				s.ordinal = uint32_t(request_number);
				s.operation = memory_operation::allocate;
				break;
			case _HOOK_REALLOC:
				s.ordinal = uint32_t(request_number);
				s.operation = memory_operation::reallocate;
				old_id = windows::crt_heap::allocation_id(user_data);
				break;
			case _HOOK_FREE:
				s.ordinal = windows::crt_heap::allocation_id(user_data);
				s.operation = memory_operation::deallocate;
				break;
			default: oThrow(std::errc::invalid_argument, "unexpected alloc type %d", alloc_type);
		}

		on_stat_ordinal(s, old_id);
	}

	return allowAllocationToProceed;
}

int context::malloc_hook(int alloc_type, void* user_data, size_t size, int block_type, long request_number, const unsigned char* path, int line)
{
	return context::singleton().on_malloc_event(alloc_type, user_data, size, block_type, request_number, path, line);
}

void ensure_initialized()
{
	context::singleton();
}
 
void enable(bool enable)
{
	context::singleton().enable(enable);
}
 
bool enabled()
{
	return context::singleton().enabled();
}

void enable_report(bool enable)
{
	context::singleton().enable_report(enable);
}

bool enable_report()
{
	return context::singleton().enable_report();
}

void new_context()
{
	context::singleton().new_context();
}

void capture_callstack(bool capture)
{
	context::singleton().capture_callstack(capture);
}

bool capture_callstack()
{
	return context::singleton().capture_callstack();
}

void thread_local_tracking(bool enable)
{
	context::singleton().thread_local_tracking(enable);
}

bool thread_local_tracking()
{
	return context::singleton().thread_local_tracking();
}

bool report(bool current_context_only)
{
	return context::singleton().report(current_context_only);
}

void reset()
{
	context::singleton().reset();
}

void ignore(void* _Pointer)
{
	context::singleton().ignore(_Pointer);
}

void add_delay()
{
	context::singleton().add_delay();
}

void release_delay()
{
	context::singleton().release_delay();
}

}}}
