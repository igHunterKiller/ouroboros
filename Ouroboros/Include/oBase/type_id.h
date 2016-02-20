// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Record a runtime generically accessible form of a type idenifier for either
// arithmetic or linear algebra types.

#pragma once
#include <oMath/hlsl.h>
#include <type_traits>

namespace ouro {

	enum class data_type
	{
		unknown,

		// arithmetic/integral types
		void_type,
		bool_type,
		char_type,
		uchar_type,
		wchar_type,
		short_type,
		ushort_type,
		int_type,
		uint_type,
		long_type,
		ulong_type,
		llong_type,
		ullong_type,

		// floating point types
		float_type,
		double_type,
		half_type,
	
		// linear algebra types
		int2_type,
		int3_type,
		int4_type,
		uint2_type,
		uint3_type,
		uint4_type,
		float2_type,
		float3_type,
		float4_type,
		float4x4_type,

		count,
	};

template<typename T> struct type_id
{
	static const int value = 
		(std::is_void<std::remove_cv<T>::type>::value                    ? type::void_type     : 
		(std::is_same<bool,std::remove_cv<T>::type>::value               ? type::bool_type     : 
		(std::is_same<char,std::remove_cv<T>::type>::value               ? type::char_type     : 
		(std::is_same<unsigned char,std::remove_cv<T>::type>::value      ? type::uchar_type    : 
		(std::is_same<wchar_t,std::remove_cv<T>::type>::value            ? type::wchar_type    : 
		(std::is_same<short,std::remove_cv<T>::type>::value              ? type::short_type    : 
		(std::is_same<unsigned short,std::remove_cv<T>::type>::value     ? type::ushort_type   : 
		(std::is_same<int,std::remove_cv<T>::type>::value                ? type::int_type      : 
		(std::is_same<unsigned int,std::remove_cv<T>::type>::value       ? type::uint_type     : 
		(std::is_same<long,std::remove_cv<T>::type>::value               ? type::long_type     : 
		(std::is_same<unsigned long,std::remove_cv<T>::type>::value      ? type::ulong_type    : 
		(std::is_same<long long,std::remove_cv<T>::type>::value          ? type::llong_type    : 
		(std::is_same<unsigned long long,std::remove_cv<T>::type>::value ? type::ullong_type   : 
		(std::is_same<half,std::remove_cv<T>::type>::value               ? type::half_type     : 
		(std::is_same<float,std::remove_cv<T>::type>::value              ? type::float_type    : 
		(std::is_same<double,std::remove_cv<T>::type>::value             ? type::double_type   : 
		(std::is_same<int2,std::remove_cv<T>::type>::value               ? type::int2_type     : 
		(std::is_same<int3,std::remove_cv<T>::type>::value               ? type::int3_type     : 
		(std::is_same<int4,std::remove_cv<T>::type>::value               ? type::int4_type     : 
		(std::is_same<uint2,std::remove_cv<T>::type>::value              ? type::uint2_type    : 
		(std::is_same<uint3,std::remove_cv<T>::type>::value              ? type::uint3_type    : 
		(std::is_same<uint4,std::remove_cv<T>::type>::value              ? type::uint4_type    : 
		(std::is_same<float2,std::remove_cv<T>::type>::value             ? type::float2_type   : 
		(std::is_same<float3,std::remove_cv<T>::type>::value             ? type::float3_type   : 
		(std::is_same<float4,std::remove_cv<T>::type>::value             ? type::float4_type   : 
		(std::is_same<float4x4,std::remove_cv<T>::type>::value           ? type::float4x4_type : 
		type::unknown))))))))))))))))))))))))));
};

template<typename T> struct is_type_id
{
	static const bool value = std::is_arithmetic<T>::value || std::is_void<T>::value || is_hlsl<T>::value;
};

}
