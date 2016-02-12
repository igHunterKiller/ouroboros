// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.
#include <oSystem/thread_traits.h>
#include <oSystem/debugger.h>
#include <oSystem/process_heap.h>

namespace ouro {

void core_thread_traits::begin_thread(const char* _ThreadName)
{
	debugger::thread_name(_ThreadName);
}

void core_thread_traits::update_thread()
{
}

void core_thread_traits::end_thread()
{
	process_heap::exit_thread();
}

void core_thread_traits::at_thread_exit(const std::function<void()>& _AtExit)
{
	throw std::system_error(std::errc::operation_not_supported, std::system_category());
}

}
