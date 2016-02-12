// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include <oGPU/gpu.h>

using namespace ouro;
using namespace ouro::gpu;

oTEST(oGPU_query)
{
	device_init init("GPU Query");
	init.enable_driver_reporting = true;
	init.api_version = version_t(10,0);
	auto dev = new_device(init);
	auto cl = dev->immediate();

	// Test timer
	{
		auto t = dev->new_timer("Timer");
		cl->begin_timer(t);
		std::this_thread::sleep_for(std::chrono::seconds(1));
		cl->end_timer(t);

		double SecondsPast = t->get_time();
		oCHECK(SecondsPast > 0.0, "No time past!");
	}
}

