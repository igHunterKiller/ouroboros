// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/gpu_api.h>
#include <oCore/stringize.h>

namespace ouro {

template<> const char* as_string(const gpu_api& api)
{
	static const char* s_names[] =
	{
		"unknown",
		"d3d11",
		"d3d12",
		"mantle",
		"ogl",
		"ogles",
		"webgl",
		"vulkan",
		"custom",
	};
	return as_string(api, s_names);
}

oDEFINE_TO_FROM_STRING(gpu_api);

}
