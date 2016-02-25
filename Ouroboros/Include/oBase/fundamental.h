// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// enumerate types to a enum or character code

#pragma once
#include <type_traits>

namespace ouro {

enum class fundamental : unsigned char
{
	unknown,		 // '?'
	void_type,	 // 'v'
	bool_type,	 // 'B'
	char_type,	 // 'c'
	uchar_type,	 // 'b'
	wchar_type,	 // 'w'
	short_type,	 // 's'
	ushort_type, // 'S'
	int_type,		 // 'i'
	uint_type,	 // 'u'
	long_type,	 // 'i'
	ulong_type,	 // 'u'
	llong_type,	 // 'I'
	ullong_type, // 'U'
	float_type,	 // 'f'
	double_type, // 'd'
		
	count,
};

char to_code(const fundamental& f);
fundamental from_code(char c);
size_t fundamental_size(const fundamental& f); 

// same as snprintf(), but takes a type instead of a format string, and a count of how many of those should be read from src
int snprintf(char* dst, size_t dst_size, const fundamental& type, uint32_t num_elements, const void* src);

template<typename T> struct fundamental_type
{
	static const unsigned char value = (unsigned char)
		(std::is_void<std::remove_cv<T>::type>::value                    ? fundamental::void_type   :
		(std::is_same<bool,std::remove_cv<T>::type>::value               ? fundamental::bool_type   :
		(std::is_same<char,std::remove_cv<T>::type>::value               ? fundamental::char_type   :
		(std::is_same<unsigned char,std::remove_cv<T>::type>::value      ? fundamental::uchar_type  :
		(std::is_same<wchar_t,std::remove_cv<T>::type>::value            ? fundamental::wchar_type  :
		(std::is_same<short,std::remove_cv<T>::type>::value              ? fundamental::short_type  :
		(std::is_same<unsigned short,std::remove_cv<T>::type>::value     ? fundamental::ushort_type :
		(std::is_same<int,std::remove_cv<T>::type>::value                ? fundamental::int_type    :
		(std::is_same<unsigned int,std::remove_cv<T>::type>::value       ? fundamental::uint_type   :
		(std::is_same<long,std::remove_cv<T>::type>::value               ? fundamental::long_type   :
		(std::is_same<unsigned long,std::remove_cv<T>::type>::value      ? fundamental::ulong_type  :
		(std::is_same<long long,std::remove_cv<T>::type>::value          ? fundamental::llong_type  :
		(std::is_same<unsigned long long,std::remove_cv<T>::type>::value ? fundamental::ullong_type :
		(std::is_same<float,std::remove_cv<T>::type>::value              ? fundamental::float_type  :
		(std::is_same<double,std::remove_cv<T>::type>::value             ? fundamental::double_type :
		fundamental::unknown)))))))))))))));
};

template<typename T> struct fundamental_code
{
	static const char value = "?vBcbwsSiuiuIUfd"[fundamental_type<T>::value];
};

template<typename T> struct is_fundamental
{
	static const bool value = std::is_arithmetic<T>::value || std::is_void<T>::value;
};

}
 