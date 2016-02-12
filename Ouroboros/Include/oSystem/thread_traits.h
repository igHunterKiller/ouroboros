// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Use oCore functionality for more robust threads. Pass this to threadpool 
// and derivatives.

#pragma once
#include <oBase/callable.h>

namespace ouro {

struct core_thread_traits
{
	static void begin_thread(const char* _ThreadName);
	static void update_thread();
	static void end_thread();
	static void at_thread_exit(const std::function<void()>& _AtExit);
	oDEFINE_CALLABLE_WRAPPERS(static, at_thread_exit,, at_thread_exit);
};

}
