// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Traits of uri rules used in ouro::uri

#pragma once
#include <oString/path_traits.h>

#define oMAX_URI 2048

namespace ouro {

template<typename charT, typename hashT, typename path_traitsT = default_posix_path_traits<charT>>
struct uri_traits
{
	typedef charT char_type;
	typedef size_t size_type;
	typedef hashT hash_type;
	typedef path_traitsT path_traits_type;
	typedef basic_path_t<char_type, path_traits_type> path_type;
	static const size_type capacity = oMAX_URI;

	static const char_type* empty_str();
	static const char_type* scheme_str();
	static const char_type* sep_str();
	static const char_type* double_sep_str();
	static const char_type* query_str();
	static const char_type* fragment_str();
	static const char_type* file_scheme_prefix_str();

	static bool is_file_scheme(const char_type* uri);

	static int compare(const char_type* uri1, const char_type* uri2);
};

template<> struct uri_traits<char, uint64_t, default_posix_path_traits<char>>
{
	typedef char char_type;
	typedef size_t size_type;
	typedef uint64_t hash_type;
	typedef default_posix_path_traits<char_type> path_traits_type;
	typedef basic_path_t<char_type, path_traits_type> path_type;
	static const size_type capacity = oMAX_URI;

	static const char_type* empty_str() { return ""; }
	static const char_type* scheme_str() { return ":"; }
	static const char_type* sep_str() { return "/"; }
	static const char_type* double_sep_str() { return "//"; }
	static const char_type* query_str() { return "?"; }
	static const char_type* fragment_str() { return "#"; }
	static const char_type* file_scheme_prefix_str() { return "file://"; }

	static bool is_file_scheme(const char_type* uri) { return 0 == _memicmp("file", uri, 4); }
	static int compare(const char_type* uri1, const char_type* uri2) { return strcmp(uri1, uri2); }
};

template<> struct uri_traits<wchar_t, uint64_t, default_posix_path_traits<wchar_t>>
{
	typedef wchar_t char_type;
	typedef size_t size_type;
	typedef uint64_t hash_type;
	typedef default_posix_path_traits<char_type> path_traits_type;
	typedef basic_path_t<char_type, path_traits_type> path_type;
	static const size_type capacity = oMAX_URI;

	static const char_type* empty_str() { return L""; }
	static const char_type* scheme_str() { return L":"; }
	static const char_type* sep_str() { return L"/"; }
	static const char_type* double_sep_str() { return L"//"; }
	static const char_type* query_str() { return L"?"; }
	static const char_type* fragment_str() { return L"#"; }
	static const char_type* file_scheme_prefix_str() { return L"file://"; }

	static bool is_file_scheme(const char_type* uri) { return 0 == _memicmp(L"file", uri, 8); }
	static int compare(const char_type* uri1, const char_type* uri2) { return wcscmp(uri1, uri2); }
};

}
