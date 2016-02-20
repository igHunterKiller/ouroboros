// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.
#include <oBase/type_id.h>
#include <oCore/stringize.h>

namespace ouro {

template<> const char* as_string(const data_type& type)
{
	static const char* s_names[] =
	{
		"unknown type",
		"void",
		"bool",
		"char",
		"unsigned char",
		"wchar",
		"short",
		"unsigned short",
		"int",
		"unsigned int",
		"long",
		"unsigned long",
		"long long",
		"unsigned long long",
		"float",
		"double",
		"half",
		"int2",
		"int3",
		"int4",
		"uint2",
		"uint3",
		"uint4",
		"float2",
		"float3",
		"float4",
		"float4x4",
	};
	return as_string(type, s_names);
}

oDEFINE_TO_FROM_STRING(data_type);

}
