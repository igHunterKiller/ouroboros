// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/gpu_api.h>
#include <oString/stringize.h>

namespace ouro {

template<> const char* as_string<gpu_api>(const gpu_api& value)
{
	switch (value)
	{
		case gpu_api::unknown: return "unknown";
		case gpu_api::d3d11: return "d3d11";
		case gpu_api::ogl: return "ogl";
		case gpu_api::ogles: return "ogles";
		case gpu_api::webgl: return "webgl";
		case gpu_api::custom: return "custom";
		default: break;
	}
	return "?";
}

oDEFINE_TO_FROM_STRING(gpu_api);

}
