// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#pragma once
#include <oBase/unit_test.h>
#include <functional>

namespace ouro {

// Utility functions, do not register these as tests.
struct test_threadpool
{
public:

	// returns the name used to identify this threadpool for test's report.
	virtual const char* name() const = 0;

	// dispatches a single task for execution on any thread. There is no execution
	// order guarantee.
	virtual void dispatch(const std::function<void()>& _Task) = 0;

	// parallel_for basically breaks up some dispatch calls to be executed on 
	// worker threads. If the underlying threadpool does not support parallel_for,
	// this should return false.
	virtual bool parallel_for(size_t begin, size_t end, const std::function<void(size_t index)>& task) = 0;

	// waits for the threadpool to be empty. The threadpool must be reusable after
	// this call (this is not join()).
	virtual void flush() = 0;

	// Release the threadpool reference obtained by enumerate_threadpool below.
	virtual void release() = 0;
};

// This can be used to do an apples-to-apples benchmark with various 
// threadpools, just implement test_threadpool and pass it to this test.
void TESTthreadpool_performance(ouro::unit_test::services& services, test_threadpool& threadpool);

// Implement this inside a TESTMyThreadpool() function.
template<typename test_threadpool_impl_t>
void TESTthreadpool_performance_impl(ouro::unit_test::services& services)
{
	struct rel_on_exit
	{
		test_threadpool_impl_t& pool;
		rel_on_exit(test_threadpool_t& tp) : pool(tp) {}
		~rel_on_exit() { pool.release(); }
	};

	test_threadpool_impl_t tp;
	rel_on_exit roe(tp);
	TESTthreadpool_performance(services, tp);
}

}
