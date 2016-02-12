// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <memory.h>
#include <cstdint>

// Visual Studio 2012 Win32 compiler's conversion (c-cast, static_cast) is 
// bugged, so here's a software version.
namespace ouro {

/** <citation
	usage="Implementation" 
	reason="win32 compiler double -> uint64_t is not correct, this is"
	author="Erik Rufelt"
	description="http://www.gamedev.net/topic/591975-c-unsigned-__int64-tofrom-double-conversions/page__st__20"
	license="*** Assumed Public Domain ***"
	licenseurl="http://www.gamedev.net/topic/591975-c-unsigned-__int64-tofrom-double-conversions/page__st__20"
	modification="assert -> static assert. uint64 -> uint64_t"
/>*/
// $(CitedCodeBegin)
uint64_t dtoull(double input)
{
	static_assert(sizeof(double) == 8, "sizeof(double) == 8");

	// Get the bits representing the double
	double d = input;
	uint64_t doubleBits;
	memcpy(reinterpret_cast<int8_t*>(&doubleBits), reinterpret_cast<int8_t*>(&d), 8);

	// Check for a negative number
	uint64_t signBit = (doubleBits >> uint64_t(63)) & uint64_t(0x1);
	if(signBit != uint64_t(0))
		return uint64_t(0);

	// Get the exponent
	uint64_t exponent = (doubleBits >> uint64_t(52)) & uint64_t(0x7ff);

	// The number is larger than the largest uint64
	if(exponent > uint64_t(1086))
		return uint64_t(0);

	// The number is less than 1
	if(exponent < uint64_t(1023))
		return uint64_t(0);

	// Get the fraction
	uint64_t fraction = (doubleBits & uint64_t(0xfffffffffffff)) | uint64_t(0x10000000000000);

	// Calculate and return integer part
	if(exponent >= uint64_t(1075))
		return fraction << (exponent - uint64_t(1075));
	else
		return fraction >> (uint64_t(1075) - exponent);
}
// $(CitedCodeEnd)

}
