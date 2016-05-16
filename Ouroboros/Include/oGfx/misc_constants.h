// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Some basic cbuffer structs for things that don't quite fit an existing 
// cbuffer and aren't complex enough to warrant their own cbuffer definition.

#ifndef oHLSL
	#pragma once
#endif
#ifndef oGfx_misc_constants_h
#define oGfx_misc_constants_h

#ifndef oHLSL
#include <oArch/compiler.h>
#include <oMath/hlsl.h>
namespace ouro { namespace gfx {
#else
#define alignas(x)
#endif

struct alignas(16) misc_constants
{
	float4 a;
	float4 b;
	float4 c;
	float4 d;
};

#ifndef oHLSL
}}
#else
oGFX_DECLARE_MISC_CONSTANTS(oGfx_misc_constants);
#endif
#endif
