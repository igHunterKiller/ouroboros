// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oArch/compiler.h>
#include <oSystem/windows/win_com.h>
#include <oSystem/windows/win_error.h>
#include <oSystem/process_heap.h>

#define WIN32_LEAN_AND_MEAN
#include <ObjBase.h>

namespace ouro { namespace windows { namespace com {

class context
{
public:
	context()
		: call_uninit_(false)
	{
		call_uninit_ = SUCCEEDED(CoInitializeEx(nullptr, COINIT_MULTITHREADED|COINIT_SPEED_OVER_MEMORY));
		if (!call_uninit_)
			throw error();
	}

	~context() { if (call_uninit_) CoUninitialize(); }

private:
	bool call_uninit_;
};
			
void ensure_initialized()
{
	static thread_local context* s_instance = nullptr;
	if (!s_instance)
	{
		process_heap::find_or_allocate(
			"com::context"
			, process_heap::per_thread
			, process_heap::garbage_collected
			, [=](void* memory) { new (memory) context(); }
			, nullptr
			, &s_instance);
	}
}
		
}}}
