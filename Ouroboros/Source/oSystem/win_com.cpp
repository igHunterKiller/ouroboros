// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oArch/compiler.h>
#include <oSystem/windows/win_com.h>
#include <oSystem/windows/win_error.h>
#include <oSystem/process_heap.h>

#define WIN32_LEAN_AND_MEAN
#include <ObjBase.h>

namespace ouro {
	namespace windows {
		namespace com {

class context
{
public:
	context()
		: CallUninit(false)
	{
		CallUninit = SUCCEEDED(CoInitializeEx(nullptr, COINIT_MULTITHREADED|COINIT_SPEED_OVER_MEMORY));
		if (!CallUninit)
			throw error();
	}

	~context() { if (CallUninit) CoUninitialize(); }

private:
	bool CallUninit;
};
			
void ensure_initialized()
{
	static oTHREAD_LOCAL context* sInstance = nullptr;
	if (!sInstance)
	{
		process_heap::find_or_allocate(
			"com::context"
			, process_heap::per_thread
			, process_heap::garbage_collected
			, [=](void* _pMemory) { new (_pMemory) context(); }
			, nullptr
			, &sInstance);
	}
}
		
		} // namespace com
	}
}
