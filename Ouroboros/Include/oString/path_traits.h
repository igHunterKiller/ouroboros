// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// traits of file path rules used in ouro::path

#pragma once
#include <oMemory/fnv1a.h>
#include <oString/string_path.h>

#define oMAX_PATH 512

namespace ouro {

template<typename charT, bool is_posix>
struct base_path_traits
{
	typedef charT char_type;
	static const bool posix = is_posix;
	static bool is_dot(char_type c);
	static bool is_sep(char_type c);
	static bool is_native_sep(char_type c);
	static bool is_unc(const char_type* path);
	static bool has_vol(const char_type* path);
	static char_type* generic_sep_str();
	static char_type* native_sep_str();
	static char_type* dot_str();
	static char_type* dotdot_str();
	static char_type* empty_str();
	static size_t cmnroot(const char_type* path1, const char_type* path2);
	static int compare(const char_type* path1, const char_type* path2); // strcmp/stricmp
	static size_t hash(const char_type* path);
};

template<> struct base_path_traits<char, false>
{
	typedef char char_type;
	static const bool posix = false;
	static bool is_dot(char_type c) { return c == '.'; }
	static bool is_sep(char_type c) { return c == '/' || c == '\\'; }
	static bool is_native_sep(char_type c) { return c == '\\'; }
	static bool is_unc(const char_type* path) { return path && is_sep(path[0]) && is_sep(path[1]) && !is_sep(path[2]); }
	static bool has_vol(const char_type* path) { return path && path[0] && path[1] == ':';  }
	static char_type generic_sep_chr() { return '/'; }
	static char_type native_sep_chr() { return '\\'; }
	static char_type* generic_sep_str() { return "/"; }
	static char_type* native_sep_str() { return "\\"; }
	static char_type* dot_str() { return "."; }
	static char_type* dotdot_str() { return ".."; }
	static char_type* empty_str() { return ""; }
	static size_t cmnroot(const char_type* path1, const char_type* path2) { return cmnroot(path1, path2); }
	static int compare(const char_type* path1, const char_type* path2) { return _stricmp(path1, path2); }
	static size_t hash(const char_type* path) { return fnv1ai<size_t>(path); }
};

template<> struct base_path_traits<char, true>
{
	typedef char char_type;
	static const bool posix = true;
	static bool is_dot(char_type c) { return c == '.'; }
	static bool is_sep(char_type c) { return c == '/'; }
	static bool is_native_sep(char_type c) { return c == '/'; }
	static bool is_unc(const char_type* path) { return path && is_sep(path[0]) && is_sep(path[1]) && !is_sep(path[2]); }
	static bool has_vol(const char_type* path) { return false;  }
	static char_type generic_sep_chr() { return '/'; }
	static char_type native_sep_chr() { return '/'; }
	static char_type* generic_sep_str() { return "/"; }
	static char_type* native_sep_str() { return "/"; }
	static char_type* dot_str() { return "."; }
	static char_type* dotdot_str() { return ".."; }
	static char_type* empty_str() { return ""; }
	static size_t cmnroot(const char_type* path1, const char_type* path2) { return cmnroot(path1, path2); }
	static int compare(const char_type* path1, const char_type* path2) { return strcmp(path1, path2); }
	static size_t hash(const char_type* path) { return fnv1a<size_t>(path); }
};

template<> struct base_path_traits<wchar_t, false>
{
	typedef wchar_t char_type;
	static const bool posix = false;
	static bool is_dot(char_type c) { return c == L'.'; }
	static bool is_sep(char_type c) { return c == L'/' || c == L'\\'; }
	static bool is_native_sep(char_type c) { return c == L'\\'; }
	static bool is_unc(const char_type* path) { return path && is_sep(path[0]) && is_sep(path[1]) && !is_sep(path[2]); }
	static bool has_vol(const char_type* path) { return path && path[0] && path[1] == L':';  }
	static char_type generic_sep_chr() { return L'/'; }
	static char_type native_sep_chr() { return L'/'; }
	static char_type* generic_sep_str() { return L"/"; }
	static char_type* native_sep_str() { return L"\\"; }
	static char_type* dot_str() { return L"."; }
	static char_type* dotdot_str() { return L".."; }
	static char_type* empty_str() { return L""; }
	static size_t cmnroot(const char_type* path1, const char_type* path2) { return wcmnroot(path1, path2); }
	static int compare(const char_type* path1, const char_type* path2) { return _wcsicmp(path1, path2); }
	static size_t hash(const char_type* path) { return fnv1ai<size_t>(path); }
};

template<> struct base_path_traits<wchar_t, true>
{
	typedef wchar_t char_type;
	static const bool posix = true;
	static bool is_dot(char_type c) { return c == L'.'; }
	static bool is_sep(char_type c) { return c == L'/'; }
	static bool is_native_sep(char_type c) { return c == L'/'; }
	static bool is_unc(const char_type* path) { return path && is_sep(path[0]) && is_sep(path[1]) && !is_sep(path[2]); }
	static bool has_vol(const char_type* path) { return false;  }
	static char_type generic_sep_chr() { return L'/'; }
	static char_type native_sep_chr() { return L'/'; }
	static char_type* generic_sep_str() { return L"/"; }
	static char_type* native_sep_str() { return L"/"; }
	static char_type* dot_str() { return L"."; }
	static char_type* dotdot_str() { return L".."; }
	static char_type* empty_str() { return L""; };
	static size_t cmnroot(const char_type* path1, const char_type* path2) { return wcmnroot(path1, path2); }
	static int compare(const char_type* path1, const char_type* path2) { return wcscmp(path1, path2); }
	static size_t hash(const char_type* path) { return fnv1a<size_t>(path); }
};

template<typename charT, bool is_posix, bool _always_clean>
struct path_traits : base_path_traits<charT, is_posix>
{
	static const bool always_clean = _always_clean;
};

template<typename charT>
struct default_posix_path_traits : path_traits<charT, true, true> {};

template<typename charT>
struct default_windows_path_traits : path_traits<charT, false, true> {};

}
