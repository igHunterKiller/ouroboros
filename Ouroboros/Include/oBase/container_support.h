// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Declaration of common functors for standard containers.

#pragma once
#include <functional>

namespace ouro {

template<typename T> struct less : public std::binary_function<T, T, bool> { bool operator()(const T& x, const T& y) const { return x < y; } };
template<typename T> struct less_i: public std::binary_function<T, T, bool> { bool operator()(const T& x, const T& y) const { return x < y; } };

template<typename T> struct same : public std::binary_function<T, T, bool> { bool operator()(const T& x, const T& y) const { return x == y; } };
template<typename T> struct same_i : public std::binary_function<T, T, bool> { bool operator()(const T& x, const T& y) const { return x == y; } };

// _____________________________________________________________________________
// standard container support

template<> struct less<const char*> { int operator()(const char* x, const char* y) const { return strcmp(x, y) < 0; } };
template<> struct less_i<const char*> { bool operator()(const char* x, const char* y) const { return _stricmp(x, y) < 0; } };

template<> struct same<const char*> { int operator()(const char* x, const char* y) const { return !strcmp(x, y); } };
template<> struct same_i<const char*> { bool operator()(const char* x, const char* y) const { return !_stricmp(x, y); } };

}
