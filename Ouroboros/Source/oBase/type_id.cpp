// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.
#include <oBase/type_id.h>
#include <oString/stringize.h>

namespace ouro {

template<> const char* as_string<type::value>(const type::value& _TypeID)
{
	switch (_TypeID)
	{
		default:
		case type::unknown: return "unknown_type";
		case type::bool_: return "bool";
		case type::char_: return "char";
		case type::uchar_: return "unsigned char";
		case type::wchar_: return "wchar";
		case type::short_: return "short";
		case type::ushort_: return "unsigned short";
		case type::int_: return "int";
		case type::uint_: return "unsigned int";
		case type::long_: return "long";
		case type::ulong_: return "unsigned long";
		case type::llong_: return "long long";
		case type::ullong_: return "unsigned long long";
		case type::float_: return "float";
		case type::double_: return "double";
		case type::half_: return "half";
		case type::int2_: return "int2";
		case type::int3_: return "int3";
		case type::int4_: return "int4";
		case type::uint2_: return "uint2";
		case type::uint3_: return "uint3";
		case type::uint4_: return "uint4";
		case type::float2_: return "float2";
		case type::float3_: return "float3";
		case type::float4_: return "float4";
		case type::float4x4_: return "float4x4";
	}
}

oDEFINE_ENUM_TO_STRING(type::value);
oDEFINE_ENUM_FROM_STRING(type::value);

}
