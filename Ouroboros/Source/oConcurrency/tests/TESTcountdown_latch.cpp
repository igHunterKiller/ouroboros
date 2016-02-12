// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include <oConcurrency/countdown_latch.h>
#include <oConcurrency/future.h>
#include <atomic>
#include <thread>
#include <vector>

using namespace ouro;

static bool test(int _Count)
{
	int latchCount = _Count;
	std::atomic<int> count(0);
	countdown_latch latch(latchCount);
	
	// NOTE: This pattern is only for testing purposes of countdown latch. In 
	// applications, it's better to async() a task that will then do other tasks
	// and wait on the future of that first async() call... such as async()'ing a 
	// parallel_for. Again, this pattern here is to purposefully exacerbate 
	// and test countdown_latch, thus sync'ing on the futures wouldn't do much 
	// good.

	std::vector<ouro::future<void>> Futures;

	for (int i = 0; i < _Count; i++)
	{
		ouro::future<void> f = ouro::async([&,i]
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100 * (i + 1))); // stagger the sleeps to simulate doing work that takes a variable amount of time
			count++;
			latch.release();
		});

		Futures.push_back(std::move(f));
	}

	latch.wait();

	for (ouro::future<void>& f : Futures)
		f.wait();

	return count == latchCount;
}

oTEST(oConcurrency_countdown_latch)
{
	oCHECK(test(5), "countdown_latch failed to wait properly.");
	oCHECK(test(1), "countdown_latch failed to wait properly.");
}
