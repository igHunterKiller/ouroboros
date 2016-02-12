// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Approximate equality for float/double types. This uses ULPS (units of last 
// place) to specify the maximum number of floating-point steps, not an absolute 
// (fixed) epsilon value, so it scales across all of float's range.

#pragma once
#include <stdexcept>

#define oDEFAULT_ULPS 5

namespace ouro {

template<typename T> inline bool equal(const T& a, const T& b, unsigned int max_ulps) { return a == b; }
template<typename T> inline bool equal(const T& a, const T& b) { return equal(a, b, oDEFAULT_ULPS); }

template<> inline bool equal(const float& a, const float& b, unsigned int max_ulps)
{
	const int aInt = *(int*)&a;
	const int bInt = *(int*)&b;
	// if signs differ, return float test for +0 == -0, else it's not equal
	if ((aInt & 0x80000000) != (bInt & 0x80000000))
		return a == b;
	const unsigned int diff = aInt > bInt ? (aInt - bInt) : (bInt - aInt);
	return diff <= max_ulps;
}

template<> inline bool equal(const double& a, const double& b, unsigned int max_ulps)
{
 	const long long aInt = *(long long*)&a;
	const long long bInt = *(long long*)&b;
	// if signs differ, return float test for +0 == -0, else it's not equal
	if ((aInt & 0x8000000000000000) != (bInt & 0x8000000000000000))
		return a == b;
	const unsigned int diff = (unsigned int)(aInt > bInt ? (aInt - bInt) : (bInt - aInt));
	return diff <= max_ulps;
}

}
