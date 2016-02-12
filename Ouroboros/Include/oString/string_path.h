// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// string manipulation for file paths

#pragma once
#include <functional>

namespace ouro {

// If zero_buffer is true, all extra chars in the destination will be set to 
// zero such that a memcmp of two cleaned buffers would be reliable.
char* clean_path(char* dst, size_t dst_size, const char* src_path, char separator = '/', bool zero_buffer = false);
template<size_t size> char* clean_path(char (&dst)[size], const char* src_path, char separator = '/', bool zero_buffer = false) { return clean_path(dst, size, src_path, separator, zero_buffer); }

// Fills dst with a version of FullPath that has all the common 
// parts between base_path and full_path removed and additionally has ../ 
// relative paths inserted until it is relative to the base path.
char* relativize_path(char* dst, size_t dst_size, const char* base_path, const char* full_path);
template<size_t size> char* relativize_path(char (&dst)[size], const char* base_path, const char* full_path) { return relativize_path(dst, size, base_path, full_path); }

// Standard Unix/MS-DOS style wildcard matching
bool matches_wildcard(const char* wildcard, const char* path);

// Returns the number of characters common to both specified paths.
size_t cmnroot(const char* path1, const char* path2);
size_t wcmnroot(const wchar_t* path1, const wchar_t* path2);

// Fills pointers into the specified path where different components start. If
// the component value does not exists, the pointer is filled with nullptr. This 
// returns the length of path.
size_t split_path(const char* path
	, bool posix
	, const char** out_root
	, const char** out_path
	, const char** out_parent_path_end
	, const char** out_basename
	, const char** out_ext);

size_t wsplit_path(const wchar_t* path
	, bool posix
	, const wchar_t**		
	, const wchar_t** out_path
	, const wchar_t** out_parent_path_end
	, const wchar_t** out_basename
	, const wchar_t** out_ext);

template<typename charT>
size_t tsplit_path(const charT* path
	, bool posix
	, const charT** out_root
	, const charT** out_path
	, const charT** out_parent_path_end
	, const charT** out_basename
	, const charT** out_ext)
{ return split_path(path, posix, out_root, out_path, out_parent_path_end, out_basename, out_ext); }

template<>
inline size_t tsplit_path(const wchar_t* path
	, bool posix
	, const wchar_t** out_root
	, const wchar_t** out_path
	, const wchar_t** out_parent_path_end
	, const wchar_t** out_basename
	, const wchar_t** out_ext)
{ return wsplit_path(path, posix, out_root, out_path, out_parent_path_end, out_basename, out_ext); }

// search_paths is a semi-colon-delimited set of strings (like Window's PATH
// environment variable). dot_path is a path to append if relative_path begins
// with a '.', like "./myfile.txt". This function goes through each of 
// search_paths paths and dot_path and prepends it to relative_path. If the 
// result causes the path_exists function to return true the full path is copied 
// into dst and a pointer to dst is returned. If all
// paths are exhausted without passing path_exists nullptr is returned.
char* search_path(char* dst
	, size_t dst_size
	, const char* search_paths
	, const char* relative_path
	, const char* dot_path
	, const std::function<bool(const char* path)>& path_exists);

template<size_t size>
char* search_path(char (&dst)[size]
	, const char* search_paths
	, const char* relative_path
	, const char* dot_path
	, const std::function<bool(const char* path)>& path_exists)
{ return search_path(dst, size, search_paths, relative_path, dot_path, path_exists); }

}
