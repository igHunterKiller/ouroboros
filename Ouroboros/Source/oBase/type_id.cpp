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

char as_code(const data_type& type)
{
	// todo: assign a symbol for each

	static const char s_codes[] =
	{
		'?',
		'v', // void (not used)
		'B', // bool
		'c', // character
		'b', // byte (unsigned character)
		'w', // wide character,
		's', // short 16-bit
		'S', // unsigned short 16-bit
		'i', // integer 32-bit
		'u', // unsigned integer 32-bit
		'i', // integer 32-bit
		'u', // unsigned integer 32-bit
		'I', // integer 64-bit
		'U', // unsigned integer 64-bit
		'f', // float 32-bit
		'd', // double 64-bit
		'h', // half 16-bit
		'?', // int2_type, $2i
		'?', // int3_type, $3i
		'?', // int4_type, $4i
		'?', // uint2_type, $2u
		'?', // uint3_type, $3u
		'?', // uint4_type, $4u
		'?', // float2_type, $2f
		'v',
		'?', // float4_type, $4f
		'm',
	};
	match_array_e(s_codes, data_type);
	return s_codes[(int)type];
}

}
