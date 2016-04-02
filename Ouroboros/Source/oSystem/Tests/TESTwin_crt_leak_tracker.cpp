// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include <oCore/finally.h>
#include <oConcurrency/future.h>
#include <oCore/assert.h>
#include <oSystem/windows/win_crt_leak_tracker.h>

using namespace ouro;
using namespace windows::crt_leak_tracker;

oTEST(oSystem_win_crt_leak_tracker)
{
	#ifdef _DEBUG
		oTrace("THIS TESTS THE LEAK REPORTING CODE, SO THIS WILL INTENTIONALLY REPORT LEAKS IN THE OUTPUT AS PART OF THAT TEST.");

		enable_report(true);
	
		bool old_value = enabled();
		oFinally { enable(old_value); };
		if (!old_value)
			enable(true);

		reset();

		oCHECK(!report(), "Outstanding leaks detected at start of test");

		char* pCharAlloc = new char;
		oCHECK(report(), "Tracker failed to detect char leak");
		delete pCharAlloc;
		oCHECK(!report(), "Tracker failed to detect char delete");

		ouro::future<void> check = ouro::async([=] {});
		check.wait();
		check = ouro::future<void>(); // zero-out the future because it makes an alloc

		oCHECK(!report(), "Tracker erroneously detected leak from a different thread");

		char* pCharAllocThreaded = nullptr;

		check = ouro::async([&]() { pCharAllocThreaded = new char; });

		check.wait();
		oCHECK(report(), "Tracker failed to detect char leak from different thread");
		delete pCharAllocThreaded;
	#else
		srv.status("Leak tracker not available in release builds");
	#endif
}
