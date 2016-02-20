// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// NOTE: these must be used inside namespace ouro::gfx

#pragma once
#include <oCore/countof.h>

typedef unsigned char BYTE; // for shader compiled code

struct oSHshader { const void* bytecode; const char* name; };

#define oSH_BEGIN(type) static const oSHshader s_##type[] =
#define oSH(bc) { bc, #bc }
#define oSH_NULL { nullptr, "none" }

#define oSH_END_COMMON__(type) \
	} template<> const char* as_string(const gfx::type& x) { return gfx::s_##type[(int)x].name; } namespace gfx { \
	match_array_e(s_##type, type);

#define oSH_END(type) oSH_END_COMMON__(type) \
	const void* bytecode(const type& x) { return s_##type[(int)x].bytecode; }

#define oSH_END_SIG(type) oSH_END_COMMON__(type) \
	const void* sig_bytecode(const type& x) { return s_##type[(int)x].bytecode; }
