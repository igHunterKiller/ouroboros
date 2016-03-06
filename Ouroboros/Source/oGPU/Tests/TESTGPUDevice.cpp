// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include <oGPU/gpu.h>

using namespace ouro::gpu;

using namespace ouro;

void TESTdevice(unit_test::services& services)
{
	device_init init("GPU device");
	init.enable_driver_reporting = true;
	init.api_version = version_t(10,0);
	ref<device> dev = new_device(init);
	device_desc desc = dev->get_desc();

	oCHECK(desc.adapter_index == 0, "Index is incorrect");
	oCHECK(desc.feature_version >= version_t(9,0), "Invalid version retrieved");
	sstring VRAMSize, SharedSize, IVer, FVer, DVer;
	format_bytes(VRAMSize, desc.native_memory, 1);
	format_bytes(SharedSize, desc.shared_system_memory, 1);
	to_string(FVer, desc.feature_version);
	to_string(DVer, desc.driver_version);
	services.status("%s %s %s %s (%s shared) running on %s v%s drivers (%s)"
		, desc.device_description.c_str()
		, as_string(desc.api)
		, FVer.c_str()
		, VRAMSize.c_str()
		, SharedSize.c_str()
		, as_string(desc.vendor)
		, DVer.c_str()
		, desc.driver_description.c_str());
}
