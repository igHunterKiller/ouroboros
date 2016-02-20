// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Enumeration of common gpu apis

#pragma once
#include <cstdint>

namespace ouro {

enum class gpu_api : uint8_t
{
	unknown,
	d3d11,
	d3d12,
	mantle,
	ogl,
	ogles,
	webgl,
	vulkan,
	custom,

	count,
};

}
