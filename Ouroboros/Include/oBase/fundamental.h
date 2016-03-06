// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// classify C++'s builtin fundamental types

#pragma once
#include <type_traits>

namespace ouro {

enum class fundamental : unsigned char
{
	unknown_type, // '?'
	void_type,    // 'v'
	bool_type,	  // 'B'
	char_type,	  // 'c'
	uchar_type,	  // 'b'
	wchar_type,	  // 'w'
	short_type,	  // 's'
	ushort_type,  // 'S'
	int_type,		  // 'i'
	uint_type,	  // 'u'
	long_type,	  // 'i'
	ulong_type,	  // 'u'
	llong_type,	  // 'I'
	ullong_type,  // 'U'
	float_type,	  // 'f'
	double_type,  // 'd'
		
	count,
};

constexpr char fundamental_to_code_const(const fundamental& f)
{
	return f == fundamental::void_type   ? 'v' :
	       f == fundamental::bool_type   ? 'B' :
	       f == fundamental::char_type	 ? 'c' :
	       f == fundamental::uchar_type	 ? 'b' :
	       f == fundamental::wchar_type	 ? 'w' :
	       f == fundamental::short_type	 ? 's' :
	       f == fundamental::ushort_type ? 'S' :
	       f == fundamental::int_type		 ? 'i' :
	       f == fundamental::uint_type	 ? 'u' :
	       f == fundamental::long_type	 ? 'i' :
	       f == fundamental::ulong_type	 ? 'u' :
	       f == fundamental::llong_type	 ? 'I' :
	       f == fundamental::ullong_type ? 'U' :
	       f == fundamental::float_type	 ? 'f' :
	       f == fundamental::double_type ? 'd' :
	                                       '?' ;
}

constexpr fundamental fundamental_from_code_const(char c)
{
	return c == 'v' ? fundamental::void_type   :
	       c == 'B' ? fundamental::bool_type   :
	       c == 'c' ? fundamental::char_type	 :
	       c == 'b' ? fundamental::uchar_type	 :
	       c == 'w' ? fundamental::wchar_type	 :
	       c == 's' ? fundamental::short_type	 :
	       c == 'S' ? fundamental::ushort_type :
	       c == 'i' ? fundamental::int_type		 :
	       c == 'u' ? fundamental::uint_type	 :
	       c == 'i' ? fundamental::long_type	 :
	       c == 'u' ? fundamental::ulong_type	 :
	       c == 'I' ? fundamental::llong_type	 :
	       c == 'U' ? fundamental::ullong_type :
	       c == 'f' ? fundamental::float_type	 :
	       c == 'd' ? fundamental::double_type :
	                  fundamental::unknown_type;
}

constexpr size_t fundamental_size_const(const fundamental& f)
{
	return f == fundamental::void_type   ? 0 :
	       f == fundamental::bool_type   ? 1 :
	       f == fundamental::char_type	 ? 1 :
	       f == fundamental::uchar_type	 ? 1 :
	       f == fundamental::wchar_type	 ? 2 :
	       f == fundamental::short_type	 ? 2 :
	       f == fundamental::ushort_type ? 2 :
	       f == fundamental::int_type		 ? 4 :
	       f == fundamental::uint_type	 ? 4 :
	       f == fundamental::long_type	 ? 4 :
	       f == fundamental::ulong_type	 ? 4 :
	       f == fundamental::llong_type	 ? 8 :
	       f == fundamental::ullong_type ? 8 :
	       f == fundamental::float_type	 ? 4 :
	       f == fundamental::double_type ? 8 :
	                                       0 ;
}

constexpr const char* fundamental_format_const(const fundamental& f)
{
	return (f == fundamental::bool_type  || f == fundamental::int_type  || f == fundamental::long_type ) ? "%d"   :
	       (f == fundamental::uchar_type || f == fundamental::uint_type || f == fundamental::ulong_type) ? "%u"   :
	       (f == fundamental::short_type)                                                                ? "%hd"  :
	       (f == fundamental::ushort_type)                                                               ? "%hu"  :
	       (f == fundamental::llong_type)                                                                ? "%lld" :
	       (f == fundamental::ullong_type)                                                               ? "%llu" :
	       (f == fundamental::float_type || f == fundamental::double_type)                               ? "%f"   :
	       (f == fundamental::char_type)                                                                 ? "%c"   :
	       (f == fundamental::wchar_type)                                                                ? "%lc"  :
	       (f == fundamental::void_type)                                                                 ? "void" :
                                                                                                         "?"    ;
}

size_t      fundamental_size     (const fundamental& f);
const char* fundamental_format   (const fundamental& f);
char        fundamental_to_code  (const fundamental& f);
fundamental fundamental_from_code(char c);

template<typename T> struct is_fundamental
{
	static const bool value = std::is_arithmetic<T>::value || std::is_void<T>::value;
};

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

// converts typed src to a string
                                  size_t fundamental_to_string(char* dst, size_t dst_size, const fundamental& type, const void* src, uint32_t num_elements = 1);
template<typename T>              size_t fundamental_to_string(char* dst, size_t dst_size, const T* src, uint32_t num_elements) { return to_string(dst, dst_size, fundamental_type<T>::value, src, num_elements); }
template<typename T>              size_t fundamental_to_string(char* dst, size_t dst_size, const T& src)                        { return to_string(dst, dst_size, fundamental_type<T>::value, &src); }
template<typename T, size_t size> size_t fundamental_to_string(char (&dst)[size],          const T* src, uint32_t num_elements) { return to_string(dst, size, fundamental_type<T>::value, src, num_elements); }
template<typename T, size_t size> size_t fundamental_to_string(char (&dst)[size],          const T& src)                        { return to_string(dst, size, fundamental_type<T>::value, &src); }

}
