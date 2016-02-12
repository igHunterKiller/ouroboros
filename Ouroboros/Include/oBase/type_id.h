// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Record a runtime generically accessible form of a type idenifier for either
// arithmetic or linear algebra types.

#pragma once
#include <oMath/hlsl.h>
#include <type_traits>

namespace ouro {

	/* enum class */ namespace type
	{	enum value {

		unknown,

		// Arithmetic types
		// (Integral types)
		void_,
		bool_,
		char_,
		uchar_,
		wchar_,
		short_,
		ushort_,
		int_,
		uint_,
		long_,
		ulong_,
		llong_,
		ullong_,
		// (Floating point types)
		float_,
		double_,
		half_,
	
		// Linear algebra types
		int2_,
		int3_,
		int4_,
		uint2_,
		uint3_,
		uint4_,
		float2_,
		float3_,
		float4_,
		float4x4_,

		count,
	};}

template<typename T> struct type_id
{
	static const type::value value = 
		(std::is_void<std::remove_cv<T>::type>::value ? type::void_ : 
		(std::is_same<bool,std::remove_cv<T>::type>::value ? type::bool_ : 
		(std::is_same<char,std::remove_cv<T>::type>::value ? type::char_ : 
		(std::is_same<unsigned char,std::remove_cv<T>::type>::value ? type::uchar_ : 
		(std::is_same<wchar_t,std::remove_cv<T>::type>::value ? type::wchar_ : 
		(std::is_same<short,std::remove_cv<T>::type>::value ? type::short_ : 
		(std::is_same<unsigned short,std::remove_cv<T>::type>::value ? type::ushort_ : 
		(std::is_same<int,std::remove_cv<T>::type>::value ? type::int_ : 
		(std::is_same<unsigned int,std::remove_cv<T>::type>::value ? type::uint_ : 
		(std::is_same<long,std::remove_cv<T>::type>::value ? type::long_ : 
		(std::is_same<unsigned long,std::remove_cv<T>::type>::value ? type::ulong_ : 
		(std::is_same<long long,std::remove_cv<T>::type>::value ? type::llong_ : 
		(std::is_same<unsigned long long,std::remove_cv<T>::type>::value ? type::ullong_ : 
		(std::is_same<half,std::remove_cv<T>::type>::value ? type::half_ : 
		(std::is_same<float,std::remove_cv<T>::type>::value ? type::float_ : 
		(std::is_same<double,std::remove_cv<T>::type>::value ? type::double_ : 
		(std::is_same<int2,std::remove_cv<T>::type>::value ? type::int2_ : 
		(std::is_same<int3,std::remove_cv<T>::type>::value ? type::int3_ : 
		(std::is_same<int4,std::remove_cv<T>::type>::value ? type::int4_ : 
		(std::is_same<uint2,std::remove_cv<T>::type>::value ? type::uint2_ : 
		(std::is_same<uint3,std::remove_cv<T>::type>::value ? type::uint3_ : 
		(std::is_same<uint4,std::remove_cv<T>::type>::value ? type::uint4_ : 
		(std::is_same<float2,std::remove_cv<T>::type>::value ? type::float2_ : 
		(std::is_same<float3,std::remove_cv<T>::type>::value ? type::float3_ : 
		(std::is_same<float4,std::remove_cv<T>::type>::value ? type::float4_ : 
		(std::is_same<float4x4,std::remove_cv<T>::type>::value ? type::float4x4_ : 
		type::unknown))))))))))))))))))))))))));
};

template<typename T> struct is_type_id
{
	static const bool value = std::is_arithmetic<T>::value || std::is_void<T>::value || is_hlsl<T>::value;
};

}
