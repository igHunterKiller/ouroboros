// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Approximate equality for float/double types. Prefer the ULPS-based equal() 
// to this version because eps looses its meaning for larger numbers. 

#pragma once
#include <float.h>

namespace ouro {

template<typename T> bool equal_eps(const T& a, const T& b, float eps)  { T diff = a - b; diff = (diff < 0.0f ? -diff : diff); return diff <= (T)eps; }
template<typename T> bool equal_eps(const T& a, const T& b, double eps) { T diff = a - b; diff = (diff < 0.0  ? -diff : diff); return diff <= (T)eps; }
template<typename T> bool equal_eps(const T& a, const T& b)             { return equal_eps(a, b, FLT_EPSILON); }
template<> inline    bool equal_eps(const double& a, const double& b)   { return equal_eps(a, b, DBL_EPSILON); }

}
