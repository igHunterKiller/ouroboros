// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include <oSystem/cpu.h>
#include <oString/stringize.h>

using namespace ouro;

oTEST(oSystem_cpu)
{
	cpu::info_t info = cpu::get_info();
	// 1 = hyperthreading; 2 = AVX1
	int has_flags = 0;

	cpu::enumerate_features([](const char* feature, const cpu::support& support, void* user)->bool
	{
		int& has_flags = *(int*)user;
		if (!_stricmp("Hyperthreading", feature))
			has_flags |= support == cpu::support::full ? 1 : 0;
		else if (!_stricmp("AVX1", feature))
			has_flags |= support == cpu::support::full ? 2 : 0;
		return true;
	}, &has_flags);

	srv.status("%s %s %s%s%s %d HWThreads", ouro::as_string(info.type), info.string, info.brand_string, (has_flags&1) ? " HT" : "", (has_flags&2) ? " AVX" : "", info.hardware_thread_count);
}
