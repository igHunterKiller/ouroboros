// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oCore/countof.h>
#include <oGfx/oGfxShaders.h>
#include <oCore/stringize.h>

typedef unsigned char BYTE;

struct SHADER
{
	const char* name;
	const void* byte_code;
};

#define oSH(x) { #x, x },
#define oBYTE_CODE(type) const void* byte_code(const type::value& shader) { match_array_e(s_##type, type); return s_##type[shader].byte_code; }
#define oAS_STRING(type) template<> const char* as_string(const gfx::type::value& shader) { return gfx::s_##type[shader].name; }

using namespace ouro::mesh;

namespace ouro {
	namespace gfx {

mesh::layout_t elements(const gfx_vl::value& input)
{
	layout_t e;
	e.fill(celement_t());
	switch (input)
	{
		case gfx_vl::null:
		default:
			break;
	}

	return e;
}

const void* vs_byte_code(const gfx_vl::value& input)
{
	static const gfx_vs::value sVS[] =
	{
		gfx_vs::null,
	};
	match_array_e(sVS, gfx::gfx_vl);
	return byte_code(sVS[input]);
}

static const SHADER s_gfx_vs[] = 
{
	oSH(nullptr)
};

static const SHADER s_gfx_hs[] = 
{
	oSH(nullptr)
};

static const SHADER s_gfx_ds[] = 
{
	oSH(nullptr)
};

static const SHADER s_gfx_gs[] = 
{
	oSH(nullptr)
};

static const SHADER s_gfx_ps[] = 
{
	oSH(nullptr)
};

static const SHADER s_gfx_cs[] = 
{
	oSH(nullptr)
};

oBYTE_CODE(gfx_vs)
oBYTE_CODE(gfx_hs)
oBYTE_CODE(gfx_ds)
oBYTE_CODE(gfx_gs)
oBYTE_CODE(gfx_ps)
oBYTE_CODE(gfx_cs)

	} // namespace gfx

template<> const char* as_string(const gfx::gfx_vl::value& input)
{
	static const char* sNames[] = 
	{
		"null",
	};
	match_array_e(sNames, gfx::gfx_vl);

	return sNames[input];
}

oAS_STRING(gfx_vs)
oAS_STRING(gfx_hs)
oAS_STRING(gfx_ds)
oAS_STRING(gfx_gs)
oAS_STRING(gfx_ps)
oAS_STRING(gfx_cs)

}
