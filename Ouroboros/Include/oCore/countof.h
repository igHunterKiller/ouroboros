// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// a safe-ish countof for determining array counts

#pragma once
#include <cstddef>

namespace ouro { namespace detail { template<typename T, size_t size> char (&sizeof_array(T(&)[size]))[size]; }}

#define countof(x) sizeof(ouro::detail::sizeof_array(x))

// ensures the count of an array matches an enum's count member value
#define match_array(arr, cnt) static_assert(countof(arr) == (size_t)cnt, "array mismatch")
#define match_array_e(arr, en) match_array(arr, en::count)
