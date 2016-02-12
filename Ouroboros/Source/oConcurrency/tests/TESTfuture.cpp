// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include <oCore/timer.h>
#include <oConcurrency/concurrency.h>
#include <oConcurrency/future.h>
#include <thread>

#include <oSystem/windows/win_crt_leak_tracker.h>

using namespace ouro;

static void exercise_thread(size_t _Index, int* _pResults, unsigned int _RuntimeMS)
{
	int n = 0;
	double MS = (double)_RuntimeMS;
	timer tm;

	while (tm.millis() < MS)
		n += rand();

	_pResults[_Index] = n;
}

static bool exercise_all_threads(ouro::unit_test::services& services)
{
	const int nTasks = 5 * std::thread::hardware_concurrency(); // ensure more work than the number of threads.
	int* results = (int*)alloca(nTasks * sizeof(int));
	memset(results, -1, nTasks * sizeof(int));

	parallel_for(0, size_t(nTasks), std::bind(exercise_thread, std::placeholders::_1, results, 2500));
	for (int i = 0; i < nTasks; i++)
		oCHECK(results[i] != -1, "Invalid results from parallel_for");
	return true;
}

static bool fail_and_report()
{
	if (1)
		throw std::invalid_argument("fail_and_report throw, this should have been caught");
	return false;
}

static void test_workstealing(ouro::unit_test::services& services)
{
	float CPUavg = 0.0f, CPUpeak = 0.0f;
	ouro::future<bool> Result = ouro::async(exercise_all_threads, std::ref(services));

	services.trace("Waiting for result...");
	bool r = Result.get();
	oCHECK(r, "future returned, but the algo returned the wrong result");

	services.get_cpu_utilization(&CPUavg, &CPUpeak);
	if (CPUpeak <= 99.0f)
	{
		float CPUpeak2 = 0.0f;
		services.reset_cpu_utilization();
		std::this_thread::sleep_for(std::chrono::seconds(10));
		services.get_cpu_utilization(&CPUavg, &CPUpeak2);
		if (CPUpeak2 > 5.0f)
			throw unit_test::skip_test("There is too much CPU activity currently on the system to properly judge ouro::future's workstealing capabilities.");
		else
			throw std::runtime_error(stringf("Failed to achieve 100%c CPU utilization. Peaked at %.01f%c", '%', CPUpeak, '%'));
	}
	else
		services.status("");
}

void TESTfuture(ouro::unit_test::services& services)
{
	{
		ouro::packaged_task<void(int, int, char*)> test_no_return([&](int _Param1, int _Param2, char*_Param3){});
		test_no_return(1,2,"t");
		test_no_return.get_future().get();
		
		// Test if reset works
		test_no_return.reset();
		test_no_return(3,4,"t");
		test_no_return.get_future().get();
	}

	// Test packaged_task with a return type
	{
		ouro::packaged_task<int(int, const char*, int)> hmmm([&](int _Param1, const char* _Param2, int _Param3)->int{ return _Param1 + _Param3; });
		// Get future before execution
		ouro::future<int> hmmfuture = hmmm.get_future();
		hmmm(10, "a", 20);
		oCHECK(hmmfuture.get() == 30, "Unexpected result1");

		// Test if reset works
		hmmm.reset();
		hmmm(20, "b", 30);
		// Get future after execution
		hmmfuture = hmmm.get_future();
		oCHECK(hmmfuture.get() == 50, "Unexpected result2");
	}

	// Test swapping packaged_tasks
	{
		ouro::packaged_task<int(int,int)> tasktest1([&](int _Param1, int _Param2)->int{ return _Param1 + _Param2; });
		ouro::packaged_task<int(int,int)> tasktest2([&](int _Param1, int _Param2)->int{ return _Param1 - _Param2; });

		oCHECK(tasktest1.valid() && tasktest2.valid(), "ouro::packaged_task should have been valid");

		tasktest1.swap(tasktest2);

		ouro::future<int> tasktest1_future = tasktest1.get_future();
		tasktest1(20, 10);

		tasktest2(20, 10);
		ouro::future<int> tasktest2_future = tasktest2.get_future();

		// tasktest1 should subtract
		oCHECK(tasktest1_future.get() == 10, "Unexpected result3");

		// tasktest2 should add
		oCHECK(tasktest2_future.get() == 30, "Unexpected result4");
	}

	// Test a packaged_task through async with maximum number of arguments
	// (std::bind is apparently limited to 10)
	{
		ouro::future<bool> Result2 = ouro::async((std::function<bool(int,int,int,int,int,int,int,int,int,int)>)[&](int _Param1,int _Param2,int _Param3,int _Param4,int _Param5,int _Param6,int _Param7,int _Param8,int _Param9,int _Param10)->bool
		{ 
			if (_Param10 == 10)
				return true; 
			else 
				return false; 
		}, 1,2,3,4,5,6,7,8,9,10);

		oCHECK(Result2.get(), "Unexpected result5");
	}
	
	// test void async()
	{
		std::atomic_int value;
		value.store(0);
		ouro::future<void> Result3 = ouro::async([&] { value++; });
		
		timer tm;
		while (tm.seconds() < 2.0) { if (value.load() != 0) break; }
		oCHECK(value != 0, "timed out waiting for void async() to finish");

		// @tony: the standard async always requires a return future. Calling get()
		// in a future's dtor seems bad since it would turn things into a sync call.
		// Still it seems there should be a way to fire-and-forget (or maybe there
		// shouldn't?) so swing back to ouro::async sometime and try to get a void
		// version.
		Result3.get();
	}

	// test failure
	{
		services.trace("Testing graceful failure - there should be some std::system_error \"not supported\" that come through.");

		ouro::future<bool> FutureToFail = ouro::async(fail_and_report);
		bool ThisShouldFail = true;

		try { ThisShouldFail = FutureToFail.get(); }
		catch (std::invalid_argument&)
		{
			ThisShouldFail = false;
		}

		oCHECK(!ThisShouldFail, "Error reporting failed");
		services.trace("Testing graceful failure - done.");
	}

	test_workstealing(services);
};
