// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Missing cbegin/cend is considered an oversight in C++11, so here it is.

#pragma once
#include <oArch/compiler.h>
#include <iterator>
#include <array>

#if oHAS_CBEGIN == 0
namespace std {
	template<typename T> auto cbegin(const T& x) -> decltype(x.cbegin()) { return x.cbegin(); }
	template<typename T> auto cend(const T& x) -> decltype(x.cend()) { return x.cend(); }
	template<typename T, size_t size> auto cbegin(const array<T, size>& x) -> decltype(x.cbegin()) { return x.cbegin(); }
	template<typename T, size_t size> auto cend(const array<T, size>& x) -> decltype(x.cend()) { return x.cend(); }
	template<typename T, int size> const T* cbegin(const T (&x)[size]) { return &x[0]; }
	template<typename T, int size> const T* cend(const T (&x)[size]) { return &x[size]; }
	template<typename T, size_t size> auto crbegin(const array<T, size>& x) -> decltype(x.crbegin()) { return x.crbegin(); }
	template<typename T, size_t size> auto crend(const array<T, size>& x) -> decltype(x.crend()) { return x.crend(); }
	template<typename T> auto crbegin(const T& x) -> decltype(x.crbegin()) { return x.crbegin(); }
	template<typename T> auto crend(const T& x) -> decltype(x.crend()) { return x.crend(); }
}
#endif
