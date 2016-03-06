// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include <oSystem/adapter.h>
#include <oCore/assert.h>

using namespace ouro;

oTEST(oSystem_adapter)
{
	struct ctx_t
	{
		ctx_t(unit_test::services& services) : services(services), n(0) {}
		unit_test::services& services;
		int n;
	};

	ctx_t ctx(srv);

	adapter::enumerate([](const adapter::info_t& info, void* user)->bool
	{
		auto& ctx = *(ctx_t*)user;

		sstring str_ver;
		to_string(str_ver, info.version);
		version_t min_ver = adapter::minimum_version(info.vendor);
		if (!ctx.n)
			ctx.services.trace("%s v%s%s", info.description.c_str(), str_ver.c_str(), info.version >= min_ver ? " (meets version requirements)" : "below min requirments");
		oTrace("%s v%s%s", info.description.c_str(), str_ver.c_str(), info.version >= min_ver ? " (meets version requirements)" : "below min requirments");
		ctx.n++;
		return true;
	}, &ctx);
};
